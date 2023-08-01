#include "insufficient_checks_pass.h"

#include <set>
#include <string>

#include "checker_common.h"
#include "glog/logging.h"
#include "llvm.h"
#include "llvm/IR/Function.h"
#include "llvm/Pass.h"
#include "proto/checker.pb.h"
#include "return_constraints_pass.h"
#include "return_propagation_pass.h"
#include "tbb/tbb.h"

namespace error_specifications {

bool InsufficientChecksPass::runOnModule(llvm::Module &module) {
  std::vector<const llvm::Function *> module_functions;
  for (const llvm::Function &fn : module) {
    module_functions.push_back(&fn);
  }

  tbb::parallel_for(
      tbb::blocked_range<std::vector<const llvm::Function *>::iterator>(
          module_functions.begin(), module_functions.end()),
      [&](auto thread_functions) {
        for (const llvm::Function *function : thread_functions) {
          for (const auto &basic_block : *function) {
            for (const auto &inst : basic_block) {
              if (const llvm::CallInst *call =
                      llvm::dyn_cast<llvm::CallInst>(&inst)) {
                this->VisitCallInst(*call);
              }
            }
          }
        }
      });

  LOG(INFO) << "Insufficient checks pass finished.";

  return false;
}

void InsufficientChecksPass::SetViolationsRequest(
    const GetViolationsRequest &request) {
  violations_request_.CopyFrom(request);

  for (const Specification &spec : violations_request_.specifications()) {
    functions_to_check_[spec.function().source_name()] = spec;
  }
}

std::set<SignLatticeElement> InsufficientChecksPass::CollectConstraints(
    const llvm::Function &parent_function, const std::string &fn_name) {
  std::set<SignLatticeElement> ret;

  ReturnConstraintsPass &return_constraints_pass =
      getAnalysis<ReturnConstraintsPass>();

  // Go over every instruction in parent_function.
  for (auto &basic_block : parent_function) {
    for (auto &inst : basic_block) {
      ReturnConstraintsFact return_constraints_fact =
          return_constraints_pass.GetInFact(&inst);
      const auto &fn_constraint = return_constraints_fact.value.find(fn_name);
      // If there there is constraint on the instruction associated with
      // fn_name.
      if (fn_constraint != return_constraints_fact.value.end()) {
        // Then add that constraint to ret.
        ret.insert(fn_constraint->second.lattice_element);
      }
    }
  }

  return ret;
}

bool InsufficientChecksPass::IsSufficientlyChecked(
    const llvm::CallInst &call_instruction,
    const Specification &specification) {
  Function callee = GetCallee(call_instruction);

  // Collect constraints with respect to the called function.
  // Use LLVM name here because the intraprocedural passes use the LLVM name.
  auto callee_constraints = CollectConstraints(
      *(call_instruction.getParent()->getParent()), callee.llvm_name());

  // ReturnConstraints returns the constraints on function return values
  // under which code is live. Taking the complement gives us the constraints
  // under which the code cannot execute.
  std::set<SignLatticeElement> dead_constraints;
  for (const auto &x : callee_constraints) {
    dead_constraints.insert(SignLattice::Complement(x));
  }

  // Take meet of every combination of elements in the power set.
  // We don't need the full powerset. Just one or two elements will suffice.
  // If the meet is greater than or equal to the error specification, then OK.
  //
  // Insert top because meet with top is identity. Ensures that we
  // check one element in addition to the meet of two elements.
  dead_constraints.insert(SignLatticeElement::SIGN_LATTICE_ELEMENT_TOP);
  // In order to differentiate between insufficient, unused, and unchecked
  // violations we need to make sure that if all the meet operations result
  // in TOP that we just ignore the result, even if it is a violation. This
  // is because unused/unchecked violations are already checked separately,
  // meaning that this can also result in many false positives.
  bool only_top = true;
  for (const auto &e1 : dead_constraints) {
    for (const auto &e2 : dead_constraints) {
      SignLatticeElement meet = SignLattice::Meet(e1, e2);
      if (meet == SIGN_LATTICE_ELEMENT_BOTTOM) {
        continue;
      }
      if (meet != SIGN_LATTICE_ELEMENT_TOP) {
        only_top = false;
      }
      SignLatticeElement meet_complement = SignLattice::Complement(meet);
      if (SignLattice::IsLessThan(specification.lattice_element(),
                                  meet_complement)) {
        // Sufficiently checked.
        return true;
      }
    }
  }

  // We ignore these types of violations here.
  if (only_top) {
    return true;
  }

  return false;
}

bool InsufficientChecksPass::IsPropagated(
    const llvm::CallInst &call_instruction) {
  ReturnPropagationPass &return_propagation_pass =
      getAnalysis<ReturnPropagationPass>();

  for (auto &basic_block : *call_instruction.getParent()->getParent()) {
    for (auto &inst : basic_block) {
      const llvm::ReturnInst *return_inst =
          llvm::dyn_cast<llvm::ReturnInst>(&inst);
      if (!return_inst) {
        continue;
      }

      // The fact at the return instruction.
      auto return_fact = return_propagation_pass.input_facts_.at(return_inst);
      if (return_inst->getNumOperands() != 1) {
        continue;
      }

      // Get the values that can be returned.
      llvm::Value *returned = return_inst->getOperand(0);
      const auto &idx = return_fact->value.find(returned);
      if (idx == return_fact->value.end()) {
        continue;
      }

      // Not a bug if the return value of call instruction is being propagated.
      if (idx->second.find(&call_instruction) != idx->second.end()) {
        // Propagated.
        return true;
      }
    }
  }

  return false;
}

void InsufficientChecksPass::VisitCallInst(
    const llvm::CallInst &call_instruction) {
  const Function function_to_check = GetCallee(call_instruction);

  const std::string &function_name = function_to_check.source_name();
  auto spec_it = functions_to_check_.find(function_name);
  if (spec_it == functions_to_check_.end()) {
    return;
  }
  const Specification &specification = spec_it->second;

  if (ShouldCheck(function_to_check, specification) == false) {
    return;
  }

  // This is a buggy call. Create a violation message.
  if (!IsPropagated(call_instruction) &&
      !IsSufficientlyChecked(call_instruction, specification)) {
    Violation violation;
    violation.mutable_location()->CopyFrom(GetDebugLocation(call_instruction));
    violation.mutable_specification()->CopyFrom(
        functions_to_check_.at(function_to_check.source_name()));
    violation.set_violation_type(
        ViolationType::VIOLATION_TYPE_INSUFFICIENT_CHECK);
    violation.set_message("Insufficient check.");

    const Function &parent_function =
        LlvmToProtoFunction(*call_instruction.getParent()->getParent());
    violation.mutable_parent_function()->CopyFrom(parent_function);

    violations_.push_back(violation);
  }
}

GetViolationsResponse InsufficientChecksPass::GetViolations() const {
  GetViolationsResponse ret;
  for (const Violation &violation : violations_) {
    ret.add_violations()->CopyFrom(violation);
  }
  return ret;
}

// This is an analysis pass that does not transform, therefore it
// does not invalidate the results of any other passes.
void InsufficientChecksPass::getAnalysisUsage(llvm::AnalysisUsage &AU) const {
  AU.addRequired<ReturnConstraintsPass>();
  AU.addRequired<ReturnPropagationPass>();
  AU.setPreservesAll();
}

char InsufficientChecksPass::ID = 0;
static llvm::RegisterPass<InsufficientChecksPass> X(
    "insufficient-checks",
    "Find insufficiently checks calls to functions with error specifications",
    false, false);

};  // namespace error_specifications
