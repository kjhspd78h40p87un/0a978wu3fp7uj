#include "instruction_labels_pass.h"

#include <algorithm>
#include <iostream>
#include <random>

#include "common/include/llvm.h"
#include "eesi/include/return_propagation_pass.h"
#include "glog/logging.h"
#include "names_pass.h"

namespace error_specifications {

bool InstructionLabelsPass::runOnModule(llvm::Module &M) {
  LabelVisitor LV(&getAnalysis<NamesPass>(),
                  (&getAnalysis<ControlFlowPass>())->FG,
                  &getAnalysis<ReturnPropagationPass>());
  LV.visit(M);

  label_to_id = LV.FG.label_to_id;

  return false;
}

void LabelVisitor::visitInstruction(llvm::Instruction &I) {
  if (llvm::isa<llvm::CallInst>(I)) {
    return;
  }

  flow_vertex_t vertex = FG.stack_vertex_map[names->GetStackName(I)];
  if (!vertex) return;
  std::string label = "F2V_INST_";
  label += I.getOpcodeName();
  label += I.getParent()->getParent()->getName();

  FG.G[vertex].label_ids.push_back(FG.GetOrCreateId(label));
}

void LabelVisitor::visitStoreInst(llvm::StoreInst &I) {
  if (I.getNumOperands() == 0) {
    return;
  }

  std::string parent_fname = I.getParent()->getParent()->getName();
  llvm::Value *sender = I.getOperand(0)->stripPointerCasts();
  vn_t sender_name = names->GetVarName(sender);
  flow_vertex_t vertex = FG.stack_vertex_map[names->GetStackName(I)];
  if (sender_name && sender_name->type == VarType::EC &&
      sender_name->Name() != "OK") {
    FG.G[vertex].label_ids.push_back(FG.GetOrCreateId(
        "F2V_STORE_ERR_" + sender_name->Name() + "_" + parent_fname));
  } else {
    std::string stored_int_str = "";
    if (const llvm::ConstantInt *stored_int =
            llvm::dyn_cast<llvm::ConstantInt>(sender)) {
      if (stored_int->getBitWidth() <= 64) {
        int64_t stored_int_value = stored_int->getSExtValue();
        stored_int_str = "_DIR_" + std::to_string(stored_int_value);
      }
    } else {
      stored_int_str = ResolveIndirectName(&I, sender);
    }

    if (stored_int_str == "") {
      return;
    }

    FG.G[vertex].label_ids.push_back(FG.GetOrCreateId(
        "F2V_INST_store" + stored_int_str + "_" + parent_fname));
  }
}

void LabelVisitor::visitLoadInst(llvm::LoadInst &I) {
  if (I.getNumOperands() == 0) {
    return;
  }

  std::string parent_fname = I.getParent()->getParent()->getName();
  llvm::Value *load_from = I.getOperand(0)->stripPointerCasts();
  vn_t load_from_name = names->GetVarName(load_from);
  flow_vertex_t vertex = FG.stack_vertex_map[names->GetStackName(I)];
  if (load_from_name && load_from_name->type == VarType::EC &&
      load_from_name->Name() != "OK") {
    FG.G[vertex].label_ids.push_back(FG.GetOrCreateId(
        "F2V_LOAD_ERR_" + load_from_name->Name() + "_" + parent_fname));
  } else {
    return;
    std::string stored_int_str = "";
    if (const llvm::ConstantInt *stored_int =
            llvm::dyn_cast<llvm::ConstantInt>(load_from)) {
      if (stored_int->getBitWidth() <= 64) {
        int64_t stored_int_value = stored_int->getSExtValue();
        stored_int_str = "_DIR_" + std::to_string(stored_int_value);
      }
    } else {
      stored_int_str = ResolveIndirectName(&I, load_from);
    }

    if (stored_int_str == "") {
      return;
    }

    FG.G[vertex].label_ids.push_back(FG.GetOrCreateId(
        "F2V_INST_load" + stored_int_str + "_" + parent_fname));
  }
}

void LabelVisitor::visitReturnInst(llvm::ReturnInst &I) {
  if (I.getNumOperands() == 0) {
    return;
  }

  std::string parent_fname = I.getParent()->getParent()->getName();
  llvm::Value *ret = I.getOperand(0)->stripPointerCasts();
  vn_t ret_name = names->GetVarName(ret);
  flow_vertex_t vertex = FG.stack_vertex_map[names->GetStackName(I)];
  if (ret_name && ret_name->type == VarType::EC && ret_name->Name() != "OK") {
    FG.G[vertex].label_ids.push_back(
        FG.GetOrCreateId("F2V_RET_" + ret_name->Name() + "_" + parent_fname));
  } else {
    std::string stored_int_str = "";
    if (const llvm::ConstantInt *stored_int =
            llvm::dyn_cast<llvm::ConstantInt>(ret)) {
      if (stored_int->getBitWidth() <= 64) {
        int64_t stored_int_value = stored_int->getSExtValue();
        stored_int_str = "_DIR_" + std::to_string(stored_int_value);
      }
    } else {
      stored_int_str = ResolveIndirectName(&I, ret);
    }

    if (stored_int_str == "") {
      return;
    }

    FG.G[vertex].label_ids.push_back(
        FG.GetOrCreateId("F2V_RET_" + stored_int_str + "_" + parent_fname));
  }
}

void LabelVisitor::visitGetElementPtrInst(llvm::GetElementPtrInst &I) {
  flow_vertex_t vertex = FG.stack_vertex_map[names->GetStackName(I)];
  std::string parent_fname = I.getParent()->getParent()->getName();
  if (I.getNumOperands() < 3) {
    FG.G[vertex].label_ids.push_back(
        FG.GetOrCreateId("F2V_INST_getelementptr_" + parent_fname));
    return;
  }
  vn_t approx_name = names->GetApproxName(I);
  if (approx_name && approx_name->type != VarType::MULTI) {
    std::string type_name = approx_name->Name();
    type_name = type_name.substr(type_name.find(".") + 1);
    type_name = type_name.substr(0, type_name.find("."));
    std::string label = "F2V_GEP_" + type_name;
    FG.G[vertex].label_ids.push_back(FG.GetOrCreateId(label + parent_fname));
  } else {
    FG.G[vertex].label_ids.push_back(
        FG.GetOrCreateId("F2V_INST_getelementptr_" + parent_fname));
  }
}

std::unordered_map<int, std::string> InstructionLabelsPass::GetIdToLabel() {
  std::unordered_map<int, std::string> id_to_label;
  std::transform(label_to_id.begin(), label_to_id.end(),
                 std::inserter(id_to_label, id_to_label.begin()),
                 [](const std::pair<std::string, int> p) {
                   return std::pair<int, std::string>(p.second, p.first);
                 });

  return id_to_label;
}

std::string LabelVisitor::ResolveIndirectName(llvm::Instruction *I,
                                              llvm::Value *indirect_value) {
  std::string indirect_name = "";
  if (rpp->output_facts_.find(I) != rpp->output_facts_.end()) {
    ReturnPropagationFact rpf = *(rpp->output_facts_.at(I));

    if (rpf.value.find(indirect_value) != rpf.value.end()) {
      std::unordered_set<const llvm::Value *> possible_values =
          (rpf.value.at(indirect_value));
      if (possible_values.size() == 0) {
        return "";
      }
      const llvm::Value *v = SelectRandom(possible_values);
      if (const llvm::ConstantInt *stored_int =
              llvm::dyn_cast<llvm::ConstantInt>(v)) {
        if (stored_int->getBitWidth() <= 64) {
          int64_t stored_int_value = stored_int->getSExtValue();
          indirect_name = "_INDIR_" + std::to_string(stored_int_value);
        }
      } else if (const llvm::CallInst *inst =
                     llvm::dyn_cast<llvm::CallInst>(v)) {
        indirect_name = "_" + GetCalleeSourceName(*inst);
      }
    }
  }

  return indirect_name;
}

const llvm::Value *LabelVisitor::SelectRandom(
    std::unordered_set<const llvm::Value *> s) {
  auto r = rand() % s.size();
  auto it = std::begin(s);

  std::advance(it, r);
  return *it;
}

void InstructionLabelsPass::getAnalysisUsage(llvm::AnalysisUsage &AU) const {
  AU.setPreservesAll();
  AU.addRequired<NamesPass>();
  AU.addRequired<ControlFlowPass>();
  AU.addRequired<ReturnPropagationPass>();
}

char InstructionLabelsPass::ID = 0;
static llvm::RegisterPass<InstructionLabelsPass> X(
    "instruction-labels", "Add per-instruction labels to the flowgraph", false,
    false);

}  // namespace error_specifications
