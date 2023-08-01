// Tests for the random walker.
// These tests are technically flaky, but they fail with extremely low
// probability.

#include "glog/logging.h"
#include "gtest/gtest.h"

#include "walker.h"

namespace error_specifications {

class VectorWalkWriter : public WalkWriter {
 public:
  virtual void WriteSentence(const Sentence &sentence) {
    tbb::mutex::scoped_lock lock(write_mutex_);
    sentences_.push_back(sentence);
  }

  std::vector<Sentence> GetSentences() const { return sentences_; }

 private:
  tbb::mutex write_mutex_;
  std::vector<Sentence> sentences_;
};

bool LabelInSentence(const Label &label, const Sentence &sentence) {
  for (const auto &sl : sentence.words()) {
    if (sl.label() == label.label()) {
      return true;
    }
  }
  return false;
}

// Tests that a straight-line walk ends at walk length.
TEST(WalkerTest, WalkLength) {
  Lpds lpds;

  const LpdsNode *a = lpds.AddNode(LpdsNode("a"));
  const LpdsNode *b = lpds.AddNode(LpdsNode("b"));
  const LpdsNode *c = lpds.AddNode(LpdsNode("c"));
  const LpdsNode *d = lpds.AddNode(LpdsNode("d"));

  LpdsEdgeData start_data;
  Label start;
  start.set_label("start");
  start_data.labels.push_back(start);

  LpdsEdgeData other_data;
  Label other;
  other_data.labels.push_back(other);

  lpds.AddEdge(LpdsEdge(a, b, &start_data));
  lpds.AddEdge(LpdsEdge(b, c, &other_data));
  lpds.AddEdge(LpdsEdge(c, d, &other_data));

  VectorWalkWriter writer;

  Walker walker;
  walker.RandomWalk(&lpds, &writer, 1, 2);

  EXPECT_EQ(writer.GetSentences().size(), 2);
}

TEST(WalkerTest, RandomTransition) {
  Lpds lpds;

  const LpdsNode *a = lpds.AddNode(LpdsNode("a"));
  const LpdsNode *b = lpds.AddNode(LpdsNode("b"));
  const LpdsNode *c = lpds.AddNode(LpdsNode("c"));

  LpdsEdgeData ab_data;
  Label ab;
  ab.set_label("ab");
  ab_data.labels.push_back(ab);

  LpdsEdgeData ac_data;
  Label ac;
  ac.set_label("ac");
  ac_data.labels.push_back(ac);

  lpds.AddEdge(LpdsEdge(a, b, &ab_data));
  lpds.AddEdge(LpdsEdge(a, c, &ac_data));

  VectorWalkWriter writer;

  Walker walker;
  walker.RandomWalk(&lpds, &writer, 100, 2);

  auto sentences = writer.GetSentences();
  bool found_ab = false;
  bool found_ac = false;
  for (const auto &sentence : sentences) {
    if (LabelInSentence(ab, sentence)) {
      found_ab = true;
    }
    if (LabelInSentence(ab, sentence)) {
      found_ac = true;
    }
  }

  EXPECT_TRUE(found_ab);
  EXPECT_TRUE(found_ac);
}

// Tests that calls return only to the appropriate return site.
TEST(WalkerTest, ContextSensitivity) {
  Lpds lpds;

  const LpdsNode *call1 = lpds.AddNode(LpdsNode("call1"));
  const LpdsNode *call2 = lpds.AddNode(LpdsNode("call2"));
  const LpdsNode *foo = lpds.AddNode(LpdsNode("foo"));
  const LpdsNode *ret1 = lpds.AddNode(LpdsNode("ret1"));
  const LpdsNode *ret2 = lpds.AddNode(LpdsNode("fet2"));
  const LpdsNode *after_ret1 = lpds.AddNode(LpdsNode("after_ret1"));
  const LpdsNode *after_ret2 = lpds.AddNode(LpdsNode("after_ret2"));

  LpdsEdgeData call1_to_foo_data;
  call1_to_foo_data.is_call = true;

  LpdsEdgeData call2_to_foo_data;
  call2_to_foo_data.is_call = true;

  LpdsEdgeData foo_to_ret1_data;
  foo_to_ret1_data.is_may_return = true;

  LpdsEdgeData foo_to_ret2_data;
  foo_to_ret2_data.is_may_return = true;

  LpdsEdgeData call1_to_ret1_data;
  Label call1_to_ret1_label;
  call1_to_ret1_label.set_label("foo");
  call1_to_ret1_data.labels.push_back(call1_to_ret1_label);

  LpdsEdgeData call2_to_ret2_data;
  Label call2_to_ret2_label;
  call2_to_ret2_label.set_label("foo");
  call2_to_ret2_data.labels.push_back(call2_to_ret2_label);

  LpdsEdgeData ret1_to_after_ret1_data;
  Label ret1_to_after_ret1_label;
  ret1_to_after_ret1_label.set_label("x");
  ret1_to_after_ret1_data.labels.push_back(ret1_to_after_ret1_label);

  LpdsEdgeData ret2_to_after_ret2_data;
  Label ret2_to_after_ret2_label;
  ret2_to_after_ret2_label.set_label("y");
  ret2_to_after_ret2_data.labels.push_back(ret2_to_after_ret2_label);

  lpds.AddEdge(LpdsEdge(call1, foo, &call1_to_foo_data));
  lpds.AddEdge(LpdsEdge(call2, foo, &call2_to_foo_data));
  lpds.AddEdge(LpdsEdge(call1, ret1, &call1_to_ret1_data));
  lpds.AddEdge(LpdsEdge(call2, ret2, &call2_to_ret2_data));
  lpds.AddEdge(LpdsEdge(foo, ret1, &foo_to_ret1_data));
  lpds.AddEdge(LpdsEdge(foo, ret2, &foo_to_ret2_data));
  lpds.AddEdge(LpdsEdge(ret1, after_ret1, &ret1_to_after_ret1_data));
  lpds.AddEdge(LpdsEdge(ret2, after_ret2, &ret2_to_after_ret2_data));

  VectorWalkWriter writer;

  Walker walker;
  walker.RandomWalk(&lpds, &writer, 100, 100);

  auto sentences = writer.GetSentences();
  bool have_ret1 = false;
  bool have_ret2 = false;
  for (const auto &sentence : sentences) {
    if (LabelInSentence(ret1_to_after_ret1_label, sentence)) {
      have_ret1 = true;
      EXPECT_FALSE(LabelInSentence(ret2_to_after_ret2_label, sentence));
    }
    if (LabelInSentence(ret2_to_after_ret2_label, sentence)) {
      have_ret2 = true;
      EXPECT_FALSE(LabelInSentence(ret1_to_after_ret1_label, sentence));
    }
  }

  EXPECT_TRUE(have_ret1);
  EXPECT_TRUE(have_ret2);
}

// Tests that vertices with one outgoing edge correctly emit labels while
// performing a walk. This is especially important for correctly emitting return
// instruction labels, as they will only have one incoming edge.
TEST(WalkerTest, SingleOutgoingEdge) {
  Lpds lpds;

  // This doesn't necessarily need to mock starting from a function name, we
  // only care about the transitions. We also need to ensure that we have at
  // least two edges, since the walker will always start a walk from each label
  // regardless of its position.
  const LpdsNode *entry_node = lpds.AddNode(LpdsNode("entry"));
  const LpdsNode *next_node = lpds.AddNode(LpdsNode("next"));
  const LpdsNode *final_node = lpds.AddNode(LpdsNode("final"));

  LpdsEdgeData entry_to_next_data;
  Label entry_to_next_label;
  entry_to_next_label.set_label("NEXT");
  entry_to_next_data.labels.push_back(entry_to_next_label);

  LpdsEdgeData next_to_final_data;
  Label next_to_final_label;
  next_to_final_label.set_label("FINAL");
  next_to_final_data.labels.push_back(next_to_final_label);

  lpds.AddEdge(LpdsEdge(entry_node, next_node, &entry_to_next_data));
  lpds.AddEdge(LpdsEdge(next_node, final_node, &next_to_final_data));

  VectorWalkWriter writer;

  Walker walker;
  walker.RandomWalk(&lpds, &writer, 1, 100);

  auto sentences = writer.GetSentences();
  bool have_next_and_final = false;
  for (const auto &sentence : sentences) {
    // We need at least one of the sentences to include both labels.
    if (LabelInSentence(entry_to_next_label, sentence) &&
        LabelInSentence(next_to_final_label, sentence)) {
      have_next_and_final = true;
    }
  }

  EXPECT_TRUE(have_next_and_final);
}

}  // namespace error_specifications
