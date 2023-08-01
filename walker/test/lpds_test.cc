#include "glog/logging.h"
#include "gtest/gtest.h"

#include "lpds.h"

namespace error_specifications {

TEST(LpdsTest, AddEdge) {
  Lpds lpds;
  LpdsNode x("x");
  LpdsNode y("y");
  const LpdsNode *source_node = lpds.AddNode(x);
  const LpdsNode *target_node = lpds.AddNode(y);
  LpdsEdge edge_to_add;
  edge_to_add.source = source_node;
  edge_to_add.target = target_node;
  lpds.AddEdge(edge_to_add);

  ASSERT_EQ(lpds.GetNodeCount(), 2);
  ASSERT_EQ(lpds.GetEdgeCount(), 1);

  auto out_edges = lpds.GetOutEdges(target_node);
  ASSERT_EQ(out_edges.size(), 0);

  out_edges = lpds.GetOutEdges(source_node);
  ASSERT_EQ(out_edges.size(), 1);

  ASSERT_EQ(out_edges[0]->target, target_node);
}

// Test the assertion that nodes are created before edges.
TEST(LpdsTest, NodesBeforeEdges) {
  Lpds lpds;

  LpdsNode x("x");
  LpdsNode y("y");
  const LpdsNode *source_node = &x;
  const LpdsNode *target_node = &y;

  LpdsEdgeData data;
  LpdsEdge edge(source_node, target_node, &data);

  EXPECT_DEATH(lpds.AddEdge(edge), "");
}

// An Lpds can be a Multigraph.
TEST(LpdsTest, MultiGraph) {
  Lpds lpds;

  LpdsNode x("x");
  LpdsNode y("y");
  const LpdsNode *source_node = lpds.AddNode(x);
  const LpdsNode *target_node = lpds.AddNode(y);
  LpdsEdge edge_to_add;
  edge_to_add.source = source_node;
  edge_to_add.target = target_node;
  lpds.AddEdge(edge_to_add);
  lpds.AddEdge(edge_to_add);

  ASSERT_EQ(lpds.GetEdgeCount(), 2);

  auto out_edges = lpds.GetOutEdges(target_node);
  ASSERT_EQ(out_edges.size(), 0);

  out_edges = lpds.GetOutEdges(source_node);
  ASSERT_EQ(out_edges.size(), 2);

  ASSERT_EQ(out_edges[0]->target, target_node);
}

}  // namespace error_specifications
