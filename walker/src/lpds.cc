#include "lpds.h"

#include <memory>
#include <vector>

#include "glog/logging.h"

namespace error_specifications {

Lpds::~Lpds() {
  for (const LpdsEdge *edge : edges_) {
    delete edge;
  }

  for (auto &kv : graph_) {
    delete kv.first;
  }
}

const LpdsNode *Lpds::AddNode(const LpdsNode &node) {
  assert(!node.name.empty());

  if (node_index_.find(node) != node_index_.end()) {
    return node_index_.at(node);
  }

  // Always creates a node. The pointer to the node servers as unique
  // identifier.
  LpdsNode *graph_node = new LpdsNode(node.name);

  // Insert into graph.
  graph_[graph_node] = tbb::concurrent_unordered_set<const LpdsEdge *>();

  // Insert into node index.
  node_index_[node] = graph_node;

  // Return a pointer to the existing or created node.
  return graph_node;
}

const LpdsEdge *Lpds::AddEdge(LpdsEdge edge) {
  assert(edge.source != nullptr);
  assert(edge.target != nullptr);

  // TODO: copy constructor for LpdsEdge
  LpdsEdge *added_edge = new LpdsEdge;
  added_edge->source = edge.source;
  added_edge->target = edge.target;
  added_edge->data = edge.data;

  // Add to label index.
  for (const Label &label : edge.data.labels) {
    label_to_edges_[label].insert(added_edge);
  }

  // Add node to neighbors in graph.
  auto graph_node_it = graph_.find(edge.source);
  assert(graph_node_it != graph_.end());
  tbb::concurrent_unordered_set<const LpdsEdge *> out_edges =
      graph_node_it->second;
  out_edges.insert(added_edge);
  graph_[edge.source] = out_edges;

  // Add to edges index.
  edges_.insert(added_edge);

  return added_edge;
}

tbb::concurrent_vector<const LpdsEdge *> Lpds::GetOutEdges(
    const LpdsNode *node) const {
  tbb::concurrent_vector<const LpdsEdge *> ret;

  for (const LpdsEdge *edge : graph_.at(node)) {
    ret.push_back(edge);
  }

  return ret;
}

tbb::concurrent_vector<Label> Lpds::GetLabels() const {
  tbb::concurrent_vector<Label> labels;

  for (const auto &label : label_to_edges_) {
    labels.push_back(label.first);
  }

  return labels;
}

tbb::concurrent_vector<const LpdsEdge *> Lpds::GetEdgesForLabel(
    const Label &label) const {
  tbb::concurrent_vector<const LpdsEdge *> ret;
  for (const LpdsEdge *edge : label_to_edges_.at(label)) {
    ret.push_back(edge);
  }
  return ret;
}

int Lpds::GetNodeCount() const { return graph_.size(); }

int Lpds::GetEdgeCount() const { return edges_.size(); }

}  // namespace error_specifications
