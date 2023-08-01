#ifndef ERROR_SPECIFICATIONS_CHECKER_INCLUDE_UNUSED_CALLS_PASS_H_
#define ERROR_SPECIFICATIONS_CHECKER_INCLUDE_UNUSED_CALLS_PASS_H_

#include <string>

#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"
#include "llvm/Pass.h"
#include "tbb/tbb.h"

#include "proto/checker.pb.h"

namespace error_specifications {

// This LLVM pass is responsible for finding calls to functions where
// the return value is never used.
class UnusedCallsPass : public llvm::ModulePass {
 public:
  static char ID;

  UnusedCallsPass() : llvm::ModulePass(ID) {}

  // Entry point.
  bool runOnModule(llvm::Module &M) override;

  virtual void getAnalysisUsage(llvm::AnalysisUsage &AU) const override;

  // Used to pass in the specifications to check. Only the name of the
  // function for each specification is used, not the lattice value.
  void SetViolationsRequest(const GetViolationsRequest &request);

  // The list of violations found.
  GetViolationsResponse GetViolations() const;

 private:
  // The violations request containing specifications to check.
  GetViolationsRequest violations_request_;

  // Map from function names to check to corresponding specification.
  // Functions can have two names. This pass asserts that the source name
  // and llvm name match for all specification functions.
  tbb::concurrent_unordered_map<std::string, Specification> functions_to_check_;

  // Unchecked call site violations.
  tbb::concurrent_vector<Violation> unused_calls_;

  void visitCallInst(const llvm::CallInst &call_instruction);
};

}  // namespace error_specifications

#endif  // ERROR_SPECIFICATIONS_CHECKER_INCLUDE_UNUSED_CALLS_PASS_H_
