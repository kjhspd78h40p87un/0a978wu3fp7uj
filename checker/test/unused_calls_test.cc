#include "gtest/gtest.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/Module.h"
#include "llvm/IRReader/IRReader.h"
#include "llvm/Support/SourceMgr.h"

#include "checker/include/unused_calls_pass.h"
#include "proto/checker.pb.h"

namespace error_specifications {

GetViolationsResponse RunUnusedCalls(const std::string &bitcode_path,
                                     const GetViolationsRequest &req) {
  UnusedCallsPass *unused_calls_pass = new UnusedCallsPass();
  unused_calls_pass->SetViolationsRequest(req);

  llvm::SMDiagnostic err;
  llvm::LLVMContext llvm_context;
  std::unique_ptr<llvm::Module> mod(
      llvm::parseIRFile(bitcode_path, err, llvm_context));
  if (!mod) {
    err.print("defined-functions-test", llvm::errs());
  }

  llvm::legacy::PassManager pass_manager;
  pass_manager.add(unused_calls_pass);
  pass_manager.run(*mod);

  return unused_calls_pass->GetViolations();
}

// A simple hello world program.
// The return value of printf is not used.
TEST(UnusedCallsTest, HelloFunctions) {
  GetViolationsRequest req;
  Specification *spec = req.add_specifications();
  spec->mutable_function()->set_source_name("printf");
  spec->set_lattice_element(
      SignLatticeElement::SIGN_LATTICE_ELEMENT_LESS_THAN_ZERO);

  GetViolationsResponse res = RunUnusedCalls("testdata/programs/hello.ll", req);

  ASSERT_EQ(res.violations_size(), 1);

  // This path is metadata in the LLVM bitcode file. It does not reflect
  // the layout of the source repository.
  EXPECT_EQ(res.violations(0).location().file(),
            "hello.c");
  EXPECT_EQ(res.violations(0).location().line(), 3);
  EXPECT_EQ(res.violations(0).violation_type(),
            ViolationType::VIOLATION_TYPE_UNUSED_RETURN_VALUE);
  EXPECT_EQ(res.violations(0).message(), "Unused return value.");
  EXPECT_EQ(res.violations(0).parent_function().source_name(), "main");
}

// A simple hello world program.
// The return value of printf is not used.
//
// Bitcode file uses a reg2mem pass optimization. This will demonstrate
// differences that can occur in debugging information that is provided
// by LLVM, as noticed with the line number.
TEST(UnusedCallsTest, HelloFunctionsReg2mem) {
  GetViolationsRequest req;
  Specification *spec = req.add_specifications();
  spec->mutable_function()->set_source_name("printf");
  spec->set_lattice_element(
      SignLatticeElement::SIGN_LATTICE_ELEMENT_LESS_THAN_ZERO);

  GetViolationsResponse res = RunUnusedCalls("testdata/programs/hello-reg2mem.ll", req);

  ASSERT_EQ(res.violations_size(), 1);

  // This path is metadata in the LLVM bitcode file. It does not reflect
  // the layout of the source repository.
  EXPECT_EQ(res.violations(0).location().file(),
            "/home/daniel/ucd/indra/ErrorSpecifications/test/programs/hello.c");
  EXPECT_EQ(res.violations(0).location().line(), 4);
  EXPECT_EQ(res.violations(0).violation_type(),
            ViolationType::VIOLATION_TYPE_UNUSED_RETURN_VALUE);
  EXPECT_EQ(res.violations(0).message(), "Unused return value.");
  EXPECT_EQ(res.violations(0).parent_function().source_name(), "main");
}

// A simple hello world program.
// Tests that an empty request leads to empty results.
TEST(UnusedCallsTest, EmptyRequest) {
  GetViolationsRequest req;
  GetViolationsResponse res = RunUnusedCalls("testdata/programs/hello.ll", req);

  ASSERT_EQ(res.violations_size(), 0);
}

// A simple hello world program.
// Tests that an empty request leads to empty results.
// Bitcode file uses a reg2mem pass optimization.
TEST(UnusedCallsTest, EmptyRequestReg2mem) {
  GetViolationsRequest req;
  GetViolationsResponse res = RunUnusedCalls("testdata/programs/hello-reg2mem.ll", req);

  ASSERT_EQ(res.violations_size(), 0);
}

// 2 of the 4 calls to foo are not used.
// Contains an if statement.
TEST(UnusedCallsTest, SavedReturn) {
  GetViolationsRequest req;
  Specification *spec = req.add_specifications();
  spec->mutable_function()->set_source_name("bar");
  spec->set_lattice_element(SignLatticeElement::SIGN_LATTICE_ELEMENT_TOP);
  GetViolationsResponse res =
      RunUnusedCalls("testdata/programs/saved_return.ll", req);

  // Behavior of checker is to discard specifications with TOP, as these
  // are unlikely to be useful.
  ASSERT_EQ(res.violations_size(), 0);
}

// 2 of the 4 calls to foo are not used.
// Contains an if statement.
// Bitcode file uses a reg2mem pass optimization.
TEST(UnusedCallsTest, SavedReturnReg2mem) {
  GetViolationsRequest req;
  Specification *spec = req.add_specifications();
  spec->mutable_function()->set_source_name("bar");
  spec->set_lattice_element(SignLatticeElement::SIGN_LATTICE_ELEMENT_TOP);
  GetViolationsResponse res =
      RunUnusedCalls("testdata/programs/saved_return-reg2mem.ll", req);

  // Behavior of checker is to discard specifications with TOP, as these
  // are unlikely to be useful.
  ASSERT_EQ(res.violations_size(), 0);
}

// Test that specifications with a value of bottom do not generate violations.
TEST(UnusedCallsTest, IgnoreBottom) {
  GetViolationsRequest req;
  Specification *spec = req.add_specifications();
  spec->mutable_function()->set_source_name("bar");
  spec->set_lattice_element(SignLatticeElement::SIGN_LATTICE_ELEMENT_BOTTOM);
  GetViolationsResponse res =
      RunUnusedCalls("testdata/programs/saved_return.ll", req);
  ASSERT_EQ(res.violations_size(), 0);
}

// Test that specifications with a value of bottom do not generate violations.
// Bitcode file uses a reg2mem pass optimization.
TEST(UnusedCallsTest, IgnoreBottomReg2mem) {
  GetViolationsRequest req;
  Specification *spec = req.add_specifications();
  spec->mutable_function()->set_source_name("bar");
  spec->set_lattice_element(SignLatticeElement::SIGN_LATTICE_ELEMENT_BOTTOM);
  GetViolationsResponse res =
      RunUnusedCalls("testdata/programs/saved_return-reg2mem.ll", req);
  ASSERT_EQ(res.violations_size(), 0);
}

}  // namespace error_specifications
