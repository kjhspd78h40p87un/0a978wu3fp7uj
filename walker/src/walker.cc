#include "walker.h"

#include <fstream>
#include <memory>
#include <random>
#include <string>
#include <vector>

#include "glog/logging.h"
#include "google/cloud/storage/client.h"
#include "include/grpcpp/grpcpp.h"
#include "proto/get_graph.pb.h"
#include "proto/walker.grpc.pb.h"
#include "servers.h"

namespace error_specifications {

Walker::Walker() { std::srand(std::time(0)); }

grpc::Status Walker::RandomWalkLegacyIcfgBackground(
    const RandomWalkLegacyIcfgRequest *request) {
  switch (request->output_walks_uri().scheme()) {
    case Scheme::SCHEME_FILE: {
      std::string output_path;
      grpc::Status convert_uri_status =
          ConvertUriToFilePath(request->output_walks_uri(), output_path);
      if (!convert_uri_status.ok()) {
        const std::string &err_msg =
            "Unable to parse walks output URI. Is it a local file?";
        LOG(ERROR) << err_msg;
        return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT, err_msg);
      }
      std::ofstream ofs(output_path);
      FileWalkWriter file_writer(&ofs);

      return DoRandomWalkLegacyIcfg(request, &file_writer);
    } break;
    case Scheme::SCHEME_GS: {
      google::cloud::StatusOr<google::cloud::storage::Client> client =
          google::cloud::storage::Client::CreateDefaultClient();
      if (!client) {
        const std::string err_msg = "Failed to create GS storage client.";
        LOG(ERROR) << err_msg << client.status();
        return grpc::Status(grpc::StatusCode::UNAUTHENTICATED, err_msg);
      }
      google::cloud::storage::ObjectWriteStream stream =
          client->WriteObject(request->output_walks_uri().authority(),
                              request->output_walks_uri().path());
      GcsWalkWriter gcs_writer(&stream);
      return DoRandomWalkLegacyIcfg(request, &gcs_writer);
    } break;
    default: {
      const std::string &err_msg = "Unsupported scheme.";
      LOG(ERROR) << err_msg;
      return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT, err_msg);
    } break;
  }
}

grpc::Status Walker::RandomWalkLegacyIcfg(
    const RandomWalkLegacyIcfgRequest *request,
    grpc::ServerWriter<Sentence> *writer) {
  LOG(INFO) << "Start RandomWalk";

  GrpcWalkWriter grpc_writer(writer);

  return DoRandomWalkLegacyIcfg(request, &grpc_writer);
}

grpc::Status Walker::DoRandomWalkLegacyIcfg(
    const RandomWalkLegacyIcfgRequest *request, WalkWriter *writer) {
  Lpds lpds;
  grpc::Status read_icfg_status =
      ReadLegacyIcfg(request->input_icfg_uri(), &lpds);
  if (!read_icfg_status.ok()) {
    LOG(ERROR) << "Unable to read the ICFG.";
    return read_icfg_status;
  }

  return RandomWalk(&lpds, writer, request->walks_per_label(),
                    request->walk_length());
}

grpc::Status Walker::RandomWalk(const Lpds *lpds, WalkWriter *writer,
                                int num_walks_per_label, int walk_length) {
  if (walk_length <= 0) {
    LOG(WARNING) << "Random walk of length " << walk_length << " requested.";
    return grpc::Status::OK;
  }

  // Get all of the labels in the LPDS.
  tbb::concurrent_vector<Label> labels = lpds->GetLabels();
  LOG(INFO) << "size of labels " << labels.size();

  tbb::parallel_for(tbb::blocked_range<int>(0, num_walks_per_label),
                    [&](auto walk_numbers) {
                      // Make a copy of the labels for this thread.
                      auto labels_for_this_walk = labels;

                      for (int walk_number = walk_numbers.begin();
                           walk_number != walk_numbers.end(); ++walk_number) {
                        // Shuffle the labels before each walk.
                        std::random_shuffle(labels_for_this_walk.begin(),
                                            labels_for_this_walk.end());

                        // Start a walk over all labels.
                        WalkWorker walk_worker(lpds, walk_number, writer);
                        walk_worker.SingleRandomWalk(labels, walk_length);
                      }
                    });

  return grpc::Status::OK;
}

void GrpcWalkWriter::WriteSentence(const Sentence &sentence) {
  // No need to unlock at the end of the function.
  // Lock is automatically released from a scoped_lock.
  tbb::mutex::scoped_lock lock(write_mutex_);

  assert(grpc_writer_ != nullptr);

  grpc_writer_->Write(sentence);
}

