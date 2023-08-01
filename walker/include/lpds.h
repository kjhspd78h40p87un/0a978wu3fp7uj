// A node may have many outgoing call edges. The purpose of these edges is to
// model indirect calls.
//
// A node may have many outgoing may_return edges. The purpose of these edges
// is to indicate the possible return sites from a function.
//
// A node must only have one outgoing internal edge (i.e. neither call nor
// may return).

#ifndef ERROR_SPECIFICATIONS_WALKER_LPDS_H_
#define ERROR_SPECIFICATIONS_WALKER_LPDS_H_

#include <memory>
#include <string>

#include "tbb/tbb.h"

#include "proto/walker.grpc.pb.h"

namespace error_specifications {

namespace {
struct LabelHash {
  size_t operator()(const Label &label) const {
    return std::hash<std::string>()(label.label());
  }
};

struct LabelCompare {
  bool operator()(const Label &lhs, const Label &rhs) const {
    return lhs.label() == rhs.label();
  }
};
}  // namespace

// A node in an Lpds.
struct LpdsNode {
  LpdsNode(const std::string &name) : name(name) {}
  std::string name;
};

// For putting nodes into maps.
struct LpdsNodeHash {
  size_t operator()(const LpdsNode &node) const {
    return std::hash<std::string>()(node.name);
  }
};

// For putting nodes into maps.
struct LpdsNodeCompare {
  bool operator()(const LpdsNode &lhs, const LpdsNode &rhs) const {
    return lhs.name == rhs.name;
  }
};

// Data that is associated with an edge.
struct LpdsEdgeData {
  // The labels for this edge.
  tbb::concurrent_vector<Label> labels;

  // True only if this edge represents a call into a function.
  bool is_call = false;

  // True only if this edge reprents a possible return from a function.
  bool is_may_return = false;
};

struct LpdsEdge {
  LpdsEdge() {}
  LpdsEdge(const LpdsNode *source, const LpdsNode *target, LpdsEdgeData *data)
      : source(source), target(target), data(*data) {}

  const LpdsNode *source = nullptr;
  const LpdsNode *target = nullptr;
  LpdsEdgeData data;
};

// An Lpds (Labeled Pushdown System) is a graph with labeled edges and the
// exposes operations that facilitate random walks. The actual stack associated
// with transitions is maintained by the walker itself.
class Lpds {
 public:
  ~Lpds();

  // Creates a node in the graph and returns a pointer to it.
  const LpdsNode *AddNode(const LpdsNode &node);

  // Nodes must exist before adding edge (or an assert wil fail).
  const LpdsEdge *AddEdge(LpdsEdge edge);

  // Returns all of the labels in the Lpds.
  tbb::concurrent_vector<Label> GetLabels() const;

  // Return the out edges of `node`.
  tbb::concurrent_vector<const LpdsEdge *> GetOutEdges(
      const LpdsNode *node) const;

  // Return all of the edges that are labeled with `label`.
  tbb::concurrent_vector<const LpdsEdge *> GetEdgesForLabel(
      const Label &label) const;

  // Return the total number of nodes.
  int GetNodeCount() const;

  // Return the total number of edges.
  int GetEdgeCount() const;

 private:
  // The node_index_ prevents adding duplicate nodes. If the same node is
  // added twice (as determined by LpdsNodeHash), then the second addition
  // is a no-op.
  tbb::concurrent_unordered_map<LpdsNode, const LpdsNode *, LpdsNodeHash,
                                LpdsNodeCompare>
      node_index_;

  // A map from node names to adjacent nodes.
  tbb::concurrent_unordered_map<const LpdsNode *,
                                tbb::concurrent_unordered_set<const LpdsEdge *>>
      graph_;

  // List of edges in the graph.
  tbb::concurrent_unordered_set<const LpdsEdge *> edges_;

  // A map from Labels to the edges that are labeled with that label.
  // Used by `GetEdgesForLabel`.
  tbb::concurrent_unordered_map<Label,
                                tbb::concurrent_unordered_set<const LpdsEdge *>,
                                LabelHash, LabelCompare>
      label_to_edges_;
};

}  // namespace error_specifications

#endif ERROR_SPECIFICATIONS_WALKER_LPDS_H_
