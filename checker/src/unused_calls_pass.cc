#include "unused_calls_pass.h"

#include <string>

#include "checker_common.h"
#include "glog/logging.h"
#include "llvm.h"
#include "llvm/IR/Function.h"
#include "llvm/Pass.h"
#include "proto/checker.pb.h"
#include "tbb/tbb.h"

namespace error_specifications {

bool UnusedCallsPass::runOnModule(llvm::Module &module) {
  LOG(INFO) << "Running unused calls pass on module...";

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
                this->visitCallInst(*call);
              }
            }
          }
        }
      });

  LOG(INFO) << "Unused calls pass finished.";

  return false;
}

void UnusedCallsPass::SetViolationsRequest(
    const GetViolationsRequest &request) {
  violations_request_.CopyFrom(request);

  for (const Specification &spec : violations_request_.specifications()) {
    functions_to_check_[spec.function().source_name()] = spec;
  }
}

void UnusedCallsPass::visitCallInst(const llvm::CallInst &call_instruction) {
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

  // Only emit a bug report if the return value is not used at all.
  if (call_instruction.use_empty()) {
    Violation violation;
    violation.mutable_location()->CopyFrom(GetDebugLocation(call_instruction));
    violation.mutable_specification()->CopyFrom(specification);
    violation.set_violation_type(
        ViolationType::VIOLATION_TYPE_UNUSED_RETURN_VALUE);
    violation.set_message("Unused return value.");

    const Function &parent_function =
        LlvmToProtoFunction(*call_instruction.getParent()->getParent());
    violation.mutable_parent_function()->CopyFrom(parent_function);

    unused_calls_.push_back(violation);
  }
}

GetViolationsResponse UnusedCallsPass::GetViolations() const {
  GetViolationsResponse ret;
  for (const Violation &violation : unused_calls_) {
    ret.add_violations()->CopyFrom(violation);
  }
  return ret;
}

// This is an analysis pass that does not transform, therefore it
// does not invalidate the results of any other passes.
void UnusedCallsPass::getAnalysisUsage(llvm::AnalysisUsage &AU) const {
  AU.setPreservesAll();
}

char UnusedCallsPass::ID = 0;
static llvm::RegisterPass<UnusedCallsPass> X(
    "unused-calls",
    "Find unchecked calls to functions with error specifications", false,
    false);

}  // namespace error_specifications