void FileWalkWriter::WriteSentence(const Sentence &sentence) {
  // No need to unlock at the end of the function.
  // Lock is automatically released from a scoped_lock.
  tbb::mutex::scoped_lock lock(write_mutex_);

  for (int i = 0; i < sentence.words_size() - 1; ++i) {
    const std::string &label_str = sentence.words()[i].label();
    output_file_stream_ << label_str << " ";
  }
  const std::string &last_label =
      sentence.words()[sentence.words_size() - 1].label();
  output_file_stream_ << last_label << std::endl;
}

void GcsWalkWriter::WriteSentence(const Sentence &sentence) {
  // No need to unlock at the end of the function.
  // Lock is automatically released from a scoped_lock.
  tbb::mutex::scoped_lock lock(write_mutex_);

  for (int i = 0; i < sentence.words_size() - 1; ++i) {
    const std::string &label_str = sentence.words()[i].label();
    output_stream_ << label_str << " ";
  }
  const std::string &last_label =
      sentence.words()[sentence.words_size() - 1].label();
  output_stream_ << last_label << std::endl;
}

grpc::Status WalkWorker::SingleRandomWalk(
    const tbb::concurrent_vector<Label> &labels, int walk_length) {
  const LpdsEdge *random_edge;

  for (const auto &label : labels) {
    Sentence sentence;

    // Pick a random edge with that label to start walk.
    const tbb::concurrent_vector<const LpdsEdge *> edges_with_label =
        lpds_->GetEdgesForLabel(label);
    assert(edges_with_label.size() != 0);
    if (edges_with_label.size() == 1) {
      random_edge = edges_with_label[0];
    } else {
      std::uniform_int_distribution<int> dist(0, edges_with_label.size() - 1);
      random_edge = edges_with_label[dist(mersenne_twister_)];
    }

    // Add the start label to the beginning of the walk.
    Label *word = sentence.add_words();
    word->CopyFrom(label);

    int edges_visited = 0;
    const LpdsNode *next_node = random_edge->target;
    while (edges_visited < walk_length - 1 && next_node != nullptr) {
      next_node = RandomTransition(next_node, &sentence);
      edges_visited++;
    }

    writer_->WriteSentence(sentence);

    // A walk of one label has finished. Clear the stack for the next label.
    return_stack_ = std::stack<const LpdsEdge *>();
  }

  return grpc::Status::OK;
}

// Returns nullptr if walk cannot continue (end of function, no may return
// edges).
const LpdsNode *WalkWorker::RandomTransition(const LpdsNode *node,
                                             Sentence *sentence) {
  // Follows a random transition out of a node, and manipulates the stack
  // accordingly.
  auto out_edges = lpds_->GetOutEdges(node);

  // Handle the common case where there is only one outgoing edge
  // (corresponding to instructions in a basic block).
  if (out_edges.size() == 1) {
    EmitLabel(out_edges[0], sentence);
    return out_edges[0]->target;
  }

  std::vector<const LpdsEdge *> may_ret_edges;
  std::vector<const LpdsEdge *> non_may_ret_edges;
  for (const auto &out_edge : out_edges) {
    if (out_edge->data.is_may_return) {
      may_ret_edges.push_back(out_edge);
    } else {
      non_may_ret_edges.push_back(out_edge);
    }
  }

  // Only out edges are may return edges. Pop.
  if (may_ret_edges.size() == out_edges.size()) {
    auto random_may_ret_edge = PopTransition(node, sentence, may_ret_edges);
    if (!random_may_ret_edge) {
      return nullptr;
    }
    EmitLabel(random_may_ret_edge, sentence);
    return random_may_ret_edge->target;
  }

  // Pick a random out edge.
  std::uniform_int_distribution<int> dist(0, non_may_ret_edges.size() - 1);
  auto random_edge = non_may_ret_edges[dist(mersenne_twister_)];

  if (random_edge->data.is_call) {
    // Cycle through edges and find the return edge.
    for (const auto &unpicked_edge : out_edges) {
      if (!unpicked_edge->data.is_call) {
        // Push the return site on the stack.
        return_stack_.push(unpicked_edge);
      }
    }
  }

  EmitLabel(random_edge, sentence);

  return random_edge->target;
}

const LpdsEdge *WalkWorker::PopTransition(
    const LpdsNode *node, Sentence *sentence,
    std::vector<const LpdsEdge *> &may_return_edges) {
  if (return_stack_.empty()) {
    if (may_return_edges.size() == 0) {
      return nullptr;
    }
    std::uniform_int_distribution<int> dist(0, may_return_edges.size() - 1);
    return may_return_edges[dist(mersenne_twister_)];
  }

  auto ret = return_stack_.top();
  return_stack_.pop();

  return ret;
}

