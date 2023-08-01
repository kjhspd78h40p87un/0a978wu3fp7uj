// This file contains end-to-end tests of the GetGraph service that
// go through the gRPC interface.

#include "getgraph/include/get_graph_server.h"

#include <unistd.h>
#include <string>

#include "gtest/gtest.h"
#include "include/grpcpp/grpcpp.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IRReader/IRReader.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/SourceMgr.h"
#include "llvm/Support/raw_ostream.h"

#include "bitcode/include/bitcode_server.h"
#include "llvm.h"
#include "proto/bitcode.grpc.pb.h"
#include "proto/get_graph.grpc.pb.h"
#include "servers.h"

namespace error_specifications {

// Starts the gRPC server before each test and shuts it down after.
// Right now this relies on starting the server with an actual port.
class GetGraphServiceTest : public ::testing::Test {
 protected:
  BitcodeServiceImpl bitcode_service_;
  grpc::ServerBuilder bitcode_builder_;
  std::shared_ptr<grpc::Channel> bitcode_channel_;
  std::unique_ptr<grpc::Server> bitcode_server_;
  std::unique_ptr<BitcodeService::Stub> bitcode_stub_;

  GetGraphServiceImpl get_graph_service_;
  grpc::ServerBuilder get_graph_builder_;
  std::unique_ptr<grpc::Server> get_graph_server_;
  std::shared_ptr<grpc::Channel> get_graph_channel_;
  std::unique_ptr<GetGraphService::Stub> get_graph_stub_;
  std::unique_ptr<OperationsService::Stub> operations_stub_;
  std::string tmp_output_path_ = "testdata/programs/graphservicetest.icfg";

  const std::string test_bitcode_server_address_ = "localhost:70051";

  void SetUp() override {
    // Start the bitcode service.
    bitcode_builder_.AddListeningPort(test_bitcode_server_address_,
                                      grpc::InsecureServerCredentials());
    bitcode_builder_.RegisterService(&bitcode_service_);
    bitcode_server_ = bitcode_builder_.BuildAndStart();
    bitcode_channel_ = grpc::CreateChannel(test_bitcode_server_address_,
                                           grpc::InsecureChannelCredentials());
    bitcode_stub_ = BitcodeService::NewStub(bitcode_channel_);

    // Start the GetGraph service.
    constexpr char kTestGetGraphServerAddress[] = "localhost:70057";
    get_graph_builder_.AddListeningPort(kTestGetGraphServerAddress,
                                        grpc::InsecureServerCredentials());
    get_graph_builder_.RegisterService(&get_graph_service_);
    get_graph_builder_.RegisterService(&get_graph_service_.operations_service);
    get_graph_server_ = get_graph_builder_.BuildAndStart();
    get_graph_channel_ = grpc::CreateChannel(
        kTestGetGraphServerAddress, grpc::InsecureChannelCredentials());
    get_graph_stub_ = GetGraphService::NewStub(get_graph_channel_);
    operations_stub_ = OperationsService::NewStub(get_graph_channel_);
  }

