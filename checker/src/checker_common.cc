#include "checker_common.h"

#include "glog/logging.h"

#include "proto/checker.pb.h"

namespace error_specifications {

bool ShouldCheck(const Function &function, const Specification &specification) {
  if (function.source_name().empty()) {
    return false;
  }

  if (function.return_type() == FunctionReturnType::FUNCTION_RETURN_TYPE_VOID) {
    return false;
  }

  if (specification.lattice_element() ==
      SignLatticeElement::SIGN_LATTICE_ELEMENT_INVALID) {
    LOG(WARNING) << "Specification of " << function.source_name()
                 << " is invalid.";
    return false;
  }

  if (specification.lattice_element() ==
          SignLatticeElement::SIGN_LATTICE_ELEMENT_BOTTOM ||
      specification.lattice_element() ==
          SignLatticeElement::SIGN_LATTICE_ELEMENT_TOP) {
    return false;
  }

  return true;
}

}  // namespace error_specifications
