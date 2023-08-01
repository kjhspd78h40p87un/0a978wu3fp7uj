// The func2vec random walker.
//
// There are two types of edges in an Lpds that influence walker behavior.
// 1. Call edges cause the walker to push the name of the return node
//    on to the stack. The return node is deduced by looking for the single
//
//    internal edge.
// 2. May return edges are not followed. They are only used to selected a
//    a return site when the end of a function is reached and the return site
//    stack is empty.
//
// If there are no outgoing edges and the return stack is empty, then
// then the walk terminates. The walker also terminates when the path length
// exceeds the maximum path length provided.
//
// Internal edges have multiple labels when there are multiple outgoing call
// edges. One label is randomly selected to emit.

#ifndef ERROR_SPECIFICATIONS_WALKER_INCLUDE_WALKER_H_
#define ERROR_SPECIFICATIONS_WALKER_INCLUDE_WALKER_H_

#include <fstream>
#include <memory>
#include <random>
#include <stack>
#include <vector>

#include "include/grpcpp/grpcpp.h"
#include "lpds.h"
#include "proto/walker.grpc.pb.h"
#include "tbb/tbb.h"

namespace error_specifications {

// Interface for thread-safe writer to output resource.
class WalkWriter {
 public:
  virtual void WriteSentence(const Sentence &sentence) = 0;
};

// Write out sentences to a gRPC ServerWriter.
class GrpcWalkWriter : public WalkWriter {
 public:
  GrpcWalkWriter(grpc::ServerWriter<Sentence> *writer) : grpc_writer_(writer) {}
  virtual void WriteSentence(const Sentence &sentence);

 private:
  // Mutex for locking when writing to a gRPC ServerWriter.
  tbb::mutex write_mutex_;
  // gRPC ServerWriter for sentences.
  grpc::ServerWriter<Sentence> *grpc_writer_ = nullptr;
};

// Write out sentences to a file.
// The format used is "Label.label Label.label\nLabel.label Label.label\n..."
class FileWalkWriter : public WalkWriter {
 public:
  FileWalkWriter(std::ofstream *output_file_stream)
      : output_file_stream_(*output_file_stream) {}

  ~FileWalkWriter() {
    output_file_stream_.flush();
    output_file_stream_.close();
  }

  // Writes out sentences to a file.
  virtual void WriteSentence(const Sentence &sentence);

 private:
  // Mutex for locking when writing to a file.
  tbb::mutex write_mutex_;
  // Output stream associated with the file being written to.
  std::ofstream &output_file_stream_;
};

// Writes out sentences to GCS storage.
// The format for sentences used is:
//    "Label.label Label.label\nLabel.label Label.label\n..."
class GcsWalkWriter : public WalkWriter {
 public:
  GcsWalkWriter(std::ostream *output_stream) : output_stream_(*output_stream) {}

  ~GcsWalkWriter() { output_stream_.flush(); }

  // Writes out sentences to GCS storage.
  virtual void WriteSentence(const Sentence &sentence);

 private:
  // Mutex for locking when writing to GCS.
  tbb::mutex write_mutex_;
  // Output stream associated with the GCS file being written to.
  std::ostream &output_stream_;
};

// Spawns walk workers to perform the actual walks.
// State inside a walker is accessed concurrently.
class Walker {
 public:
  Walker();

  // Performs a random walk over a legacy ICFG and writes it to a file.
  grpc::Status RandomWalkLegacyIcfgBackground(
      const RandomWalkLegacyIcfgRequest *request);

  // Performs a random walk over a legacy ICFG and streams sentences
  // out via gRPC.
  grpc::Status RandomWalkLegacyIcfg(const RandomWalkLegacyIcfgRequest *request,
                                    grpc::ServerWriter<Sentence> *writer);

  // Performs a random walk over all labels in an Lpds object.
  grpc::Status RandomWalk(const Lpds *lpds, WalkWriter *writer,
                          int num_walks_per_label, int walk_length);

 private:
  // Reads a legacy func2vec ICFG and performs a random walk.
  // The output is written to `writer`.
  grpc::Status DoRandomWalkLegacyIcfg(
      const RandomWalkLegacyIcfgRequest *request, WalkWriter *writer);

  // icfg_path: path to file containing legacy func2vec getgraph output.
  // Returns true if parsing was successful
  grpc::Status ReadLegacyIcfg(const Uri &icfg_uri, Lpds *lpds) const;
};

// A walk worker performs a single random walk over all labels.
// State inside a walk worker is not accessed concurrently.
class WalkWorker {
 public:
  // lpds: the Lpds object to walk.
  // walk_number: which walk this is.
  WalkWorker(const Lpds *lpds, int walk_number, WalkWriter *writer)
      : lpds_(lpds),
        walk_number_(walk_number),
        writer_(writer),
        mersenne_twister_((std::random_device())()) {}

  // Performs a single random walk over all labels in lpds_.
  grpc::Status SingleRandomWalk(const tbb::concurrent_vector<Label> &labels,
                                int walk_length);

 private:
  // Random number generator.
  std::mt19937 mersenne_twister_;

  // Stack of return edges to visit.
  std::stack<const LpdsEdge *> return_stack_;

  // The Lpds structure being walked.
  const Lpds *lpds_;

  // Which walk number this is.
  const int walk_number_;

  // Output writer.
  WalkWriter *writer_;

  // Performs a random transition in the Lpds.
  const LpdsNode *RandomTransition(const LpdsNode *node, Sentence *sentence);

  // Helper for RandomTransition. Executed when transitioning from the
  // end of a function.
  const LpdsEdge *PopTransition(
      const LpdsNode *node, Sentence *sentence,
      std::vector<const LpdsEdge *> &may_return_edges);

  // Selects a label and adds it to the walk.
  void EmitLabel(const LpdsEdge *edge, Sentence *sentence);
};

}  // namespace error_specifications

#endif  // ERROR_SPECIFICATIONS_WALKER_INCLUDE_WALKER_H_
