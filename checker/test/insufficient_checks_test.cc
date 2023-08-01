#include "gtest/gtest.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/Module.h"
#include "llvm/IRReader/IRReader.h"
#include "llvm/Support/SourceMgr.h"

#include "checker/include/insufficient_checks_pass.h"
#include "proto/checker.pb.h"

namespace error_specifications {

GetViolationsResponse RunInsufficientChecks(const std::string &bitcode_path,
                                            const GetViolationsRequest &req) {
  InsufficientChecksPass *insufficient_checks_pass =
      new InsufficientChecksPass();
  insufficient_checks_pass->SetViolationsRequest(req);

  llvm::SMDiagnostic err;
  llvm::LLVMContext llvm_context;
  std::unique_ptr<llvm::Module> mod(
      llvm::parseIRFile(bitcode_path, err, llvm_context));
  if (!mod) {
    err.print("defined-functions-test", llvm::errs());
  }

  llvm::legacy::PassManager pass_manager;
  pass_manager.add(insufficient_checks_pass);
  pass_manager.run(*mod);

  return insufficient_checks_pass->GetViolations();
}

TEST(InsufficientChecksTest, HelloFunctions) {
  GetViolationsRequest req;
  Specification *spec = req.add_specifications();
  spec->mutable_function()->set_source_name("printf");
  spec->set_lattice_element(
      SignLatticeElement::SIGN_LATTICE_ELEMENT_LESS_THAN_ZERO);

  GetViolationsResponse res =
      RunInsufficientChecks("testdata/programs/hello.ll", req);

  ASSERT_EQ(res.violations_size(), 1);

  // This path is metadata in the LLVM bitcode file. It does not reflect
  // the layout of the source repository.
  EXPECT_EQ(res.violations(0).location().file(),
            "hello.c");
  EXPECT_EQ(res.violations(0).location().line(), 3);
  EXPECT_EQ(res.violations(0).violation_type(),
            ViolationType::VIOLATION_TYPE_INSUFFICIENT_CHECK);
  EXPECT_EQ(res.violations(0).message(), "Insufficient check.");
  EXPECT_EQ(res.violations(0).parent_function().source_name(), "main");
}

TEST(InsufficientChecksTest, HelloFunctionsReg2mem) {
  GetViolationsRequest req;
  Specification *spec = req.add_specifications();
  spec->mutable_function()->set_source_name("printf");
  spec->set_lattice_element(
      SignLatticeElement::SIGN_LATTICE_ELEMENT_LESS_THAN_ZERO);

  GetViolationsResponse res =
      RunInsufficientChecks("testdata/programs/hello-reg2mem.ll", req);

  ASSERT_EQ(res.violations_size(), 1);

  // This path is metadata in the LLVM bitcode file. It does not reflect
  // the layout of the source repository.
  EXPECT_EQ(res.violations(0).location().file(),
            "/home/daniel/ucd/indra/ErrorSpecifications/test/programs/hello.c");
  EXPECT_EQ(res.violations(0).location().line(), 4);
  EXPECT_EQ(res.violations(0).violation_type(),
            ViolationType::VIOLATION_TYPE_INSUFFICIENT_CHECK);
  EXPECT_EQ(res.violations(0).message(), "Insufficient check.");
  EXPECT_EQ(res.violations(0).parent_function().source_name(), "main");
}


TEST(InsufficientChecksTest, OkLtzCheck) {
  GetViolationsRequest req;
  Specification *spec = req.add_specifications();
  spec->mutable_function()->set_source_name("mustcheck");
  spec->set_lattice_element(
      SignLatticeElement::SIGN_LATTICE_ELEMENT_LESS_THAN_ZERO);

  GetViolationsResponse res =
      RunInsufficientChecks("testdata/programs/ltz_check.ll", req);

  ASSERT_EQ(res.violations_size(), 0);
}

TEST(InsufficientChecksTest, OkLtzCheckReg2mem) {
  GetViolationsRequest req;
  Specification *spec = req.add_specifications();
  spec->mutable_function()->set_source_name("mustcheck");
  spec->set_lattice_element(
      SignLatticeElement::SIGN_LATTICE_ELEMENT_LESS_THAN_ZERO);

  GetViolationsResponse res =
      RunInsufficientChecks("testdata/programs/ltz_check-reg2mem.ll", req);

  ASSERT_EQ(res.violations_size(), 0);
}

TEST(InsufficientChecksTest, BuggyLtzCheck) {
  GetViolationsRequest req;
  Specification *spec = req.add_specifications();
  spec->mutable_function()->set_source_name("mustcheck");
  spec->set_lattice_element(
      SignLatticeElement::SIGN_LATTICE_ELEMENT_LESS_THAN_EQUAL_ZERO);

  GetViolationsResponse res =
      RunInsufficientChecks("testdata/programs/ltz_check.ll", req);

  ASSERT_EQ(res.violations_size(), 1);

  // This path is metadata in the LLVM bitcode file. It does not reflect
  // the layout of the source repository.
  EXPECT_EQ(res.violations(0).location().file(), "ltz_check.c");
  EXPECT_EQ(res.violations(0).location().line(), 4);
  EXPECT_EQ(res.violations(0).violation_type(),
            ViolationType::VIOLATION_TYPE_INSUFFICIENT_CHECK);
  EXPECT_EQ(res.violations(0).message(), "Insufficient check.");
}

TEST(InsufficientChecksTest, BuggyLtzCheckReg2mem) {
  GetViolationsRequest req;
  Specification *spec = req.add_specifications();
  spec->mutable_function()->set_source_name("mustcheck");
  spec->set_lattice_element(
      SignLatticeElement::SIGN_LATTICE_ELEMENT_LESS_THAN_EQUAL_ZERO);

  GetViolationsResponse res =
      RunInsufficientChecks("testdata/programs/ltz_check-reg2mem.ll", req);

  ASSERT_EQ(res.violations_size(), 1);

  // This path is metadata in the LLVM bitcode file. It does not reflect
  // the layout of the source repository.
  EXPECT_EQ(res.violations(0).location().file(), "ltz_check.c");
  EXPECT_EQ(res.violations(0).location().line(), 4);
  EXPECT_EQ(res.violations(0).violation_type(),
            ViolationType::VIOLATION_TYPE_INSUFFICIENT_CHECK);
  EXPECT_EQ(res.violations(0).message(), "Insufficient check.");
}

TEST(InsufficientChecksTest, OkNtzCheck) {
  GetViolationsRequest req;
  Specification *spec = req.add_specifications();
  spec->mutable_function()->set_source_name("mustcheck");
  spec->set_lattice_element(
      SignLatticeElement::SIGN_LATTICE_ELEMENT_LESS_THAN_ZERO);

  GetViolationsResponse res =
      RunInsufficientChecks("testdata/programs/ntz_check.ll", req);

  ASSERT_EQ(res.violations_size(), 0);
}

TEST(InsufficientChecksTest, OkNtzCheckReg2mem) {
  GetViolationsRequest req;
  Specification *spec = req.add_specifications();
  spec->mutable_function()->set_source_name("mustcheck");
  spec->set_lattice_element(
      SignLatticeElement::SIGN_LATTICE_ELEMENT_LESS_THAN_ZERO);

  GetViolationsResponse res =
      RunInsufficientChecks("testdata/programs/ntz_check-reg2mem.ll", req);

  ASSERT_EQ(res.violations_size(), 0);
}

TEST(InsufficientChecksTest, BuggyNtzCheck) {
  GetViolationsRequest req;
  Specification *spec = req.add_specifications();
  spec->mutable_function()->set_source_name("mustcheck");
  spec->set_lattice_element(
      SignLatticeElement::SIGN_LATTICE_ELEMENT_LESS_THAN_EQUAL_ZERO);

  GetViolationsResponse res =
      RunInsufficientChecks("testdata/programs/ntz_check.ll", req);

  ASSERT_EQ(res.violations_size(), 1);

  EXPECT_EQ(res.violations(0).location().file(), "ntz_check.c");
  EXPECT_EQ(res.violations(0).location().line(), 4);
  EXPECT_EQ(res.violations(0).violation_type(),
            ViolationType::VIOLATION_TYPE_INSUFFICIENT_CHECK);
  EXPECT_EQ(res.violations(0).message(), "Insufficient check.");
}

TEST(InsufficientChecksTest, BuggyNtzCheckReg2mem) {
  GetViolationsRequest req;
  Specification *spec = req.add_specifications();
  spec->mutable_function()->set_source_name("mustcheck");
  spec->set_lattice_element(
      SignLatticeElement::SIGN_LATTICE_ELEMENT_LESS_THAN_EQUAL_ZERO);

  GetViolationsResponse res =
      RunInsufficientChecks("testdata/programs/ntz_check-reg2mem.ll", req);

  ASSERT_EQ(res.violations_size(), 1);

  EXPECT_EQ(res.violations(0).location().file(), "ntz_check.c");
  EXPECT_EQ(res.violations(0).location().line(), 4);
  EXPECT_EQ(res.violations(0).violation_type(),
            ViolationType::VIOLATION_TYPE_INSUFFICIENT_CHECK);
  EXPECT_EQ(res.violations(0).message(), "Insufficient check.");
}

TEST(InsufficientChecksTest, OkEqLtzCheck) {
  GetViolationsRequest req;
  Specification *spec = req.add_specifications();
  spec->mutable_function()->set_source_name("mustcheck");
  spec->set_lattice_element(
      SignLatticeElement::SIGN_LATTICE_ELEMENT_LESS_THAN_ZERO);

  GetViolationsResponse res =
      RunInsufficientChecks("testdata/programs/eq_ltz_check.ll", req);

  ASSERT_EQ(res.violations_size(), 0);
}

TEST(InsufficientChecksTest, OkEqLtzCheckReg2mem) {
  GetViolationsRequest req;
  Specification *spec = req.add_specifications();
  spec->mutable_function()->set_source_name("mustcheck");
  spec->set_lattice_element(
      SignLatticeElement::SIGN_LATTICE_ELEMENT_LESS_THAN_ZERO);

  GetViolationsResponse res =
      RunInsufficientChecks("testdata/programs/eq_ltz_check-reg2mem.ll", req);

  ASSERT_EQ(res.violations_size(), 0);
}

TEST(InsufficientChecksTest, BuggyEqLtzCheck) {
  GetViolationsRequest req;
  Specification *spec = req.add_specifications();
  spec->mutable_function()->set_source_name("mustcheck");
  spec->set_lattice_element(SignLatticeElement::SIGN_LATTICE_ELEMENT_NOT_ZERO);

  GetViolationsResponse res =
      RunInsufficientChecks("testdata/programs/eq_ltz_check.ll", req);

  ASSERT_EQ(res.violations_size(), 1);
  EXPECT_EQ(res.violations(0).location().file(), "eq_ltz_check.c");
  EXPECT_EQ(res.violations(0).location().line(), 4);
  EXPECT_EQ(res.violations(0).violation_type(),
            ViolationType::VIOLATION_TYPE_INSUFFICIENT_CHECK);
  EXPECT_EQ(res.violations(0).message(), "Insufficient check.");
}

TEST(InsufficientChecksTest, BuggyEqLtzCheckReg2mem) {
  GetViolationsRequest req;
  Specification *spec = req.add_specifications();
  spec->mutable_function()->set_source_name("mustcheck");
  spec->set_lattice_element(SignLatticeElement::SIGN_LATTICE_ELEMENT_NOT_ZERO);

  GetViolationsResponse res =
      RunInsufficientChecks("testdata/programs/eq_ltz_check-reg2mem.ll", req);

  ASSERT_EQ(res.violations_size(), 1);
  EXPECT_EQ(res.violations(0).location().file(), "eq_ltz_check.c");
  EXPECT_EQ(res.violations(0).location().line(), 4);
  EXPECT_EQ(res.violations(0).violation_type(),
            ViolationType::VIOLATION_TYPE_INSUFFICIENT_CHECK);
  EXPECT_EQ(res.violations(0).message(), "Insufficient check.");
}

TEST(InsufficientChecksTest, OkSplitEqZeroEqLtzCheck) {
  GetViolationsRequest req;
  Specification *spec = req.add_specifications();
  spec->mutable_function()->set_source_name("mustcheck");
  spec->set_lattice_element(
      SignLatticeElement::SIGN_LATTICE_ELEMENT_LESS_THAN_EQUAL_ZERO);

  GetViolationsResponse res = RunInsufficientChecks(
      "testdata/programs/check_eqzero_eqnegative.ll", req);

  ASSERT_EQ(res.violations_size(), 0);
}

TEST(InsufficientChecksTest, OkSplitEqZeroEqLtzCheckReg2mem) {
  GetViolationsRequest req;
  Specification *spec = req.add_specifications();
  spec->mutable_function()->set_source_name("mustcheck");
  spec->set_lattice_element(
      SignLatticeElement::SIGN_LATTICE_ELEMENT_LESS_THAN_EQUAL_ZERO);

  GetViolationsResponse res = RunInsufficientChecks(
      "testdata/programs/check_eqzero_eqnegative-reg2mem.ll", req);

  ASSERT_EQ(res.violations_size(), 0);
}

TEST(InsufficientChecksTest, BuggySplitEqZeroEqLtzCheck) {
  GetViolationsRequest req;
  Specification *spec = req.add_specifications();
  spec->mutable_function()->set_source_name("mustcheck");
  spec->set_lattice_element(
      SignLatticeElement::SIGN_LATTICE_ELEMENT_GREATER_THAN_EQUAL_ZERO);

  GetViolationsResponse res = RunInsufficientChecks(
      "testdata/programs/check_eqzero_eqnegative.ll", req);

  ASSERT_EQ(res.violations_size(), 1);
  EXPECT_EQ(res.violations(0).location().file(), "check_eqzero_eqnegative.c");
  EXPECT_EQ(res.violations(0).location().line(), 4);
  EXPECT_EQ(res.violations(0).violation_type(),
            ViolationType::VIOLATION_TYPE_INSUFFICIENT_CHECK);
  EXPECT_EQ(res.violations(0).message(), "Insufficient check.");
}

TEST(InsufficientChecksTest, BuggySplitEqZeroEqLtzCheckReg2mem) {
  GetViolationsRequest req;
  Specification *spec = req.add_specifications();
  spec->mutable_function()->set_source_name("mustcheck");
  spec->set_lattice_element(
      SignLatticeElement::SIGN_LATTICE_ELEMENT_GREATER_THAN_EQUAL_ZERO);

  GetViolationsResponse res = RunInsufficientChecks(
      "testdata/programs/check_eqzero_eqnegative-reg2mem.ll", req);

  ASSERT_EQ(res.violations_size(), 1);
  EXPECT_EQ(res.violations(0).location().file(), "check_eqzero_eqnegative.c");
  EXPECT_EQ(res.violations(0).location().line(), 4);
  EXPECT_EQ(res.violations(0).violation_type(),
            ViolationType::VIOLATION_TYPE_INSUFFICIENT_CHECK);
  EXPECT_EQ(res.violations(0).message(), "Insufficient check.");
}

TEST(InsufficientChecksTest, DirectPropagation) {
  GetViolationsRequest req;
  Specification *spec = req.add_specifications();
  spec->mutable_function()->set_source_name("bar");
  spec->set_lattice_element(
      SignLatticeElement::SIGN_LATTICE_ELEMENT_GREATER_THAN_EQUAL_ZERO);

  GetViolationsResponse res =
      RunInsufficientChecks("testdata/programs/propagation_direct.ll", req);

  ASSERT_EQ(res.violations_size(), 0);
}

TEST(InsufficientChecksTest, DirectPropagationReg2mem) {
  GetViolationsRequest req;
  Specification *spec = req.add_specifications();
  spec->mutable_function()->set_source_name("bar");
  spec->set_lattice_element(
      SignLatticeElement::SIGN_LATTICE_ELEMENT_GREATER_THAN_EQUAL_ZERO);

  GetViolationsResponse res =
      RunInsufficientChecks("testdata/programs/propagation_direct-reg2mem.ll", req);

  ASSERT_EQ(res.violations_size(), 0);
}

TEST(InsufficientChecksTest, PropagationInsideIf) {
  GetViolationsRequest req;
  Specification *spec = req.add_specifications();
  spec->mutable_function()->set_source_name("bar");
  spec->set_lattice_element(SignLatticeElement::SIGN_LATTICE_ELEMENT_ZERO);

  GetViolationsResponse res =
      RunInsufficientChecks("testdata/programs/propagation_inside_if.ll", req);

  ASSERT_EQ(res.violations_size(), 0);
}

TEST(InsufficientChecksTest, PropagationInsideIfReg2mem) {
  GetViolationsRequest req;
  Specification *spec = req.add_specifications();
  spec->mutable_function()->set_source_name("bar");
  spec->set_lattice_element(SignLatticeElement::SIGN_LATTICE_ELEMENT_ZERO);

  GetViolationsResponse res =
      RunInsufficientChecks("testdata/programs/propagation_inside_if-reg2mem.ll", req);

  ASSERT_EQ(res.violations_size(), 0);
}

// Add struct field test that fails (expected behavior)

// Add passed to parameter test that fails (expected behavior)

}  // namespace error_specifications
