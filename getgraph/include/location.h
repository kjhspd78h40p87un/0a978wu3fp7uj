// The pair of file / line number is the best unique identifier for an
// instruction that we have that remains constant between llvm instantiations.

#ifndef ERROR_SPECIFICATIONS_GET_GRAPH_INCLUDE_LOCATION_H_
#define ERROR_SPECIFICATIONS_GET_GRAPH_INCLUDE_LOCATION_H_

#include <string>
#include "llvm/Support/raw_ostream.h"

class Location {
 public:
  std::string file;
  unsigned line = 0;

  Location() {}
  Location(std::string file, unsigned line) : file(file), line(line) {}
  Location(std::pair<std::string, unsigned> loc)
      : file(loc.first), line(loc.second) {}

  bool Empty() const { return line == 0; }

  bool operator==(const Location &right) const {
    return file == right.file && line == right.line;
  }

  bool operator!=(const Location &right) const {
    return file != right.file || line != right.line;
  }

  // So Locations can be inserted into sets
  bool operator<(const Location &right) const {
    if (file == right.file) {
      return line < right.line;
    } else {
      return file < right.file;
    }
  }

  std::string Str() const { return file + ":" + std::to_string(line); }

  friend llvm::raw_ostream &operator<<(llvm::raw_ostream &OS,
                                       const Location &L) {
    return OS << L.Str();
  }

  friend std::ostream &operator<<(std::ostream &OS, const Location &L) {
    return OS << L.Str();
  }
};

#endif
