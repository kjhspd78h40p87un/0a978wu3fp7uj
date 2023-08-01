#include "get_graph_server.h"

#include <numeric>

#include "glog/logging.h"
#include "include/grpcpp/grpcpp.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/Module.h"
#include "llvm/IRReader/IRReader.h"
#include "llvm/Support/SourceMgr.h"
#include "tbb/task.h"

#include "control_flow_pass.h"
#include "flow_graph.h"
#include "instruction_labels_pass.h"
#include "names_pass.h"
#include "proto/bitcode.grpc.pb.h"
#include "proto/operations.grpc.pb.h"
#include "servers.h"

namespace error_specifications {

tbb::task *GetGraphTask::execute(void) {
  LOG(INFO) << task_name;

  Operation result;
  result.set_name(task_name);

  // Checking the output URI.
  std::ofstream output_file_stream;
  switch (request.output_graph_uri().scheme()) {
    case Scheme::SCHEME_FILE: {
      std::string output_path;
      grpc::Status convert_uri_status =
          ConvertUriToFilePath(request.output_graph_uri(), output_path);
      if (!convert_uri_status.ok()) {
        const std::string &err_msg =
            "Unable to parse graph output URI. Is it a local file?";
        LOG(ERROR) << err_msg;
        google::rpc::Status *error_pb_message = result.mutable_error();
        error_pb_message->set_code(grpc::StatusCode::DATA_LOSS);
        error_pb_message->set_message(err_msg);
        result.set_done(1);
        operations_service->UpdateOperation(task_name, result);
        return NULL;
      }
      output_file_stream = std::ofstream(output_path);
    } break;
    case Scheme::SCHEME_GS: {
      const std::string &err_msg = "GCS URIs are currently not supported!";
      LOG(ERROR) << err_msg;
      google::rpc::Status *error_pb_message = result.mutable_error();
      error_pb_message->set_code(grpc::StatusCode::INVALID_ARGUMENT);
      error_pb_message->set_message(err_msg);
      result.set_done(1);
      operations_service->UpdateOperation(task_name, result);
      return NULL;
    } break;
    default: {
      const std::string &err_msg = "Unsupported scheme.";
      LOG(ERROR) << err_msg;
      google::rpc::Status *error_pb_message = result.mutable_error();
      error_pb_message->set_code(grpc::StatusCode::INVALID_ARGUMENT);
      error_pb_message->set_message(err_msg);
      result.set_done(1);
      operations_service->UpdateOperation(task_name, result);
      return NULL;
    } break;
  }

  // Connect to the bitcode service
  std::shared_ptr<grpc::Channel> channel;
  std::unique_ptr<BitcodeService::Stub> stub;
  channel = grpc::CreateChannel(bitcode_server_address,
                                grpc::InsecureChannelCredentials());
  stub = BitcodeService::NewStub(channel);

  grpc::ClientContext download_context;
  DownloadBitcodeRequest download_req;
  download_req.mutable_bitcode_id()->CopyFrom(request.bitcode_id());
  std::unique_ptr<grpc::ClientReader<DataChunk>> reader(
      stub->DownloadBitcode(&download_context, download_req));
  std::vector<std::string> chunks;
  DataChunk chunk;
  while (reader->Read(&chunk)) {
    chunks.push_back(chunk.content());
  }

  std::string bitcode_bytes =
      std::accumulate(chunks.begin(), chunks.end(), std::string(""));

  // Initialize an LLVM MemoryBuffer.
  std::unique_ptr<llvm::MemoryBuffer> buffer =
      llvm::MemoryBuffer::getMemBuffer(bitcode_bytes);

  // Parse IR into an llvm Module.
  llvm::SMDiagnostic err;
  llvm::LLVMContext llvm_context;
  std::unique_ptr<llvm::Module> module(
      llvm::parseIR(buffer->getMemBufferRef(), err, llvm_context));

  if (!module) {
    err.print("getgraph-server", llvm::errs());
    abort();
  }

  // Setting up the passes for GetGraph.
  llvm::legacy::PassManager pass_manager;
  NamesPass *names = new NamesPass();
  std::vector<ErrorCode> ec(request.error_codes().begin(),
                            request.error_codes().end());
  names->SetErrorCodes(ec);
  ControlFlowPass *cfp = new ControlFlowPass();
  cfp->remove_cross_folder = request.remove_cross_folder();
  InstructionLabelsPass *ilp = new InstructionLabelsPass();
  pass_manager.add(names);
  pass_manager.add(cfp);
  pass_manager.add(ilp);

  pass_manager.run(*module);

  // Get the FlowGraph and write out to file.
  FlowGraph flow_graph = cfp->GetFlowGraph();
  const auto graph_id_to_label = ilp->GetIdToLabel();
  FileGetGraphWriter fw(&output_file_stream);
  Edgelist out_edgelist = fw.WriteGraph(&flow_graph, graph_id_to_label);

  // Check that the generated file was successfully written.
  std::string graph_data_str;
  grpc::Status read_graph_status =
      ReadUriIntoString(request.output_graph_uri(), graph_data_str);
  if (!read_graph_status.ok()) {
    const std::string err_msg = "Unable to read generated graph file!";
    LOG(ERROR) << err_msg;
    google::rpc::Status *error_pb_message = result.mutable_error();
    error_pb_message->set_code(grpc::StatusCode::DATA_LOSS);
    error_pb_message->set_message(err_msg);
    result.set_done(1);
    operations_service->UpdateOperation(task_name, result);
    return NULL;
  }

  std::string out_graph_id;
  // Hash the bitcode file and use that as unique identifier (handle).
  grpc::Status hash_err = HashString(graph_data_str, out_graph_id);
  if (!hash_err.ok()) {
    const std::string err_msg = "Unable to hash graph data.";
    LOG(ERROR) << err_msg;
    google::rpc::Status *error_pb_message = result.mutable_error();
    error_pb_message->set_code(grpc::StatusCode::DATA_LOSS);
    error_pb_message->set_message(err_msg);
    result.set_done(1);
    operations_service->UpdateOperation(task_name, result);
    return NULL;
  }

  // Build the response and pack in result.
  GetGraphResponse response;
  Handle *graph_handle = response.mutable_graph_id();
  graph_handle->set_id(out_graph_id);
  response.mutable_edgelist()->CopyFrom(out_edgelist);
  result.mutable_response()->PackFrom(response);

  result.set_done(1);
  operations_service->UpdateOperation(task_name, result);
  LOG(INFO) << "GetGraph task finished.";

  return NULL;
};

grpc::Status GetGraphServiceImpl::GetGraph(grpc::ServerContext *context,
                                           const GetGraphRequest *request,
                                           Operation *operation) {
  LOG(INFO) << "GetGraph rpc";

  const std::string bitcode_server_address = request->bitcode_id().authority();
  if (bitcode_server_address.empty()) {
    const std::string &err_msg = "Authority missing in bitcode Handle.";
    LOG(ERROR) << err_msg;
    return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT, err_msg);
  }

