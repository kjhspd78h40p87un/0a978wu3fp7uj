#include "checker_server.h"

#include <iostream>
#include <numeric>
#include <string>

#include "glog/logging.h"
#include "include/grpcpp/grpcpp.h"
#include "insufficient_checks_pass.h"
#include "llvm.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/Module.h"
#include "llvm/IRReader/IRReader.h"
#include "llvm/Support/SourceMgr.h"
#include "proto/bitcode.grpc.pb.h"
#include "servers.h"
#include "tbb/task.h"
#include "unused_calls_pass.h"

namespace error_specifications {

class GetViolationsTask : public tbb::task {
 public:
  GetViolationsTask(ViolationType violation_type)
      : violation_type(violation_type){};

  tbb::task *execute(void) {
    LOG(INFO) << task_name_;
    LOG(INFO) << "Downloading bitcode...";

    Operation result;
    result.set_name(task_name_);

    // Connect to the bitcode service.
    std::shared_ptr<grpc::Channel> channel;
    std::unique_ptr<BitcodeService::Stub> stub;
    channel = grpc::CreateChannel(bitcode_server_address_,
                                  grpc::InsecureChannelCredentials());
    stub = BitcodeService::NewStub(channel);

    grpc::ClientContext download_context;
    DownloadBitcodeRequest download_req;
    download_req.mutable_bitcode_id()->CopyFrom(request_.bitcode_id());
    std::unique_ptr<grpc::ClientReader<DataChunk>> reader(
        stub->DownloadBitcode(&download_context, download_req));
    std::vector<std::string> chunks;
    DataChunk chunk;
    while (reader->Read(&chunk)) {
      chunks.push_back(chunk.content());
    }

    std::string bitcode_bytes =
        std::accumulate(chunks.begin(), chunks.end(), std::string(""));

    LOG(INFO) << "Parsing bitcode\n";

    // Initialize an LLVM MemoryBuffer.
    std::unique_ptr<llvm::MemoryBuffer> buffer =
        llvm::MemoryBuffer::getMemBuffer(bitcode_bytes);

    // Parse IR into an llvm Module.
    llvm::SMDiagnostic err;
    llvm::LLVMContext llvm_context;
    std::unique_ptr<llvm::Module> module(
        llvm::parseIR(buffer->getMemBufferRef(), err, llvm_context));

    if (!module) {
      const std::string &err_msg = "Unable to parse bitcode file.";
      result.mutable_error()->set_code(grpc::StatusCode::DATA_LOSS);
      result.mutable_error()->set_message(err_msg);
      operations_service_->UpdateOperation(task_name_, result);
      LOG(ERROR) << err_msg;
      return NULL;
    }

    llvm::legacy::PassManager pass_manager;

    // Which LLVM pass is run is determined by the type of violation that
    // has been requested.
    GetViolationsResponse get_violations_response;
    if (violation_type == ViolationType::VIOLATION_TYPE_UNUSED_RETURN_VALUE) {
      UnusedCallsPass *unused_calls_pass = new UnusedCallsPass();
      unused_calls_pass->SetViolationsRequest(request_);
      pass_manager.add(unused_calls_pass);
      pass_manager.run(*module);
      get_violations_response = unused_calls_pass->GetViolations();
    } else if (violation_type ==
               ViolationType::VIOLATION_TYPE_INSUFFICIENT_CHECK) {
      LOG(INFO) << "Running Insufficient checks pass";
      InsufficientChecksPass *insufficient_checks_pass =
          new InsufficientChecksPass();
      insufficient_checks_pass->SetViolationsRequest(request_);
      pass_manager.add(insufficient_checks_pass);
      pass_manager.run(*module);
      get_violations_response = insufficient_checks_pass->GetViolations();
    }

    result.set_done(1);

    // Packing into google.protobuf.Any
    result.mutable_response()->PackFrom(get_violations_response);

    operations_service_->UpdateOperation(task_name_, result);

    return NULL;
  }

  std::string task_name_;
  std::string bitcode_server_address_;
  GetViolationsRequest request_;
  OperationsServiceImpl *operations_service_;
  ViolationType violation_type;
};

grpc::Status CheckerServiceImpl::GetViolations(
    grpc::ServerContext *context, const GetViolationsRequest *request,
    Operation *operation) {
  LOG(INFO) << "GetViolations rpc";

  const std::string bitcode_server_address = request->bitcode_id().authority();
  if (bitcode_server_address.empty()) {
    const std::string &err_msg = "Authority missing in bitcode Handle.";
    LOG(ERROR) << err_msg;
    return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT, err_msg);
  }

  // Return the name of the operation so client can check on progress.
  // Include violation_type as part of task name so that results
  // for different violation types do not collide.
  const std::string &task_name = "GetViolations-" +
                                 std::to_string(request->violation_type()) +
                                 "-" + request->bitcode_id().id();
  operation->set_name(task_name);
  operation->set_done(0);
  operations_service_.UpdateOperation(task_name, *operation);

  GetViolationsTask *task = new (tbb::task::allocate_root())
      GetViolationsTask((ViolationType)request->violation_type());
  task->operations_service_ = &operations_service_;
  task->request_ = *request;
  task->task_name_ = task_name;
  task->bitcode_server_address_ = bitcode_server_address;
  tbb::task::enqueue(*task);

  return grpc::Status::OK;
}

void RunCheckerServer(const std::string &server_address) {
  CheckerServiceImpl service;

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