  void TearDown() override {
    bitcode_server_->Shutdown();
    get_graph_server_->Shutdown();
    remove(tmp_output_path_.c_str());
  }
};

// Helper that looks for specific labels within a FlowGraph.
bool LabelsInGraph(const Edgelist &edgelist,
                   const std::set<std::string> &labels_to_find) {
  std::set<std::string> found_labels;
  for (auto edge : edgelist.edges()) {
    for (int label_id : edge.label_id()) {
      std::string label = edgelist.id_to_label().at(label_id).label();
      if (labels_to_find.find(label) != labels_to_find.end()) {
        found_labels.insert(label);
        if (found_labels.size() == labels_to_find.size()) {
          return true;
        }
      }
    }
  }

  return false;
}

bool LabelInGraph(const Edgelist &edgelist, const std::string &label_to_find) {
  return LabelsInGraph(edgelist, {label_to_find});
}

TEST_F(GetGraphServiceTest, BazCoverBarGraph) {
  // Register the bitcode file.
  RegisterBitcodeRequest register_bitcode_req;
  RegisterBitcodeResponse register_bitcode_res;
  grpc::ClientContext register_bitcode_context;

  const Uri &file_uri =
      FilePathToUri("testdata/programs/baz_cover_bar-reg2mem.ll");
  register_bitcode_req.mutable_uri()->CopyFrom(file_uri);
  grpc::Status status = bitcode_stub_->RegisterBitcode(
      &register_bitcode_context, register_bitcode_req, &register_bitcode_res);

  if (status.error_code() != grpc::OK) {
    std::cerr << status.error_message() << std::endl;
  }
  ASSERT_EQ(status.error_code(), grpc::OK);

  // Send GetGraphRequest to GetGraph server.
  Handle remote_bitcode_handle;
  remote_bitcode_handle.set_id(register_bitcode_res.bitcode_id().id());
  remote_bitcode_handle.set_authority(test_bitcode_server_address_);

  GetGraphRequest get_graph_req;
  Operation get_graph_operation;
  grpc::ClientContext get_graph_context;
  get_graph_req.mutable_bitcode_id()->CopyFrom(remote_bitcode_handle);
  const Uri &output_uri = FilePathToUri(tmp_output_path_);
  get_graph_req.mutable_output_graph_uri()->CopyFrom(output_uri);
  status = get_graph_stub_->GetGraph(&get_graph_context, get_graph_req,
                                     &get_graph_operation);
  if (status.error_code() != grpc::OK) {
    std::cerr << status.error_message() << std::endl;
  }
  ASSERT_EQ(status.error_code(), grpc::OK);

  // Get the status of the operation and wait until finished or error.
  // Timeout after 30 seconds.
  int number_of_tries = 0;
  while (!get_graph_operation.done()) {
    grpc::ClientContext get_operation_context;
    GetOperationRequest get_operation_req;
    get_operation_req.set_name(get_graph_operation.name());
    status = operations_stub_->GetOperation(
        &get_operation_context, get_operation_req, &get_graph_operation);

    if (status.error_code() != grpc::OK) {
      std::cerr << status.error_message() << std::endl;
    }
    ASSERT_EQ(status.error_code(), grpc::OK);
    ASSERT_LE(number_of_tries, 10);
    number_of_tries++;
    usleep(1000 * 1000);
  }
  ASSERT_EQ(get_graph_operation.done(), true);

  // Get the results of the operation.
  GetGraphResponse response;
  get_graph_operation.response().UnpackTo(&response);

  // The Graph ID will be based on the hash of the file. The hash of the file
  // will likely not be the same for every test run, considering that the
  // proto is being serialized to a file, which has no guarantee of ordering.
  ASSERT_FALSE(response.graph_id().id().empty()) << response.DebugString();
  ASSERT_EQ(response.edgelist().edges().size(), 55) << response.DebugString();
}

// Tests getting conditional labels for <0, representing both the true and
// false conditions. This bitcode file's original source code returns within
// an if-statement.
TEST_F(GetGraphServiceTest, IfFooReturn) {
  // Register the bitcode file.
  RegisterBitcodeRequest register_bitcode_req;
  RegisterBitcodeResponse register_bitcode_res;
  grpc::ClientContext register_bitcode_context;

  const Uri &file_uri = FilePathToUri("testdata/programs/if_foo_return.ll");
  register_bitcode_req.mutable_uri()->CopyFrom(file_uri);
  grpc::Status status = bitcode_stub_->RegisterBitcode(
      &register_bitcode_context, register_bitcode_req, &register_bitcode_res);

  if (status.error_code() != grpc::OK) {
    std::cerr << status.error_message() << std::endl;
  }
  ASSERT_EQ(status.error_code(), grpc::OK);

  // Send GetGraphRequest to GetGraph server.
  Handle remote_bitcode_handle;
  remote_bitcode_handle.set_id(register_bitcode_res.bitcode_id().id());
  remote_bitcode_handle.set_authority(test_bitcode_server_address_);

  GetGraphRequest get_graph_req;
  Operation get_graph_operation;
  grpc::ClientContext get_graph_context;
  get_graph_req.mutable_bitcode_id()->CopyFrom(remote_bitcode_handle);
  const Uri &output_uri = FilePathToUri(tmp_output_path_);
  get_graph_req.mutable_output_graph_uri()->CopyFrom(output_uri);
  status = get_graph_stub_->GetGraph(&get_graph_context, get_graph_req,
                                     &get_graph_operation);
  if (status.error_code() != grpc::OK) {
    std::cerr << status.error_message() << std::endl;
  }
  ASSERT_EQ(status.error_code(), grpc::OK);

  // Get the status of the operation and wait until finished or error.
  // Timeout after 30 seconds.
  int number_of_tries = 0;
  while (!get_graph_operation.done()) {
    grpc::ClientContext get_operation_context;
    GetOperationRequest get_operation_req;
    get_operation_req.set_name(get_graph_operation.name());
    status = operations_stub_->GetOperation(
        &get_operation_context, get_operation_req, &get_graph_operation);

    if (status.error_code() != grpc::OK) {
      std::cerr << status.error_message() << std::endl;
    }
    ASSERT_EQ(status.error_code(), grpc::OK);
    ASSERT_LE(number_of_tries, 10);
    number_of_tries++;
    usleep(1000 * 1000);
  }
  ASSERT_EQ(get_graph_operation.done(), true);

  // Get the results of the operation.
  GetGraphResponse response;
  get_graph_operation.response().UnpackTo(&response);

  // The Graph ID will be based on the hash of the file. The hash of the file
  // will likely not be the same for every test run, considering that the
  // proto is being serialized to a file, which has no guarantee of ordering.
  ASSERT_FALSE(response.graph_id().id().empty()) << response.DebugString();
  ASSERT_EQ(response.edgelist().edges().size(), 30) << response.DebugString();

  ASSERT_TRUE(LabelsInGraph(response.edgelist(),
                            {"F2V_CONDBR_SLT_ZERO", "F2V_CONDBR_SGT_ZERO"}));
}

// Tests getting conditional labels for <0, representing both the true and
// false conditions. This bitcode file's original source code only contains
// one return statement.
TEST_F(GetGraphServiceTest, IfFooNoReturn) {
  // Register the bitcode file.
  RegisterBitcodeRequest register_bitcode_req;
  RegisterBitcodeResponse register_bitcode_res;
  grpc::ClientContext register_bitcode_context;

  const Uri &file_uri = FilePathToUri("testdata/programs/if_foo_no_return.ll");
  register_bitcode_req.mutable_uri()->CopyFrom(file_uri);
  grpc::Status status = bitcode_stub_->RegisterBitcode(
      &register_bitcode_context, register_bitcode_req, &register_bitcode_res);

  if (status.error_code() != grpc::OK) {
    std::cerr << status.error_message() << std::endl;
  }
  ASSERT_EQ(status.error_code(), grpc::OK);

  // Send GetGraphRequest to GetGraph server.
  Handle remote_bitcode_handle;
  remote_bitcode_handle.set_id(register_bitcode_res.bitcode_id().id());
  remote_bitcode_handle.set_authority(test_bitcode_server_address_);

  GetGraphRequest get_graph_req;
  Operation get_graph_operation;
  grpc::ClientContext get_graph_context;
  get_graph_req.mutable_bitcode_id()->CopyFrom(remote_bitcode_handle);
  const Uri &output_uri = FilePathToUri(tmp_output_path_);
  get_graph_req.mutable_output_graph_uri()->CopyFrom(output_uri);
  status = get_graph_stub_->GetGraph(&get_graph_context, get_graph_req,
                                     &get_graph_operation);
  if (status.error_code() != grpc::OK) {
    std::cerr << status.error_message() << std::endl;
  }
  ASSERT_EQ(status.error_code(), grpc::OK);

  // Get the status of the operation and wait until finished or error.
  // Timeout after 30 seconds.
  int number_of_tries = 0;
  while (!get_graph_operation.done()) {
    grpc::ClientContext get_operation_context;
    GetOperationRequest get_operation_req;
    get_operation_req.set_name(get_graph_operation.name());
    status = operations_stub_->GetOperation(
        &get_operation_context, get_operation_req, &get_graph_operation);

    if (status.error_code() != grpc::OK) {
      std::cerr << status.error_message() << std::endl;
    }
    ASSERT_EQ(status.error_code(), grpc::OK);
    ASSERT_LE(number_of_tries, 10);
    number_of_tries++;
    usleep(1000 * 1000);
  }
  ASSERT_EQ(get_graph_operation.done(), true);

  // Get the results of the operation.
  GetGraphResponse response;
  get_graph_operation.response().UnpackTo(&response);

  // The Graph ID will be based on the hash of the file. The hash of the file
  // will likely not be the same for every test run, considering that the
  // proto is being serialized to a file, which has no guarantee of ordering.
  ASSERT_FALSE(response.graph_id().id().empty()) << response.DebugString();
  ASSERT_EQ(response.edgelist().edges().size(), 29) << response.DebugString();

  ASSERT_TRUE(LabelsInGraph(response.edgelist(),
                            {"F2V_CONDBR_SLT_ZERO", "F2V_CONDBR_SGT_ZERO"}));
}

}  // namespace error_specifications