  // Return the name of the operation so client can check on progress.
  std::string task_name = GetTaskName("GetGraph", request->bitcode_id().id());
  operation->set_name(task_name);
  operation->set_done(0);
  operations_service.UpdateOperation(task_name, *operation);

  GetGraphTask *task = new (tbb::task::allocate_root()) GetGraphTask();
  task->operations_service = &operations_service;
  task->request = *request;
  task->task_name = task_name;
  task->bitcode_server_address = bitcode_server_address;
  tbb::task::enqueue(*task);

  return grpc::Status::OK;
}

Edgelist FileGetGraphWriter::WriteGraph(
    FlowGraph *FG, const std::unordered_map<int, std::string> &id_to_label) {
  Edgelist edgelist;

  BGL_FORALL_EDGES(e, FG->G, _FlowGraph) {
    FlowVertex &source = FG->G[boost::source(e, FG->G)];
    FlowVertex &target = FG->G[boost::target(e, FG->G)];
    Edge *edge = edgelist.add_edges();
    edge->set_source(source.stack);
    edge->set_target(target.stack);

    // Set the meta labels depending on graph edge.
    if (FG->G[e].may_ret) {
      edge->set_meta_label("may_ret");
    } else if (FG->G[e].call) {
      edge->set_meta_label("call");
    } else if (FG->G[e].ret) {
      edge->set_meta_label("ret");
    } else if (!source.label_ids.empty()) {
      for (const auto &id : source.label_ids) {
        edge->add_label_id(id);
      }
    }

    // Location of source node
    if (!source.loc.Empty()) {
      edge->set_source_location(source.loc.Str());
    }

    assert(edge->label_id_size() == 0 || edge->meta_label().empty());
  }

  auto *m = edgelist.mutable_id_to_label();
  for (const auto &kv : id_to_label) {
    Label label;
    label.set_label(kv.second);
    bool ok = m->insert({kv.first, label}).second;
    assert(ok);
  }

  // Write out to file.
  edgelist.SerializeToOstream(&output_file_stream_);

  return edgelist;
}

void RunGetGraphServer(const std::string &server_address) {
  GetGraphServiceImpl service;

  grpc::ServerBuilder builder;
  // Listen on the given address without any authentication mechanism.
  builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
  // Register "service" as the instance through which we'll communicate with
  // clients. In this case it corresponds to an *synchronous* service.
  builder.RegisterService(&service);
  builder.RegisterService(&service.operations_service);

  // Finally assemble the server.
  std::unique_ptr<grpc::Server> server(builder.BuildAndStart());
  std::cout << "Server listening on " << server_address << std::endl;

  // Wait for the server to shutdown. Note that some other thread must be
  // responsible for shutting down the server for this call to ever return.
  server->Wait();
}

}  // namespace error_specifications
