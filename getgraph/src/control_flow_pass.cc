#include "control_flow_pass.h"

#include <llvm/IR/CFG.h>
#include <llvm/IR/InstIterator.h>
#include <unistd.h>

#include "glog/logging.h"

namespace error_specifications {

static llvm::RegisterPass<ControlFlowPass> C("control-flow", "Build CFG", false,
                                             false);
static llvm::cl::opt<std::string> WriteDotOpt(
    "dot-epcfg", llvm::cl::desc("Write dot file for EP CFG"));
static llvm::cl::opt<std::string> DotStart(
    "dot-start", llvm::cl::desc("(Optional) function to start dot file at"));

bool ControlFlowPass::runOnModule(llvm::Module &M) {
  names = &getAnalysis<NamesPass>();

  llvm::Function *main = M.getFunction("main");
  FlowVertex main_v("main.0", main);
  FG.Add(main_v);

  if (main) {
    llvm::BasicBlock &entry = main->getEntryBlock();
    std::string entry_name;
    tie(entry_name, std::ignore) = names->GetBBNames(entry);
    FlowVertex entry_v(entry_name, main);
    FlowGraph::add_t added = FG.Add(main_v, entry_v);
    fn2vtx_[main] = std::get<0>(added);
  }

  for (llvm::Module::iterator f = M.begin(), e = M.end(); f != e; ++f) {
    if (f->isIntrinsic() || f->isDeclaration()) {
      continue;
    }

    FlowVertex fn_v(names->GetCallName(*f), &*f);
    if (!main) {
      FlowGraph::add_t added = FG.Add(main_v, fn_v);

      flow_edge_t edge_from_main = std::get<3>(added);
      FG.G[edge_from_main].main = true;

      flow_vertex_t fn_entry = std::get<1>(added);
      fn2vtx_[&*f] = fn_entry;
    }

    // Connect function to first basic block
    llvm::BasicBlock &entry = f->getEntryBlock();
    std::string bbe;
    tie(bbe, std::ignore) = names->GetBBNames(entry);
    FlowVertex entry_v(bbe, &*f);
    FG.Add(fn_v, entry_v);

    visitFunction(&*f);
  }

  for (llvm::Module::iterator f = M.begin(), e = M.end(); f != e; ++f) {
    AddMayReturnEdges(*f);
  }

  // This is not being utilized by the current GetGraph service. May be useful
  // at some point.
  if (!WriteDotOpt.empty()) {
    WriteDot(WriteDotOpt);
  }

  return false;
}

flow_vertex_t ControlFlowPass::GetFunctionVertex(
    const llvm::Function *F) const {
  return fn2vtx_.at(F);
}

void ControlFlowPass::WriteDot(const std::string path) {
  std::ofstream out(path);

  if (!DotStart.empty()) {
    std::string start_stack = DotStart + ".0";
    FG.WriteGraphviz(out, start_stack);
  } else {
    FG.WriteGraphviz(out);
  }

  out.close();
}

FlowVertex ControlFlowPass::CreatePredicateValueVertex(
    llvm::BranchInst *branch_inst, llvm::BasicBlock *succ_block) {
  // This function should never be called on an unconditional branching
  // instruction.
  assert(
      branch_inst->isConditional() &&
      "Cannot create predicate/value vertex, BranchInst is not conditional!");
  llvm::Value *condition = branch_inst->getCondition();

  // This function should also never be called on any conditional branching
  // instruction that does not utilize ICmpInst!
  llvm::ICmpInst *icmp = llvm::dyn_cast<llvm::ICmpInst>(condition);
  assert(icmp &&
         "Cannot create predicate/value vertex, condition is not from an "
         "ICmpInst!");

  // If getSuccessor(0) equals the supplied successor block, then the supplied
  // successor block is the true block.
  bool is_true_block = branch_inst->getSuccessor(0) == succ_block;

  // Get the predicate used as part of the ICmpInst.
  llvm::ICmpInst::Predicate predicate = icmp->getSignedPredicate();
  // If the instruction is in the successor FALSE block, swap the
  // ICmpInst::Predicate.
  if (!is_true_block) {
    predicate = llvm::ICmpInst::getSwappedPredicate(predicate);
  }

  // Getting the numeric label.
  std::string num_label;
  // Usually a constant int operand will be the second operand.
  llvm::ConstantInt *num =
      llvm::dyn_cast<llvm::ConstantInt>(icmp->getOperand(1));
  // If the number is not the second operand, try the first and swap the
  // predicate.
  if (!num) {
    num = llvm::dyn_cast<llvm::ConstantInt>(icmp->getOperand(0));

    // Only swap the predicate if the constant int was the first operand. We
    // only swap the predicate in this case because the constant int was
    // swapped.
    if (num) predicate = llvm::ICmpInst::getSwappedPredicate(predicate);
  }
  // If one of the operands was a number, get its sign.
  if (num) {
    if (num->isZero()) {
      num_label = "ZERO";
    } else if (num->isNegative()) {
      num_label = "NEG";
    } else {
      num_label = "POS";
    }
  } else {
    // If the comparison is not against a constant int, use a separate label.
    num_label = "NAC";
  }

  // We can now create the predicate label after all conditions have been
  // checked and predicates have been swapped appropriately.
  std::string predicate_label;
  switch (predicate) {
    case llvm::ICmpInst::Predicate::ICMP_EQ:
      predicate_label = "EQ";
      break;
    case llvm::ICmpInst::Predicate::ICMP_NE:
      predicate_label = "NE";
      break;
    case llvm::ICmpInst::Predicate::ICMP_SGT:
      predicate_label = "SGT";
      break;
    case llvm::ICmpInst::Predicate::ICMP_SGE:
      predicate_label = "SGE";
      break;
    case llvm::ICmpInst::Predicate::ICMP_SLT:
      predicate_label = "SLT";
      break;
    case llvm::ICmpInst::Predicate::ICMP_SLE:
      predicate_label = "SLE";
      break;
    default:
      predicate_label = "NAP";
  }

  // Combining the final label to include the predicate label and the numeric
  // label.
  FlowVertex predicate_value_v(names->GetBBNames(*succ_block).first + "pred_v",
                               succ_block->getParent());
  predicate_value_v.label_ids.push_back(
      FG.GetOrCreateId("F2V_CONDBR_" + predicate_label + "_" + num_label));
  return predicate_value_v;
}

void ControlFlowPass::visitFunction(llvm::Function *F) {
  for (auto bi = F->begin(), be = F->end(); bi != be; ++bi) {
    std::string bb_enter, bb_exit;
    tie(bb_enter, bb_exit) = names->GetBBNames(*bi);

    // Connect exit of each predecessor block to entry of this block
    FlowVertex bbe_v(bb_enter, F);
    for (auto pi = pred_begin(&*bi), pe = pred_end(&*be); pi != pe; ++pi) {
      llvm::BasicBlock *pred = *pi;
      std::string pred_exit;
      tie(std::ignore, pred_exit) = names->GetBBNames(*pred);
      FlowVertex predx_v(pred_exit, F);

      // We inject an extra vertex between conditional branching
      // instructions and their true/false successors. This is done to keep
      // track of the predicate/value that is used as a comparison for the
      // branching instruction.
      if (auto *branch_inst =
              llvm::dyn_cast<llvm::BranchInst>(&(pred->back()))) {
        if (branch_inst->isConditional() &&
            llvm::isa<llvm::ICmpInst>(branch_inst->getCondition())) {
          FlowVertex predicate_v =
              CreatePredicateValueVertex(branch_inst, &*bi);
          // Adding from branching instruction to predicate/value vertex.
          FG.Add(predx_v, predicate_v);
          // Adding from predicate/value vertex to successor block.
          FG.Add(predicate_v, bbe_v);
          continue;
        }
      }
      FG.Add(predx_v, bbe_v);
    }

    llvm::BasicBlock *bb = &*bi;
    FlowVertex prev = bbe_v;
    for (auto ii = bb->begin(), ie = bb->end(); ii != ie; ++ii) {
      llvm::Instruction *i = &*ii;
      prev = visitInstruction(i, prev);

      FlowGraph::add_t added;
      if (i == bb->getTerminator()) {
        FlowVertex bbx_v(bb_exit, F);
        added = FG.Add(prev, bbx_v);
      }

      // Populate fn2ret_
      if (llvm::isa<llvm::ReturnInst>(i)) {
        flow_vertex_t ret_vtx = std::get<1>(added);
        if (!ret_vtx) {
          LOG(WARNING) << "ControlFlowPass::visitInstruction received null "
                       << "return vertex\n";
          return;
        }
        fn2ret_[F] = ret_vtx;
      }
    }
  }
}

void ControlFlowPass::AddMayReturnEdges(llvm::Function &F) {
  if (F.isIntrinsic() || F.isDeclaration()) {
    return;
  }

  std::string stack = names->GetCallName(F);
  flow_vertex_t vtx = FG.GetVertex(stack);

  if (!vtx) {
    LOG(WARNING) << "ControlFlowPass::addMayReturnEdges unable to find vertex "
                 << "for stack.";
    return;
  }

  std::vector<flow_vertex_t> return_sites;
  flow_in_edge_iter iei, iei_end;
  for (tie(iei, iei_end) = in_edges(vtx, FG.G); iei != iei_end; ++iei) {
    FlowEdge e = FG.G[*iei];
    if (e.call) {
      flow_vertex_t call_site = source(*iei, FG.G);

      // We add an edge to the vertex immediately after call site
      flow_out_edge_iter oei, oei_end;
      for (tie(oei, oei_end) = out_edges(call_site, FG.G); oei != oei_end;
           ++oei) {
        if (FG.G[*oei].ret) {
          flow_vertex_t ret_to = target(*oei, FG.G);
          return_sites.push_back(ret_to);
          break;
        }
      }
    }
  }

  // Goto return inst for function
  auto ret_vtx_it = fn2ret_.find(&F);
  if (ret_vtx_it == fn2ret_.end()) {
    LOG(WARNING) << "ControlFlowPass::addMayReturnEdges unable to find "
                 << "return vertex\n";
    return;
  }
  flow_vertex_t ret_from = ret_vtx_it->second;

  for (flow_vertex_t ret_to : return_sites) {
    bool success;
    flow_edge_t may_ret;
    tie(may_ret, success) = boost::add_edge(ret_from, ret_to, FG.G);
    FG.G[may_ret].may_ret = true;
    if (!success) {
      LOG(WARNING) << "ContolFlowPass::addMayReturnEdges failed to add return "
                   << "edge";
      return;
    }
  }
}

FlowVertex ControlFlowPass::visitInstruction(llvm::Instruction *I,
                                             FlowVertex prev) {
  if (!I) {
    LOG(ERROR)
        << "FATAL ERROR: ControlFlowPass::visitInstruction called with null "
           "instruction\n";
    abort();
  }

  if (llvm::isa<llvm::IntrinsicInst>(I)) {
    return prev;
  }

  std::string iid = names->GetStackName(*I);

  FlowVertex i_v(iid, GetSource(I), I);
  // Add the vertex. Be careful of the insanity of FlowGraph properties being
  // overwritten if add is called on vertices that already exist.
  FlowGraph::add_t added = FG.Add(prev, i_v);

  flow_vertex_t from = std::get<0>(added);

  // CallInst are not terminators, so all calls are guaranteed to be visited as
  // prev
  if (prev.I && llvm::isa<llvm::CallInst>(prev.I)) {
    AddCalls(from, i_v);
  }

  return i_v;
}

void ControlFlowPass::AddCalls(flow_vertex_t call_desc, FlowVertex ret_v) {
  FlowVertex call_v = FG.G[call_desc];
  llvm::CallInst *call = llvm::dyn_cast<llvm::CallInst>(call_v.I);
  if (!call) abort();

  llvm::Value *cv = call->getCalledValue();
  if (!cv) return;

  llvm::Function *f = call->getCalledFunction();
  if (f && f->isDeclaration()) {
    std::string call_name = f->getName().str();
    FlowVertex callee_v(call_name, call->getParent()->getParent());
    FG.Add(call_v, callee_v, ret_v);
    return;
  }

  // Populate the memory index for indirect calls
  // This is for the word2vec project, but it might
  // be useful for something else.
  bool multi = false;
  mem_t load_index = names->GetLoadIndex(cv);
  if (load_index) {
    multi = true;
    call_v.mem_index = load_index;
  }

  vn_t callee_name = names->GetVarName(cv);
  if (!callee_name) return;

  mul_t callees;
  if (callee_name->type == VarType::MULTI) {
    if (!multi) {
      LOG(WARNING) << "Inconsistent MULTINAME when adding calls.";
      return;
    }
    callees = std::static_pointer_cast<MultiName>(callee_name);
  } else {
    callees = std::make_shared<MultiName>();
    callees->Insert(callee_name);
  }

  for (vn_t callee_vn : callees->Names()) {
    if (callee_vn->type != VarType::FUNCTION) {
      // See test102, could be function pointer declaration we don't have
      // Nothing we can do
      return;
    }

    fn_t callee_fn = std::static_pointer_cast<FunctionName>(callee_vn);
    llvm::Function *callee = callee_fn->function;

    // ==============================================
    // FIXME!: HACK put this somewhere, anywhere else.
    // If the mem_index is populated, then this came from the points-to
    // analysis.
    // FIXME: Use boost. Handle absolute vs. not absolute paths.
    bool add_edge = true;
    if (remove_cross_folder && call_v.mem_index) {
      std::string file1, file2;
      std::string component_f1, component_f2;

      llvm::Function *f = callee_fn->function;
      // Get location of the calling function
      assert(call_v.I);
      f = call_v.I->getParent()->getParent();
      // Refactor this loop into a function that can be used by definedfunctions
      // pass and here.
      for (llvm::inst_iterator I = inst_begin(f), E = inst_end(f); I != E;
           ++I) {
        if (llvm::DILocation *loc = I->getDebugLoc()) {
          file1 = loc->getFilename();
          if (file1.find('.') == 0) {
            file1 = file1.substr(2);
          }
          auto idx = file1.find('/');
          component_f1 = file1.substr(0, idx);
          break;
        }
      }

      // Get location of the called function
      f = callee_fn->function;
      for (llvm::inst_iterator I = inst_begin(f), E = inst_end(f); I != E;
           ++I) {
        if (llvm::DILocation *loc = I->getDebugLoc()) {
          file2 = loc->getFilename();
          if (file2.find('.') == 0) {
            file2 = file2.substr(2);
          }
          auto idx = file2.find('/');
          component_f2 = file2.substr(0, idx);
          break;
        }
      }

      if (component_f1 != component_f2 && component_f1 != "include" &&
          component_f2 != "include") {
        add_edge = false;
      }
    }
    // ==============================================

    std::string call_name = names->GetCallName(*callee);
    FlowVertex callee_v(call_name, call->getParent()->getParent());
    if (add_edge) {
      FG.Add(call_v, callee_v, ret_v);
    }
  }
}

void ControlFlowPass::getAnalysisUsage(llvm::AnalysisUsage &AU) const {
  AU.addRequired<NamesPass>();
  AU.setPreservesAll();
}

char ControlFlowPass::ID = 0;

}  //  namespace error_specifications
