#include "utility.h"

#include <fstream>
#include "glog/logging.h"
#include "llvm/Support/raw_ostream.h"

namespace error_specifications {

Location GetSource(llvm::Instruction *I) {
  if (I == nullptr) {
    LOG(ERROR) << "getSource requested for nullptr\n";
    abort();
  }

  llvm::DILocation *loc = I->getDebugLoc();

  return loc ? Location(loc->getFilename(), loc->getLine()) : Location();
}

// Read the list of interesting functions from a file
// One function name per line
std::unordered_set<std::string> read_interesting_functions(
    const std::string &path) {
  std::unordered_set<std::string> ret;
  std::ifstream f_functions(path);
  std::string line;

  while (getline(f_functions, line)) {
    ret.insert(line);
  }

  return ret;
}

// For some functions LLVM will
std::string getName(llvm::Function &F) {
  std::string name = F.getName();
  auto idx = name.find('.');
  if (idx != std::string::npos) {
    name = name.substr(0, idx);
  }
  return name;
}

};  // namespace error_specifications