void WalkWorker::EmitLabel(const LpdsEdge *edge, Sentence *sentence) {
  if (edge->data.labels.size() == 0) {
    return;
  }

  std::uniform_int_distribution<int> dist(0, edge->data.labels.size() - 1);

  const Label &selected_label = edge->data.labels[dist(mersenne_twister_)];
  Label *new_word = sentence->add_words();
  new_word->CopyFrom(selected_label);
}

grpc::Status Walker::ReadLegacyIcfg(const Uri &icfg_uri, Lpds *lpds) const {
  // TODO: Make reading writing for URI generic.
  Edgelist edgelist;
  switch (icfg_uri.scheme()) {
    case Scheme::SCHEME_FILE: {
      LOG(INFO) << "file";
      std::string file_path;
      std::ifstream icfg_ifs(icfg_uri.path(), std::ios::binary);
      if (!icfg_ifs) {
        const std::string &err_msg = "Unable to read ICFG file.";
        LOG(ERROR) << err_msg;
        return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT, err_msg);
      }

      if (!edgelist.ParseFromIstream(&icfg_ifs)) {
        const std::string &err_msg = "Unable to parse ICFG protobuf.";
        LOG(ERROR) << err_msg;
        return grpc::Status(grpc::StatusCode::DATA_LOSS, err_msg);
      }
    } break;
    case Scheme::SCHEME_GS: {
      google::cloud::StatusOr<google::cloud::storage::Client> client =
          google::cloud::storage::Client::CreateDefaultClient();
      if (!client) {
        const std::string &err_msg = "Failed to create GS storage client.";
        LOG(ERROR) << err_msg << client.status();
        return grpc::Status(grpc::StatusCode::UNAUTHENTICATED, err_msg);
      }
      google::cloud::storage::ObjectReadStream stream =
          client->ReadObject(icfg_uri.authority(), icfg_uri.path());

      if (!edgelist.ParseFromIstream(&stream)) {
        const std::string &err_msg = "Unable to parse ICFG protobuf.";
        LOG(ERROR) << err_msg;
        return grpc::Status(grpc::StatusCode::DATA_LOSS, err_msg);
      }
    } break;
    default: {
      const std::string &err_msg = "Unsupported scheme.";
      LOG(ERROR) << err_msg;
      return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT, err_msg);
    } break;
  }

  LOG(INFO) << "Parsed ICFG.";

  auto label_id_to_str = edgelist.id_to_label();

  // Map from nodes IDs of call nodes to the function being called.
  std::unordered_map<std::string, std::unordered_set<std::string>> call_nodes;

  // Collect all of the edges for each node.
  for (const Edge &edge : edgelist.edges()) {
    // Keep track of the return sites for call nodes. That way when we
    // process a non-call edge from a call node, we can attach the label
    // of the function being called.
    if (edge.meta_label() == "call") {
      if (call_nodes.find(edge.source()) == call_nodes.end()) {
        call_nodes[edge.source()] = std::unordered_set<std::string>();
      }
      call_nodes[edge.source()].insert(StripSuffixAfterDot(edge.target()));
    }
  }

  // Insert edges into Lpds, marking call and may_return edges.
  for (const Edge &edge : edgelist.edges()) {
    if (edge.source() == "main.0") {
      continue;
    }

    // Create the nodes in the Lpds.
    const LpdsNode *source_node = lpds->AddNode(LpdsNode(edge.source()));
    const LpdsNode *target_node = lpds->AddNode(LpdsNode(edge.target()));

    LpdsEdge edge_to_add;
    edge_to_add.source = source_node;
    edge_to_add.target = target_node;

    LpdsEdgeData edge_data;
    // edge.meta_label holds the three meta-labels "ret", "call", and "may_ret"
    // We don't do anything sepcial for the return edge at this step.
    if (edge.meta_label() == "call") {
      edge_data.is_call = true;
    } else if (edge.meta_label() == "may_ret") {
      edge_data.is_may_return = true;
    }

    // If this is call node, but not a call edge, then it should be labeled
    // with the name of the function being called.
    if (call_nodes.find(edge.source()) != call_nodes.end()) {
      for (const std::string &call_name : call_nodes.at(edge.source())) {
        bool has_call = false;
        for (const Label &existing_call_label : edge_data.labels) {
          if (existing_call_label.label() == call_name) {
            has_call = true;
          }
        }
        if (!has_call) {
          Label call_label;
          call_label.set_label(call_name);
          edge_data.labels.push_back(call_label);
        }
      }
    }

    // Lookup label names in map and covert IDs to strings.
    for (const int &label_id : edge.label_id()) {
      const std::string &str_label = label_id_to_str[label_id].label();
      Label label;
      label.set_label(str_label);
      edge_data.labels.push_back(label);
    }

    edge_to_add.data = edge_data;

    lpds->AddEdge(edge_to_add);
  }

  LOG(INFO) << "Created LPDS.";

  return grpc::Status::OK;
}

}  // namespace error_specifications
