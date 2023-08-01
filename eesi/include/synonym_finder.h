#ifndef ERROR_SPECIFICATIONS_EESI_INCLUDE_SYNONYM_FINDER_H_
#define ERROR_SPECIFICATIONS_EESI_INCLUDE_SYNONYM_FINDER_H_

#include <memory>
#include <string>
#include <vector>

#include "eesi/include/confidence_lattice.h"
#include "proto/embedding.grpc.pb.h"

namespace error_specifications {

// SynonymFinder class uses a function embedding to find function synonyms.
class SynonymFinder {
 public:
  SynonymFinder(const Handle &embedding_model_id,
                const ExpansionOperationType &expansion_operation);
  SynonymFinder() {}
  virtual ~SynonymFinder() {}

  // Returns at most k (synonym, similarity) pairs whose similarity with the
  // given function is above the given threshold.
  virtual std::vector<std::pair<std::string, float>> GetSynonyms(
      const std::string &function_name, int k, float threshold);

  // Returns the vocabulary for the embedding that is associated with the
  // SynonymFinder. The vocabulary in this case is the list of functions that
  // are represented in the embedding.
  virtual std::vector<std::string> GetVocabulary();

  virtual LatticeElementConfidence ExpansionOperation(
      std::vector<LatticeElementConfidence> lattice_element_confidences) {
    return expansion_operation_(lattice_element_confidences);
  }

 private:
  Handle embedding_id_;
  std::unique_ptr<EmbeddingService::Stub> stub_;
  LatticeElementConfidence (*expansion_operation_)(
      const std::vector<LatticeElementConfidence> &lattice_element_confidences);
};

}  // namespace error_specifications

#endif  // ERROR_SPECIFICATIONS_EESI_INCLUDE_SYNONYM_FINDER_H_
