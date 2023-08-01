#include "names_pass.h"

#include <llvm/IR/CFG.h>
#include <llvm/IR/DebugInfo.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/InstVisitor.h>
#include <llvm/Support/CommandLine.h>
#include <fstream>
#include <iostream>
#include <sstream>
#include <unordered_set>
#include "glog/logging.h"
#include "llvm/IR/BasicBlock.h"

namespace error_specifications {

void NamesPass::SetErrorCodes(std::vector<ErrorCode> error_codes) {
  for (const auto ec : error_codes) {
    error_names_[ec.value()] =
        std::make_shared<ErrorName>("TENTATIVE_" + ec.name());
  }
}

bool NamesPass::runOnModule(llvm::Module &M) {
  module_ = &M;

  // Collect global variables
  for (llvm::Module::global_iterator i = M.global_begin(), e = M.global_end();
       i != e; ++i) {
    VarType t = ResolveType(&*i);
    if (llvm::ConstantStruct *literal = GetStruct(&*i)) {
      // Create struct abstraction based on type
      // TODO: Merge into utility function with visitStore loop
      llvm::StructType *st = literal->getType();
      unsigned num_elems = st->getNumElements();
      for (unsigned idx = 0; idx < num_elems; ++idx) {
        llvm::Constant *element =
            literal->getAggregateElement(idx)->stripPointerCasts();

        if (llvm::Function *f = llvm::dyn_cast<llvm::Function>(element)) {
          std::string approx_str = GetApproxName(st);
          if (!approx_str.empty()) {
            MemoryName approx_name(approx_str, 0, idx, VarScope::GLOBAL,
                                   nullptr);
            BackFunction(approx_name, f);
          }
          if (!i->getName().empty()) {
            MemoryName actual_name(i->getName(), 0, idx, VarScope::GLOBAL,
                                   nullptr);
            BackFunction(actual_name, f);
          }
        }
      }  // Each element
    }

    if (t != VarType::EMPTY) {
      std::string name = i->getName();
      names_[&*i] = std::make_shared<VarName>(name, VarScope::GLOBAL, t);
    }
  }

  // Prepend function name to function arguments,
  // populate raw function names for function pointers
  // and add global return names
  for (llvm::Module::iterator function = M.begin(), e = M.end(); function != e;
       ++function) {
    if (function->isIntrinsic() || function->isDeclaration()) {
      continue;
    }
    SetupFunction(&*function);
  }
  for (llvm::Module::iterator function = M.begin(), e = M.end(); function != e;
       ++function) {
    if (function->isIntrinsic() || function->isDeclaration()) {
      continue;
    }
    visit(&*function);
  }

  return false;
}

// TODO: typedef
std::map<std::string, std::set<std::string>>
NamesPass::GetBootstrapFunctions() {
  std::map<std::string, std::set<std::string>> ret;

  for (std::map<VarName, vn_t>::iterator i = memory_model_.begin(),
                                         e = memory_model_.end();
       i != e; ++i) {
    std::string idx = i->first.Name();
    vn_t value = i->second;

    // TODO: Isolate string parsing somewhere
    if (idx.find("#") != std::string::npos) continue;
    if (ret.find(idx) == ret.end()) {
      ret[idx] = std::set<std::string>();
    }

    if (value->type == VarType::FUNCTION) {
      ret[idx].insert(value->Name());
    } else if (value->type == VarType::MULTI) {
      mul_t mul_value = std::static_pointer_cast<MultiName>(value);
      for (const vn_t v : mul_value->Names()) {
        if (v->type == VarType::FUNCTION) {
          ret[idx].insert(v->Name());
        }
      }
    }
  }

  for (std::map<std::string, std::set<std::string>>::iterator i = ret.begin(),
                                                              e = ret.end();
       i != e; ++i) {
    if (i->second.size() < 2) {
      ret.erase(i);
    }
  }

  return ret;
}

std::map<int, vn_t> NamesPass::GetErrorNames() { return error_names_; }

std::string NamesPass::GetApproxName(llvm::StructType *st) {
  if (st->isLiteral()) return "";
  std::string approx_str = st->getName();

  std::string::size_type struct_dot = approx_str.find("struct.");
  if (struct_dot == std::string::npos) {
    struct_dot = 0;
  } else {
    struct_dot = 7;
  }
  std::string::size_type dot = approx_str.find('.', struct_dot + 1);
  if (dot != std::string::npos) {
    approx_str = approx_str.substr(0, dot);
  }

  return approx_str;
}

void NamesPass::SetupFunction(llvm::Function *function) {
  vn_t extant = GetVarName(function);
  if (!extant) {
    fn_t fname = std::make_shared<FunctionName>(function->getName());
    fname->function = &*function;
    names_[&*function] = fname;
    extant = fname;
  }

  fn_t fname = std::static_pointer_cast<FunctionName>(extant);

  for (auto a = function->arg_begin(), ae = function->arg_end(); a != ae; ++a) {
    llvm::Value *arg = &*a;
    VarType t = ResolveType(arg);
    if (t == VarType::EMPTY) {
      continue;
    }

    // This is were the formal arg names are generated
    // The actual names are generate in visitStoreInst
    std::string an = fname->Name() + "$" + arg->getName().str();
    names_[&*a] = std::make_shared<VarName>(an, VarScope::GLOBAL, t);
  }
}

// Two requests for VarName of same instruction will yield same vn_t.
// What about same name for different instructions?
vn_t NamesPass::GetVarName(const llvm::Value *V) {
  if (!V) {
    LOG(ERROR) << "VarName requested for null value.";
    abort();
  }

  const llvm::Value *stripped_V;
  if (!llvm::isa<llvm::GetElementPtrInst>(V)) {
    stripped_V = V->stripPointerCasts();
  } else {
    stripped_V = V;
  }

  // Error code
  if (const llvm::ConstantInt *c =
          llvm::dyn_cast<llvm::ConstantInt>(stripped_V)) {
    int numberValue = c->getLimitedValue();
    if (error_names_.find(numberValue) != error_names_.end()) {
      return error_names_.at(numberValue);
    } else {
      return EC_OK_;
    }
  }

  // Something our analysis couldn't handle
  if (names_.find(stripped_V) == names_.end()) {
    return nullptr;
  }

  return names_.at(stripped_V);
}

std::string NamesPass::GetStackName(llvm::Instruction &I) {
  std::string fname = I.getParent()->getParent()->getName();
  std::string iid;

  if (stack_iids_.find(&I) != stack_iids_.end()) {
    iid = stack_iids_[&I];
  } else {
    iid = std::to_string(stack_cnt_++);
    stack_iids_[&I] = iid;
  }

  return fname + "." + iid;
}

std::string NamesPass::GetDummyName(llvm::Instruction *I) {
  std::string fname = I->getParent()->getParent()->getName();
  return fname + "." + std::to_string(dummy_cnt_++) + "x";
}

// Basic blocks are named by the iid of their first instruction
// bbe is appended for the entry point and bbx for the exit
std::pair<std::string, std::string> NamesPass::GetBBNames(
    llvm::BasicBlock &BB) {
  std::pair<std::string, std::string> ret_names;

  llvm::Instruction &front = BB.front();
  std::string bb_enter = GetStackName(front) + "bbe";
  std::string bb_exit = GetStackName(front) + "bbx";

  ret_names = std::make_pair(bb_enter, bb_exit);
  return ret_names;
}

// Call names are just plain strings (stack locations)
std::string NamesPass::GetCallName(llvm::Function &F) {
  std::string fname = F.getName();
  return fname + ".0";
}

// Get the names of the functions that are called by this instruction
// This is not the same as the callname stack location
// TODO: Disambiguate this
std::vector<std::string> NamesPass::GetCalleeNames(const llvm::CallInst &CI) {
  std::vector<std::string> callee_names;

  const llvm::Value *cv = CI.getCalledValue();
  if (!cv) {
    return callee_names;
  }

  const llvm::Function *f = CI.getCalledFunction();
  if (f && !f->isIntrinsic()) {
    callee_names.push_back(f->getName());
    return callee_names;
  }

  vn_t callee_name = GetVarName(cv);
  if (callee_name) {
    mul_t callees;
    if (callee_name->type == VarType::MULTI) {
      callees = std::static_pointer_cast<MultiName>(callee_name);
    } else {
      callees = std::make_shared<MultiName>();
      callees->Insert(callee_name);
    }

    for (vn_t callee_vn : callees->Names()) {
      if (callee_vn->type != VarType::FUNCTION) {
        continue;
      }
      fn_t callee_fn = std::static_pointer_cast<FunctionName>(callee_vn);
      callee_names.push_back(callee_fn->Name());
    }
  }

  return callee_names;
}

std::set<llvm::Value *> NamesPass::GetLocalValues(llvm::Function &F) {
  return locals_[&F];
}

VarType NamesPass::ResolveType(llvm::Value *V) {
  llvm::Type *t = V->getType();

  if (const llvm::GlobalVariable *gv =
          llvm::dyn_cast<llvm::GlobalVariable>(V)) {
    if (gv->hasGlobalUnnamedAddr()) {
      return VarType::EMPTY;
    }
  }

  if (llvm::isa<llvm::ConstantPointerNull>(V)) {
    return VarType::EMPTY;
  }

  // We assume any pointer could point to an int
  if (t->isPointerTy() || t->isIntegerTy(32) || t->isIntegerTy(64)) {
    return VarType::INT;
  }

  return VarType::EMPTY;
}

bool NamesPass::Filter(llvm::Type *T) {
  if (T->isPointerTy()) {
    return true;
  }

  return T->isIntegerTy(32) || T->isIntegerTy(64);
}

llvm::ConstantStruct *NamesPass::GetStruct(llvm::Value *V) {
  llvm::Type *t = V->getType();
  llvm::Constant *c = llvm::dyn_cast<llvm::Constant>(V);
  unsigned num_operands = 0;
  if (c) {
    // Filter out things like null which are constant but with no operands
    num_operands = c->getNumOperands();
  }

  // Struct constants are pointers
  if (llvm::isa<llvm::PointerType>(t) && num_operands >= 1) {
    if (llvm::ConstantStruct *literal =
            llvm::dyn_cast<llvm::ConstantStruct>(c->getOperand(0))) {
      return literal;
    }
  }

  return nullptr;
}

void NamesPass::BackFunction(MemoryName index, llvm::Function *f) {
  fn_t fpoint = std::make_shared<FunctionName>(f->getName());
  fpoint->function = f;
  mul_t fp_container = std::make_shared<MultiName>();
  fp_container->Insert(fpoint);
  UpdateMemory(index, fp_container);
}

void NamesPass::UpdateMemory(MemoryName index, vn_t update) {
  if (index.Name() == "union" || index.Name() == "union.0.0") {
    return;
  }

  vn_t backing_name = memory_model_[index];

  mul_t update_mul = nullptr;
  mul_t backing_mul = nullptr;
  if (backing_name && backing_name->type == VarType::MULTI) {
    backing_mul = std::static_pointer_cast<MultiName>(backing_name);
  }
  if (update->type == VarType::MULTI) {
    update_mul = std::static_pointer_cast<MultiName>(update);
  }

  if (update_mul && backing_mul) {
    // Both MultiName
    for (vn_t vn : update_mul->Names()) {
      backing_mul->Insert(vn);
    }
  } else if (!update_mul && backing_mul) {
    // Only receiver is MultiName
    backing_mul->Insert(update);
  } else if (update_mul && !backing_mul && backing_name) {
    update_mul->Insert(backing_name);
    memory_model_[index] = update_mul;
  } else if (update->type == VarType::FUNCTION && backing_name &&
             update->Name() != backing_name->Name()) {
    // Where MultiNames are created, only handle functions for now
    // Removing the VarType::FUNCTION filter above will require adjustiing
    // several visitor functions to handle MultiNames
    mul_t multi = std::make_shared<MultiName>();
    multi->Insert(backing_name);
    multi->Insert(update);
    memory_model_[index] = multi;

  } else {
    memory_model_[index] = update;
  }
}

mem_t NamesPass::GetLoadIndex(const llvm::Value *v) const {
  if (load_index_.find(v) == load_index_.end()) {
    return nullptr;
  }
  return load_index_.at(v);
}

// Name of load instruction is the name of what it loads
void NamesPass::visitLoadInst(llvm::LoadInst &I) {
  llvm::Value *from = I.getOperand(0);

  if (llvm::ConstantExpr *expr = llvm::dyn_cast<llvm::ConstantExpr>(from)) {
    // To avoid duplicating logic for expressions,
    // visit this as if it were an instruction
    // (generates GEP name)
    llvm::Instruction *i_expr = expr->getAsInstruction();
    to_cleanup_.push_back(i_expr);
    visit(i_expr);
    from = i_expr;
  }

  if (llvm::isa<llvm::GetElementPtrInst>(from)) {
    vn_t gep_name = GetVarName(from);
    if (!gep_name || gep_name->type != VarType::MEMORY) {
      return;
    }

    mem_t gep_mem = std::static_pointer_cast<MemoryName>(gep_name);

    // Need to look up backing VarName
    if (memory_model_.find(*gep_mem) != memory_model_.end()) {
      // We already have the backing name
      names_[&I] = memory_model_.at(*gep_mem);
      load_index_[&I] = gep_mem;
      vn_t vn = names_[&I];
    } else if (llvm::GlobalValue *gv =
                   module_->getNamedValue(gep_mem->base_name)) {
      // Global constant GEP lookups
      if (llvm::ConstantStruct *literal = GetStruct(gv)) {
        llvm::Constant *element = literal->getAggregateElement(gep_mem->idx2);
        if (element) {
          element = element->stripPointerCasts();
          vn_t el_name = GetVarName(element);
          names_[&I] = el_name;
        }
        load_index_[&I] = gep_mem;
      }  // constant struct
    } else {
      // Try approximating by struct type of load target
      llvm::GetElementPtrInst *gep_inst =
          llvm::dyn_cast<llvm::GetElementPtrInst>(from);
      mem_t index = GetApproxName(*gep_inst);

      if (index && memory_model_.find(*index) != memory_model_.end()) {
        names_[&I] = memory_model_.at(*index);
        load_index_[&I] = index;
      }
    }
  } else {
    // Non-GEP
    // We already have a usable name for load, just copy the reference
    vn_t ref = GetVarName(from);

    // TODO: If this is VarType::MEMORY log imprecision

    if (ref && ref->type != VarType::MEMORY) {
      names_[&I] = ref;
    }
  }
}

mem_t NamesPass::GetApproxName(llvm::GetElementPtrInst &I) {
  mem_t ret = nullptr;

  llvm::Type *t = I.getOperand(0)->getType()->getContainedType(0);
  if (llvm::StructType *st = llvm::dyn_cast<llvm::StructType>(t)) {
    llvm::Value *idx2 = I.getOperand(2);
    llvm::ConstantInt *idx2_int = llvm::dyn_cast<llvm::ConstantInt>(idx2);
    std::string approx_str = GetApproxName(st);
    ret = std::make_shared<MemoryName>(
        approx_str, 0, idx2_int->getLimitedValue(), VarScope::GLOBAL, nullptr);
  }

  return ret;
}

// Local variables have function name prepended
void NamesPass::visitAllocaInst(llvm::AllocaInst &I) {
  vn_t vn = names_[&I];

  // Already have a name from llvm.dbg
  if (vn) {
    return;
  }

  // No real var name, generate an intermediate name (cabs2cil_)
  std::string intermediate = GenerateIntermediateName();

  llvm::Function *f = I.getParent()->getParent();
  names_[&I] = std::make_shared<IntName>(intermediate, f);
  locals_[f].insert(&I);
}

std::string NamesPass::GenerateIntermediateName() {
  return "cabs2cil_" + std::to_string(intermediate_cnt_++);
}

// Set name to name of global exchange var
void NamesPass::visitCallInst(llvm::CallInst &I) {
  // dbg.declares give us a more reliable name than getName
  // GetVarName will prefer information provided here
  if (llvm::DbgDeclareInst *dbg = llvm::dyn_cast<llvm::DbgDeclareInst>(&I)) {
    llvm::DIVariable *DV = static_cast<llvm::DIVariable *>(dbg->getVariable());
    llvm::Value *V = dbg->getAddress();

    llvm::Function *f = I.getParent()->getParent();
    std::string fname = f->getName();
    std::string vname = DV->getName();

    names_[V] = std::make_shared<IntName>(fname + "#" + vname, f);
    locals_[f].insert(V);
    return;
  }

  if (llvm::isa<llvm::IntrinsicInst>(&I)) {
    return;
  }

  llvm::Value *cv = I.getCalledValue();
  if (!cv) {
    return;
  }

  vn_t callee = GetVarName(cv);
  if (!callee) {
    return;
  }

  if (callee->type == VarType::FUNCTION) {
    // Indirect call with a single value
    fn_t function_name = std::static_pointer_cast<FunctionName>(callee);
    std::string callee_name = function_name->function->getName();
    vn_t exchange = std::make_shared<VarName>(callee_name + "$return",
                                              VarScope::GLOBAL, VarType::INT);
    names_[&I] = exchange;
  } else if (callee->type == VarType::MULTI) {
    // Indirect call with multiple possibilities
    mul_t multi_callee = std::static_pointer_cast<MultiName>(callee);
    mul_t exchange_container = std::make_shared<MultiName>();

    for (vn_t fn : multi_callee->Names()) {
      if (fn->type != VarType::FUNCTION) {
        continue;
      }

      fn_t function_name = std::static_pointer_cast<FunctionName>(fn);
      std::string callee_name = function_name->function->getName();
      vn_t exchange = std::make_shared<VarName>(callee_name + "$return",
                                                VarScope::GLOBAL, VarType::INT);
      exchange_container->Insert(exchange);
    }
    names_[&I] = exchange_container;

  } else {
    // Normal call
    llvm::Function *f = I.getCalledFunction();
    if (f) {
      std::string callee_name = f->getName();
      vn_t exchange = std::make_shared<VarName>(callee_name + "$return",
                                                VarScope::GLOBAL, VarType::INT);
      names_[&I] = exchange;
    }
  }
}

void NamesPass::visitStoreInst(llvm::StoreInst &I) {
  llvm::Value *sender = I.getOperand(0)->stripPointerCasts();
  llvm::Value *receiver = I.getOperand(1);

  vn_t sender_name = GetVarName(sender);
  vn_t receiver_name = GetVarName(receiver);
  if (!sender_name || !receiver_name) {
    return;
  }

  // We handle storing to GEP separately;
  // It is indexed by MemoryNames (GEP indexes), pointing at "backing"
  // VarNames
  llvm::Type *sender_type = sender->getType();
  if (llvm::isa<llvm::GetElementPtrInst>(receiver)) {
    // Check to see if memory model already has a VarName for GEP
    vn_t gep_name = GetVarName(receiver);
    if (!gep_name) {
      return;
    }
    if (gep_name->type != VarType::MEMORY) {
      LOG(ERROR) << "GEP with non-memory name!";
      abort();
    }
    mem_t mem_name = std::static_pointer_cast<MemoryName>(gep_name);

    UpdateMemory(*mem_name, sender_name);
    return;
  }

  // Handle global struct literals with function pointers in them
  // If sender is a struct literal then we need to initialize backing
  // VarNames for receiver

  if (llvm::ConstantStruct *literal = GetStruct(sender)) {
    // Go through each element in struct literal and create backing VarName
    llvm::StructType *st =
        llvm::cast<llvm::StructType>(sender_type->getContainedType(0));
    unsigned num_elems = st->getNumElements();
    for (unsigned idx = 0; idx < num_elems; ++idx) {
      llvm::Constant *element =
          literal->getAggregateElement(idx)->stripPointerCasts();

      if (llvm::Function *f = llvm::dyn_cast<llvm::Function>(element)) {
        if (receiver_name && receiver_name->type != VarType::MULTI) {
          std::string r_name = receiver_name->Name();
          MemoryName mem_name(r_name, 0, idx, receiver_name->scope,
                              receiver_name->parent);
          BackFunction(mem_name, f);
        }
      }
    }  // Each element
  }

  // To transparently merge pointer names
  receiver = receiver->stripPointerCasts();

  VarType t = ResolveType(sender);
  if (t == VarType::EMPTY) {
    return;
  }

  // If sender is a function, then this is a function pointer
  // Copy sender name to receiver name. This will overwrite the
  // receiver name, which is OK in this case.
  if (sender_name->GetType() == VarType::FUNCTION) {
    names_[receiver] = sender_name;
  }

  llvm::Function *f = I.getParent()->getParent();
  // Check to see if this is a copy from formal to actual arguments
  for (auto a = f->arg_begin(), ae = f->arg_end(); a != ae; ++a) {
    if (&*a == sender) {
      std::string fname = f->getName();
      std::string arg_name = a->getName();
      std::string receiver_name = fname + "#" + arg_name;
      names_[receiver] = std::make_shared<IntName>(receiver_name, f);
      locals_[f].insert(receiver);
    }
  }
}

// add, sub, etc.
// Any binary operations imply it wasn't an error code
void NamesPass::visitBinaryOperator(llvm::BinaryOperator &I) {
  names_[&I] = std::make_shared<ErrorName>("OK");
}

void NamesPass::visitPtrToIntInst(llvm::PtrToIntInst &I) {
  names_[&I] = GetVarName(I.getOperand(0));
}

void NamesPass::visitIntToPtrInst(llvm::IntToPtrInst &I) {
  names_[&I] = GetVarName(I.getOperand(0));
}

// // We take phi instructions and create a multiname from all possible values
void NamesPass::visitPHINode(llvm::PHINode &I) {
  mul_t phi = std::make_shared<MultiName>();
  for (unsigned i = 0, e = I.getNumIncomingValues(); i != e; ++i) {
    vn_t vn = GetVarName(I.getIncomingValue(i));
    if (!vn) continue;
    if (vn->GetType() == VarType::MULTI) {
      mul_t mn = std::static_pointer_cast<MultiName>(vn);
      for (vn_t sub : mn->Names()) {
        phi->Insert(sub);
      }
    } else {
      phi->Insert(vn);
    }
  }
  names_[&I] = phi;
}

void NamesPass::visitSelectInst(llvm::SelectInst &I) {
  vn_t true_vn = GetVarName(I.getTrueValue());
  vn_t false_vn = GetVarName(I.getFalseValue());

  mul_t select_vn = std::make_shared<MultiName>();

  if (true_vn) {
    select_vn->Insert(true_vn);
  }
  if (false_vn) {
    select_vn->Insert(false_vn);
  }

  if (select_vn->Names().size() > 0) {
    names_[&I] = select_vn;
  }
}

void NamesPass::visitGetElementPtrInst(llvm::GetElementPtrInst &I) {
  if (I.getNumOperands() < 3) {
    return;
  }

  llvm::Value *idx1 = I.getOperand(1);
  llvm::Value *idx2 = I.getOperand(2);
  llvm::ConstantInt *idx1_int = llvm::dyn_cast<llvm::ConstantInt>(idx1);
  llvm::ConstantInt *idx2_int = llvm::dyn_cast<llvm::ConstantInt>(idx2);
  vn_t struct_name = GetVarName(I.getOperand(0));

  // We only support constant indexes
  if (!idx1_int || !idx2_int) {
    return;
  }

  if (struct_name && struct_name->type != VarType::MULTI &&
      struct_name->type != VarType::EC) {
    // Give memory name based on GEP index
    // e.g. main#foo.0.0 for first element in struct foo
    vn_t mn = std::make_shared<MemoryName>(
        struct_name->Name(), idx1_int->getLimitedValue(),
        idx2_int->getLimitedValue(), struct_name->scope, struct_name->parent);
    mn->value = &I;

    names_[&I] = mn;

    if (struct_name->parent != nullptr) {
      locals_[struct_name->parent].insert(&I);
    }
  } else if (!struct_name ||
             (struct_name && struct_name->type == VarType::EC)) {
    // If we don't have a name, but this is a struct, use the approximate
    // name.
    llvm::Type *t = I.getOperand(0)->getType()->getContainedType(0);
    if (llvm::StructType *st = llvm::dyn_cast<llvm::StructType>(t)) {
      std::string approx_str = GetApproxName(st);
      uint64_t idx1_num = idx1_int->getLimitedValue();
      uint64_t idx2_num = idx2_int->getLimitedValue();
      mem_t mn = std::make_shared<MemoryName>(approx_str, idx1_num, idx2_num,
                                              VarScope::GLOBAL, nullptr);
      mn->value = &I;
      names_[&I] = mn;
    }
  }
}

void NamesPass::getAnalysisUsage(llvm::AnalysisUsage &AU) const {
  AU.setPreservesAll();
}

char NamesPass::ID = 0;
static llvm::RegisterPass<NamesPass> N("names", "Propagate Names", false,
                                       false);

}  // namespace error_specifications
