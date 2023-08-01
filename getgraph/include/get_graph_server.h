// This file defines the C++ API for the GetGraphService gRPC calls.
// See proto/get_graph.proto for details about individual rpc calls.

#ifndef ERROR_SPECIFICATIONS_GET_GRAPH_INCLUDE_GET_GRAPH_SERVER_H_
#define ERROR_SPECIFICATIONS_GET_GRAPH_INCLUDE_GET_GRAPH_SERVER_H_

#include <string>

#include "tbb/task.h"

#include "flow_graph.h"
#include "operations_service.h"
#include "proto/get_graph.grpc.pb.h"
#include "proto/operations.grpc.pb.h"

namespace error_specifications {

// Logic and data behind the server's behavior.
class GetGraphServiceImpl final : public GetGraphService::Service {
  grpc::Status GetGraph(grpc::ServerContext *context,
                        const GetGraphRequest *request,
                        Operation *operation) override;

 public:
  // Because TBB can throw exceptions.
  ~GetGraphServiceImpl() throw() {}

  // The operations service for this GetGraph service.
  OperationsServiceImpl operations_service;
};

// This is a TBB task that runs GetGraph on a bitcode file by
// id. The Bitcode file is retrieved from the bitcode service and
// the operations service is updated when the task is complete.
class GetGraphTask : public tbb::task {
 public:
  tbb::task *execute(void);

  std::string task_name;
  GetGraphRequest request;
  OperationsServiceImpl *operations_service;
  std::string bitcode_server_address;
};

class FileGetGraphWriter {
 public:
  FileGetGraphWriter(std::ofstream *output_file_stream)
      : output_file_stream_(*output_file_stream){};

  ~FileGetGraphWriter() {
    output_file_stream_.flush();
    output_file_stream_.close();
  }

  // Writes out the FlowGraph to the proto Edgelist message format and writes
  // the message out to file.
  Edgelist WriteGraph(FlowGraph *flow_graph,
                      const std::unordered_map<int, std::string> &id_to_label);

 private:
  std::ofstream &output_file_stream_;
};

void RunGetGraphServer(const std::string &get_graph_server_address);

}  // namespace error_specifications

#endif  // ERROR_SPECIFICATIONS_GET_GRAPH_INCLUDE_GET_GRAPH_SERVER_H_
