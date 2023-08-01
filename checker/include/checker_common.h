#ifndef ERROR_SPECIFICATIONS_CHECKER_INCLUDE_CHECKER_COMMON_H_
#define ERROR_SPECIFICATIONS_CHECKER_INCLUDE_CHECKER_COMMON_H_

#include "proto/checker.pb.h"

namespace error_specifications {

// Implements filtering rules for checkers.
// Returns true if checker should check a call to function with specification.
// Returns false if a checker should not.
bool ShouldCheck(const Function &function, const Specification &specification);

}  // namespace error_specifications

#endif  // ERROR_SPECIFICATIONS_CHECKER_INCLUDE_CHECKER_COMMON_H_
