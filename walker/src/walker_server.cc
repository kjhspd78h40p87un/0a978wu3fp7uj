#include "walker_server.h"

#include "glog/logging.h"
#include "include/grpcpp/grpcpp.h"
#include "tbb/task.h"

#include "proto/walker.grpc.pb.h"
#include "servers.h"
#include "walker.h"

namespace error_specifications {

class RandomWalkLegacyIcfgTask : public tbb::task {
 public:
  tbb::task *execute(void) {
    LOG(INFO) << "Executing " << task_name_;

    Operation result;
    result.set_name(task_name_);

    Walker walker;
    grpc::Status err = walker.RandomWalkLegacyIcfgBackground(&request_);
    if (!err.ok()) {
      LOG(ERROR) << "Unable to complete random walk.";
      google::rpc::Status *error_pb_message = result.mutable_error();
      error_pb_message->set_code(err.error_code());
      error_pb_message->set_message(err.error_message());
      result.set_done(1);
      operations_service_->UpdateOperation(task_name_, result);
      return NULL;
    }

    // Currently we have nothing to put into the response.
    // The file is written to the location in the request.
    RandomWalkLegacyIcfgResponse response;
    response.mutable_request()->CopyFrom(request_);
    result.mutable_response()->PackFrom(response);

    result.set_done(1);
    operations_service_->UpdateOperation(task_name_, result);

    LOG(INFO) << "RandomWalkLegacyIcfg task finished.";

    return NULL;
  }

  std::string task_name_;
  RandomWalkLegacyIcfgRequest request_;
  OperationsServiceImpl *operations_service_;
};

grpc::Status WalkerServiceImpl::RandomWalkLegacyIcfgBackground(
    grpc::ServerContext *context, const RandomWalkLegacyIcfgRequest *request,
    Operation *operation) {
  LOG(INFO) << "Start RandomWalkLegacyIcfgBackground RPC";

  const std::string &task_parameters =
      UriSchemes::scheme_to_string.at(request->input_icfg_uri().scheme()) +
      request->input_icfg_uri().authority() + request->input_icfg_uri().path() +
      std::to_string(request->walk_length()) +
      std::to_string(request->walks_per_label());
  std::string task_parameters_hash;
  HashString(task_parameters, task_parameters_hash);
  const std::string &task_name = "Walk-" + task_parameters_hash;

  operation->set_name(task_name);
  operation->set_done(0);
  operations_service_.UpdateOperation(task_name, *operation);

  RandomWalkLegacyIcfgTask *task =
      new (tbb::task::allocate_root()) RandomWalkLegacyIcfgTask();
  task->task_name_ = task_name;
  task->request_ = *request;
  task->operations_service_ = &operations_service_;
  tbb::task::enqueue(*task);
  return grpc::Status::OK;

  LOG(INFO) << "Finish RandomWalk RPC";

  return grpc::Status::OK;
}

grpc::Status WalkerServiceImpl::RandomWalkLegacyIcfg(
    grpc::ServerContext *context, const RandomWalkLegacyIcfgRequest *request,
    grpc::ServerWriter<Sentence> *writer) {
  LOG(INFO) << "Start RandomWalkLegacyIcfg RPC";

  Walker walker;
  grpc::Status walk_status = walker.RandomWalkLegacyIcfg(request, writer);
  if (!walk_status.ok()) {
    LOG(ERROR) << "Unable to complete random walk.";
    return walk_status;
  }

  LOG(INFO) << "Finish RandomWalkLegacyIcfg RPC";

  return grpc::Status::OK;
}

void RunWalkerServer(std::string server_address) {
  WalkerServiceImpl service;

  grpc::ServerBuilder builder;
  // Listen on the given address without any authentication mechanism.
  builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
  // Register "service" as the instance through which we'll communicate with
  // clients. In this case it corresponds to an *synchronous* service.
  builder.RegisterService(&service);
  builder.RegisterService(&service.operations_service_);

  // Finally assemble the server.
  std::unique_ptr<grpc::Server> server(builder.BuildAndStart());
  std::cout << "Server listening on " << server_address << std::endl;

  // Wait for the server to shutdown. Note that some other thread must be
  // responsible for shutting down the server for this call to ever return.
  server->Wait();
}

}  // namespace error_specifications
