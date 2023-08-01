// This file defines the C++ API for the walker gRPC calls.
// See proto/walker.proto for details about individual rpc calls.

#ifndef ERROR_SPECIFICATIONS_WALKER_WALKER_SERVER_H_
#define ERROR_SPECIFICATIONS_WALKER_WALKER_SERVER_H_

#include "proto/walker.grpc.pb.h"

#include <string>
#include <unordered_map>

#include "operations_service.h"
#include "proto/bitcode.grpc.pb.h"

namespace error_specifications {

// Logic and data behind the server's behavior.
class WalkerServiceImpl final : public WalkerService::Service {
  // Non-blocking for client.
  // Performs a random walk over legacy ICFG (func2vec) artifact.
  grpc::Status RandomWalkLegacyIcfgBackground(
      grpc::ServerContext *context, const RandomWalkLegacyIcfgRequest *request,
      Operation *operation) override;

  // Blocking for client.
  // Performs a random walk over legacy ICFG (func2vec) artifact, and
  // streams the sentences back as they are generated.
  grpc::Status RandomWalkLegacyIcfg(
      grpc::ServerContext *context, const RandomWalkLegacyIcfgRequest *request,
      grpc::ServerWriter<Sentence> *writer) override;

 public:
  // The operations service for managing long-running tasks.
  OperationsServiceImpl operations_service_;
};

// Start up the BitcodeService.
void RunWalkerServer(std::string server_address);

}  // namespace error_specifications.

#endif  // ERROR_SPECIFICATIONS_WALKER_WALKER_SERVER_H_
