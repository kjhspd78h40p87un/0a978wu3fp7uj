#ifndef ERROR_SPECIFICATIONS_GET_GRAPH_INCLUDE_VAR_NAME_H_
#define ERROR_SPECIFICATIONS_GET_GRAPH_INCLUDE_VAR_NAME_H_

#include <iostream>
#include <memory>
#include <set>
#include <string>
#include "llvm/IR/Function.h"

enum class VarType { INT, EC, FUNCTION, MEMORY, MULTI, EMPTY };
enum class VarScope { LOCAL, GLOBAL, EMPTY };

class VarName;
class FunctionName;
class MultiName;
class MemoryName;

typedef std::shared_ptr<VarName> vn_t;
typedef std::shared_ptr<FunctionName> fn_t;
typedef std::shared_ptr<MultiName> mul_t;
typedef std::shared_ptr<MemoryName> mem_t;

class VarName {
  friend class NamesPass;

 public:
  VarName() : name_(""), scope(VarScope::EMPTY), type(VarType::EMPTY) {}

  VarName(std::string name, VarScope scope, VarType type,
          llvm::Function *parent)
      : name_(name), scope(scope), type(type), parent(parent) {}

  VarName(std::string name, VarScope scope, VarType type)
      : name_(name), scope(scope), type(type) {}

  VarName(std::string name, VarType type, llvm::Function *parent)
      : name_(name), scope(VarScope::LOCAL), type(type), parent(parent) {}

 protected:
  std::string name_;

 public:
  VarScope scope;
  VarType type;
  llvm::Value *value = nullptr;

  VarType GetType() { return type; }

  void SetName(std::string name) { name_ = name; }

  std::string GetTypeAsStr() {
    switch (type) {
      case VarType::INT:
        return "INT";
        break;
      case VarType::EC:
        return "EC";
        break;
      case VarType::FUNCTION:
        return "FUNCTION";
        break;
      case VarType::MEMORY:
        return "MEMORY";
        break;
      case VarType::MULTI:
        return "MULTI";
        break;
      case VarType::EMPTY:
        return "EMPTY";
        break;
      default:
        abort();
    }
  }

  VarScope getScope() { return scope; }

  bool operator<(const VarName &other) const { return name_ < other.name_; }
  bool operator==(const VarName &other) const { return name_ == other.name_; }

  // For local vars
  llvm::Function *parent = nullptr;

  bool Empty() const { return type == VarType::EMPTY; }
  virtual std::string Name() const { return name_; }
};

class IntName : public VarName {
 public:
  IntName(std::string name, llvm::Function *parent)
      : VarName(name, VarType::INT, parent) {}
  IntName(std::string name) : VarName(name, VarScope::GLOBAL, VarType::INT) {}
};

class FunctionName : public VarName {
 public:
  FunctionName(std::string name)
      : VarName(name, VarScope::GLOBAL, VarType::FUNCTION) {}
  llvm::Function *function;
};

class MultiName : public VarName {
 public:
  MultiName() : VarName("", VarScope::EMPTY, VarType::MULTI) {}

  virtual std::string Name() const override {
    std::cerr << "FATAL ERROR: Treating MultiName like single VarName.\n";
    abort();
  }

  void Insert(vn_t vn) {
    // Flatten MultiNames to prevent nesting
    if (vn->type == VarType::MULTI) {
      mul_t mn = std::static_pointer_cast<MultiName>(vn);

      for (vn_t sub : mn->Names()) {
        if (sub->type == VarType::MULTI) {
          std::cerr << "FATAL ERROR: Nested MultiName detected.";
          abort();
        }

        Insert(sub);
      }
    } else if (lookup_.find(*vn) == lookup_.end()) {
      names_.insert(vn);
      lookup_.insert(*vn);
    }
  }

  std::set<vn_t> Names() const { return names_; }

 private:
  std::set<VarName> lookup_;
  std::set<vn_t> names_;
};

class ErrorName : public VarName {
 public:
  ErrorName(std::string name) : VarName(name, VarScope::EMPTY, VarType::EC) {}
};

class MemoryName : public VarName {
 public:
  MemoryName(std::string base_name, unsigned idx1, unsigned idx2,
             VarScope scope, llvm::Function *parent)
      : VarName(base_name, scope, VarType::MEMORY, parent) {
    this->base_name = base_name;
    this->idx1 = idx1;
    this->idx2 = idx2;
    name_ = base_name + "." + std::to_string(idx1) + "." + std::to_string(idx2);
  }

  std::string base_name;
  unsigned idx1;
  unsigned idx2;
};

#endif

