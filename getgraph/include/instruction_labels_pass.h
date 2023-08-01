#ifndef ERROR_SPECIFICATIONS_GET_GRAPH_INCLUDE_INSTRUCTION_LABELS_PASS_H_
#define ERROR_SPECIFICATIONS_GET_GRAPH_INCLUDE_INSTRUCTION_LABELS_PASS_H_

#include "control_flow_pass.h"
#include "eesi/include/return_propagation_pass.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/InstVisitor.h"
#include "llvm/Pass.h"
#include "names_pass.h"

namespace error_specifications {

struct LabelVisitor : llvm::InstVisitor<LabelVisitor> {
  LabelVisitor(NamesPass *names, FlowGraph &FG, ReturnPropagationPass *rpp)
      : names(names), FG(FG), rpp(rpp) {}

  void visitInstruction(llvm::Instruction &I);

  void visitStoreInst(llvm::StoreInst &I);

  void visitLoadInst(llvm::LoadInst &I);

  void visitReturnInst(llvm::ReturnInst &I);

  void visitGetElementPtrInst(llvm::GetElementPtrInst &I);

  // function that that takes string -> id
  // generate id if string does not exist
  int GetOrCreateId(const std::string &label);

  std::string ResolveIndirectName(llvm::Instruction *I,
                                  llvm::Value *indirect_value);
  const llvm::Value *SelectRandom(std::unordered_set<const llvm::Value *> s);
  NamesPass *names;
  FlowGraph &FG;
  ReturnPropagationPass *rpp;
};

class InstructionLabelsPass : public llvm::ModulePass {
 public:
  static char ID;

  InstructionLabelsPass() : llvm::ModulePass(ID) {}

  virtual bool runOnModule(llvm::Module &M) override;

  std::unordered_map<int, std::string> GetIdToLabel();

  void getAnalysisUsage(llvm::AnalysisUsage &AU) const override;

  std::unordered_map<std::string, int> label_to_id;
};

}  // namespace error_specifications

#endif  // ERROR_SPECIFICATIONS_GET_GRAPH_INCLUDE_INSTRUCTION_LABELS_H_
