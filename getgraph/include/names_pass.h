// This pass is responsible for keeping a map of Values to names
// These names are what we want to use in the WPDS file
// GetVarName is the function most commonly used by clients

// Local Variables
// ===============
// Local variables have the following forms:
// foo#x: Local variable x in function foo.
//        These are generated from llvm.dbg.declare calls.
//
// There is also a map calle locals indexed by pointers to function values
// Each element is the set of names local to that function
// These are drawn from the function arguments and alloca instructions

// Global variables
// ================
// There are three ways that a global variable can come about
// 1. Global variables that actually exist in the program.
//    These are collected in NamesPass::runOnModule
// 2. Variables named foo$return to hold the return value from function foo
//    These are generated when addGlobalReturn is called
//    The names map also holds a mapping from the CallInst value to this name
// 3. Variables named foo$arg which hold the value of arg passed to function foo
//    These are collect in NamesPass::runOnModule

// Function Pointers
// =================
// Function pointers are handled *if* the pointer is stored in a memory location
// that we handle (pretty much just structs at the moment).
// Function pointers stored in any other variable will probably only get a
// single possible function name, and function pointers as arguments are not
// handled at all.

#ifndef ERROR_SPECIFICATIONS_GET_GRAPH_INCLUDE_NAMES_PASS_H_
#define ERROR_SPECIFICATIONS_GET_GRAPH_INCLUDE_NAMES_PASS_H_

#include <queue>
#include <set>
#include <unordered_set>
#include "llvm/IR/Function.h"
#include "llvm/IR/InstVisitor.h"
#include "llvm/Pass.h"
#include "llvm/Support/raw_ostream.h"

#include "proto/domain_knowledge.pb.h"
#include "var_name.h"

namespace error_specifications {

class NamesPass : public llvm::ModulePass, public llvm::InstVisitor<NamesPass> {
  // Functions are virtual to allow mocking

 public:
  static char ID;
  NamesPass() : llvm::ModulePass(ID) {}
  NamesPass(std::string ecfile) : llvm::ModulePass(ID), ecpath_arg_(ecfile) {}

  ~NamesPass() {
    for (auto *inst : to_cleanup_) {
      inst->deleteValue();
    }
  }

  void getAnalysisUsage(llvm::AnalysisUsage &AU) const override;

  // Sets the error codes that will be used by the NamesPass.
  void SetErrorCodes(std::vector<ErrorCode> error_codes);

  // Get a pointer to the VarName associated with an LLVM Value
  // Setting allow_ec to false will return nullptr for error codes
  virtual vn_t GetVarName(const llvm::Value *V);

  // Get the stack name to use for an instruction.
  std::string GetStackName(llvm::Instruction &I);

  // Get the name to use for a basic block
  std::pair<std::string, std::string> GetBBNames(llvm::BasicBlock &BB);

  // Get the name to use for a call to a function
  std::string GetCallName(llvm::Function &F);

  // Get the names of all of the functions called
  std::vector<std::string> GetCalleeNames(const llvm::CallInst &CI);

  // Get the next dummy name, for inserting extra rules
  std::string GetDummyName(llvm::Instruction *I);

  // Get the next tmp name, for unsaved returns
  // This will cause the tmp name to be added into locals (otherwise invalid)
  VarName GetUnsavedName(llvm::CallInst &I);

  // Get the name to use for formal argument
  // Also creates global exchange var
  std::string GetFormalArgName(llvm::Argument &A);

  // Get local names for a specific function
  std::set<llvm::Value *> GetLocalValues(llvm::Function &F);

  bool runOnModule(llvm::Module &M) override;
  void visitLoadInst(llvm::LoadInst &I);
  void visitAllocaInst(llvm::AllocaInst &I);
  void visitCallInst(llvm::CallInst &I);
  void visitStoreInst(llvm::StoreInst &I);
  void visitBinaryOperator(llvm::BinaryOperator &I);
  void visitPtrToIntInst(llvm::PtrToIntInst &I);
  void visitIntToPtrInst(llvm::IntToPtrInst &I);
  void visitPHINode(llvm::PHINode &I);
  void visitSelectInst(llvm::SelectInst &I);
  void visitGetElementPtrInst(llvm::GetElementPtrInst &I);

  // Should we track this value?
  // returns true for yes, false for no
  VarType ResolveType(llvm::Value *V);
  bool Filter(llvm::Type *T);

  std::map<int, vn_t> GetErrorNames();

  // For func2vec.
  std::map<std::string, std::set<std::string>> GetBootstrapFunctions();

  // Get an approximate name for GEP based on struct type
  mem_t GetApproxName(llvm::GetElementPtrInst &I);

  mem_t GetLoadIndex(const llvm::Value *v) const;

 private:
  // Core name maps
  std::map<const llvm::Value *, vn_t> names_;
  std::map<int, vn_t> error_names_;

  // The memory model is indexed by MemoryNames (GEP indexes)
  std::map<VarName, vn_t> memory_model_;

  // Load -> memory model index
  std::map<const llvm::Value *, mem_t> load_index_;

  std::map<llvm::Function *, VarName> return_names_;

  // Values that we know are not ECs in a block
  std::map<llvm::BasicBlock *, std::set<llvm::Value *>> safe_values_;

  std::map<llvm::Function *, std::set<llvm::Value *>> locals_;

  vn_t EC_OK_ = std::make_shared<ErrorName>("OK");

  std::map<llvm::Instruction *, std::string> stack_iids_;
  unsigned stack_cnt_ = 1;

  unsigned dummy_cnt_ = 1;
  unsigned intermediate_cnt_ = 1;

  std::string GetIID(llvm::Value *V);
  std::string GenerateIntermediateName();

  llvm::Module *module_;

  // For converting a Value to a constant struct
  // Used to Get global struct literals
  // Returns null if the conversion fails
  llvm::ConstantStruct *GetStruct(llvm::Value *V);

  // Creates a function MultiName in memory_model
  void BackFunction(MemoryName index, llvm::Function *f);

  void UpdateMemory(MemoryName index, vn_t name);

  void SetupFunction(llvm::Function *f);

  std::queue<llvm::Function *> worklist_;

  // Explicity set by mining rather than on command line
  std::string ecpath_arg_;

  std::string GetApproxName(llvm::StructType *t);

  // Beacuse of use of GetAsInstruction()
  std::vector<llvm::Instruction *> to_cleanup_;
};

}  // namespace error_specifications

#endif
