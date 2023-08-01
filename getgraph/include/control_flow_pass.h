#ifndef ERROR_SPECIFICATIONS_GET_GRAPH_INCLUDE_CONTROL_FLOW_PASS_H_
#define ERROR_SPECIFICATIONS_GET_GRAPH_INCLUDE_CONTROL_FLOW_PASS_H_

#include <unordered_map>
#include "llvm/IR/Module.h"
#include "llvm/Pass.h"

#include "flow_graph.h"
#include "location.h"
#include "names_pass.h"

namespace error_specifications {

class ControlFlowPass : public llvm::ModulePass {
 public:
  ControlFlowPass() : ModulePass(ID) {}

  static char ID;

  virtual bool runOnModule(llvm::Module &M) override;
  // Visits the LLVM function and builds the FlowGraph by iterating over basic
  // blocks.
  void visitFunction(llvm::Function *f);
  FlowVertex visitInstruction(llvm::Instruction *I, FlowVertex prev);
  // Creates a FlowVertex with a label representing the predicate and value
  // used for a conditional branch that checks a condition from an ICmpInst.
  // This predicate also depends on whether the edge is to the true or false
  // successor.
  FlowVertex CreatePredicateValueVertex(llvm::BranchInst *branch_inst,
                                        llvm::BasicBlock *succ_block);
  // Add may return edges to the FlowGraph.
  void AddMayReturnEdges(llvm::Function &F);
  // Add calls to the FlowGraph.
  void AddCalls(flow_vertex_t call, FlowVertex ret_v);

  FlowGraph GetFlowGraph() { return FG; }

  void getAnalysisUsage(llvm::AnalysisUsage &AU) const override;

  // The result of running the NamesPass on the bitcode file.
  NamesPass *names;
  // The FlowGraph representing the entire program.
  FlowGraph FG;

  // Get or generate an id for this value
  void AddPredecessorRules(llvm::Instruction *);

  void WriteDot(const std::string path);

  // Returns the function vertex related to the LLVM function.
  flow_vertex_t GetFunctionVertex(const llvm::Function *F) const;

  // Currently part of a hack. Is explained further in the actual
  // implementation.
  bool remove_cross_folder = false;

 private:
  unsigned id_cnt_ = 1;

  // Map from function to the vertex holding the return instruction
  // This is populated in runOnFunction and consumed by addMayReturnEdges
  std::map<llvm::Function *, flow_vertex_t> fn2ret_;

  // Map from function to its entry vertex descriptor
  // Populated in runOnModule
  std::map<const llvm::Function *, flow_vertex_t> fn2vtx_;
};

}  // namespace error_specifications

#endif
