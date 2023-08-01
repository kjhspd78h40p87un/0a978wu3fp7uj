// This file defines the C++ API for the CheckerService gRPC calls.
// See proto/checker.proto for details about individual rpc calls.

#ifndef ERROR_SPECIFICATIONS_CHECKER_INCLUDE_CHECKER_SERVER_H_
#define ERROR_SPECIFICATIONS_CHECKER_INCLUDE_CHECKER_SERVER_H_

#include <string>
#include <unordered_map>

#include "operations_service.h"
#include "proto/checker.grpc.pb.h"

namespace error_specifications {

// Logic and data behind the server's behavior.
class CheckerServiceImpl final : public CheckerService::Service {
 public:
  // TBB can throw exceptions.
  ~CheckerServiceImpl() throw() {}

  grpc::Status GetViolations(grpc::ServerContext *context,
                             const GetViolationsRequest *request,
                             Operation *operation);

  // The operations service is responsible for keeping track of the status
  // of running tasks.
  OperationsServiceImpl operations_service_;
};

// Start the Checker service.
void RunCheckerServer(const std::string &server_address);

}  // namespace error_specifications

#endif  // ERROR_SPECIFICATIONS_CHECKER_INCLUDE_CHECKER_SERVER_H_
