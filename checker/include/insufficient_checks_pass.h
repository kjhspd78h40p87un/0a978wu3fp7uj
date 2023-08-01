// This pass finds insufficiently checked calls to functions.
// A call is insufficiently checked if there are not error handling
// blocks for the range of values. Because we do not know the exact
// constants that may be returned by a function, it is considered sufficient
// to check a single value for each lattice element <= the error specification.
// For example, checking -1 is considered sufficient for an error specification
// of less than zero.
//
// If a call is insufficiently checked, but the return value is returned
// from the parent function of the call, then it is not considered a bug.

#ifndef ERROR_SPECIFICATIONS_EESI_INCLUDE_INSUFFICIENT_CHECKS_PASS_H_
#define ERROR_SPECIFICATIONS_EESI_INCLUDE_INSUFFICIENT_CHECKS_PASS_H_

#include <string>

#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"
#include "llvm/Pass.h"
#include "tbb/tbb.h"

#include "proto/checker.pb.h"
#include "return_constraints_pass.h"

namespace error_specifications {

class InsufficientChecksPass : public llvm::ModulePass {
 public:
  static char ID;

  InsufficientChecksPass() : llvm::ModulePass(ID) {}

  // Entry point.
  bool runOnModule(llvm::Module &module) override;

  virtual void getAnalysisUsage(llvm::AnalysisUsage &AU) const override;

  // Used to pass in the specifications to check.
  void SetViolationsRequest(const GetViolationsRequest &request);

  // The list of violations found.
  GetViolationsResponse GetViolations() const;

 private:
  // The violations request constaining specifications to check.
  GetViolationsRequest violations_request_;

  // Map from function names to check to corresponding specification.
  tbb::concurrent_unordered_map<std::string, Specification> functions_to_check_;

  // Unchecked call site violations.
  tbb::concurrent_vector<Violation> violations_;

  // Queries ReturnConstraints about constraints on the return value
  // of fn_name in parent_function. Returns unique set of lattice elements that
  // are associated with any such constraint.
  std::set<SignLatticeElement> CollectConstraints(
      const llvm::Function &parent_function, const std::string &fn_name);

  // Visits every call instruction. Most of the logic for the checker is here.
  void VisitCallInst(const llvm::CallInst &call_instruction);

  // Returns true if the return value of call_instruction is returned by
  // the parent function of call_instruction (propagated).
  bool IsPropagated(const llvm::CallInst &call_instruction);

  // Returns true if the call instruction is sufficiently checked.
  bool IsSufficientlyChecked(const llvm::CallInst &call_instruction,
                             const Specification &specification);
};

}  // namespace error_specifications

#endif  // ERROR_SPECIFICATIONS_EESI_INCLUDE_INSUFFICIENT_CHECKS_PASS_H_
