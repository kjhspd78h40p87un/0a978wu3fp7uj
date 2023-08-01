#include "synonym_finder.h"

#include "glog/logging.h"
#include "include/grpcpp/grpcpp.h"
#include "proto/eesi.grpc.pb.h"

namespace error_specifications {

SynonymFinder::SynonymFinder(
    const Handle &embedding_id,
    const ExpansionOperationType &expansion_operation) {
  // Check that the authority section of the model ID is not empty.
  if (embedding_id.authority().empty()) {
    // const std::string &err_msg = "Model ID must contain an authority.";
    // LOG(ERROR) << err_msg;
    stub_ = nullptr;
    return;
  }

  std::shared_ptr<grpc::Channel> channel;
  channel = grpc::CreateChannel(embedding_id.authority(),
                                grpc::InsecureChannelCredentials());

  if (!channel) {
    // const std::string &err_msg = "Unable to connect to embedding service.";
    // LOG(ERROR) << err_msg;
    stub_ = nullptr;
    return;
  }
  stub_ = EmbeddingService::NewStub(channel);
  embedding_id_ = embedding_id;

  switch (expansion_operation) {
    case ExpansionOperationType::EXPANSION_OPERATION_INVALID:
      return;
    case ExpansionOperationType::EXPANSION_OPERATION_MEET:
      expansion_operation_ = &ConfidenceLattice::MeetOnVector;
      break;
    case ExpansionOperationType::EXPANSION_OPERATION_JOIN:
      expansion_operation_ = &ConfidenceLattice::JoinOnVector;
      break;
    case ExpansionOperationType::EXPANSION_OPERATION_MAX:
      expansion_operation_ = &ConfidenceLattice::KeepHighest;
      break;
  }
}

std::vector<std::pair<std::string, float>> SynonymFinder::GetSynonyms(
    const std::string &function_name, int k, float threshold) {
  if (!stub_) return std::vector<std::pair<std::string, float>>();

  GetMostSimilarRequest request;
  request.mutable_embedding_id()->CopyFrom(embedding_id_);
  request.mutable_label()->set_label(function_name);
  request.set_top_k(k);

  GetMostSimilarResponse response;
  grpc::ClientContext context;
  grpc::Status status = stub_->GetMostSimilar(&context, request, &response);
  if (!status.ok()) {
    // This happens when a label does not exist in the model, which
    // can happen frequently (e.g. the function is never called).
    LOG(WARNING) << status.error_message();
    return std::vector<std::pair<std::string, float>>();
  }
  std::vector<std::pair<std::string, float>> func_synonyms;
  // Filter out functions that are less than the threshold.
  for (auto i = 0; i < response.labels_size(); i++) {
    // if (response.similarities(i) >= threshold) {
    func_synonyms.push_back(
        std::make_pair(response.labels(i).label(), response.similarities(i)));
    //}
  }
  return func_synonyms;
}

std::vector<std::string> SynonymFinder::GetVocabulary() {
  if (!stub_) return std::vector<std::string>();

  GetVocabularyRequest request;
  request.mutable_embedding_id()->CopyFrom(embedding_id_);

  GetVocabularyResponse response;
  grpc::ClientContext context;
  grpc::Status status = stub_->GetVocabulary(&context, request, &response);

  // This call should never fail as GetVocabulary is only called when we have
  // a valid SynonymFinder that has a registered embedding.
  assert(status.ok());

  return std::vector<std::string>(response.function_labels().begin(),
                                  response.function_labels().end());
}

}  // namespace error_specifications
