"""Embeddding gRPC server."""

import argparse
from concurrent import futures
import hashlib
import sys
sys.path = [path for path in sys.path if "com_" not in path]

import glog as log
import grpc

import gensim.models

import proto.embedding_pb2
import proto.embedding_pb2_grpc
import proto.get_graph_pb2

def _read_walks_from_file(uri):
    """Read walks file from local machine.

    Arguments:
        uri: URI to local file.
    """

    with open(uri.path, 'r') as file:
        walks = [line.strip().split() for line in file]

    return walks

def _read_walks(uri):
    """Read in a walks file and returns the walks list."""

    if uri.scheme == proto.operations_pb2.Scheme.SCHEME_FILE:
        return _read_walks_from_file(uri)

    raise ValueError("Invalid scheme for uri.")

def _read_embedding_from_file(uri):
    """Read embedding from local storage."""

    embedding = gensim.models.KeyedVectors.load_word2vec_format(
        uri.path, binary=False)
    embedding_hash = _hash_file(uri.path)

    return embedding, embedding_hash

def _read_embedding(uri):
    """Reads in an embedding file and returns its ID and w2v representation."""

    if uri.scheme == proto.operations_pb2.Scheme.SCHEME_FILE:
        return _read_embedding_from_file(uri)

    raise ValueError("Invalid scheme for embedding_uri.")

def _write_embedding(embedding, output_embedding_uri):
    """Persists a embedding to output_uri.

    Returns:
        sha256 hash of the embedding file.
    """

    if output_embedding_uri.scheme == proto.operations_pb2.Scheme.SCHEME_FILE:
        embedding.wv.save_word2vec_format(
            output_embedding_uri.path, binary=False)
        return _hash_file(output_embedding_uri.path)

    raise ValueError("Invalid scheme for output_embedding_uri.")

def _hash_file(path):
    """Returns the SHA256 hash of the file."""

    file_hash = hashlib.sha256()
    with open(path, 'rb') as file:
        file_hash.update(file.read())

    return file_hash.hexdigest()

class EmbeddingServiceServicer(
        proto.embedding_pb2_grpc.EmbeddingServiceServicer):
    """Provides methods that implement functionality of embedding server."""

    # Mapping from EmbeddingMethod to appropriate gensim embedding method.
    EMBEDDING_METHOD_TO_GENSIM = {
        proto.embedding_pb2.EmbeddingMethod.EMBEDDING_METHOD_WORD2VEC:
            gensim.models.Word2Vec,
        proto.embedding_pb2.EmbeddingMethod.EMBEDDING_METHOD_FASTTEXT:
            gensim.models.FastText,
        proto.embedding_pb2.EmbeddingMethod.EMBEDDING_METHOD_INVALID:
            None,
    }

    def __init__(self):
        # Map from embedding IDs to the actual embeddings. This map is populated
        # by RegisterEmbedding.
        self.registered_embeddings = dict()

    def Train(self, request, context):
        """Trains a embedding based on the walks file from the request."""

        log.info("Request: Train {}".format(request.random_walks_uri))

        # If the walk file cannot be read, set status to NOT_FOUND.
        try:
            # Read the walks.
            walks_list = _read_walks(request.random_walks_uri)
        except OSError:
            context.set_code(grpc.StatusCode.NOT_FOUND)
            return proto.embedding_pb2.TrainResponse()
        except ValueError:
            context.set_code(grpc.StatusCode.INVALID_ARGUMENT)
            return proto.embedding_pb2.TrainResponse()

        embedding_method = self.EMBEDDING_METHOD_TO_GENSIM[
            request.embedding_method]
        if not embedding_method:
            # Only W2V and fastText are supported ATM. Any other option
            # (EMBEDDING_METHOD_INVALID) results in an INVALID_ARGUMENT status
            # code.
            context.set_code(grpc.StatusCode.INVALID_ARGUMENT)
            return proto.embedding_pb2.TrainResponse()

        embedding = embedding_method(
            walks_list,
            window=request.window,
            min_count=request.mincount,
            vector_size=request.dimensions,
        )
        
        # Write out the trained embedding.
        file_hash = _write_embedding(embedding, request.output_embedding_uri)

        # Automatically "registering" embedding with the service after training.
        self.registered_embeddings[file_hash] = embedding

        response = proto.embedding_pb2.TrainResponse(
            embedding_id=proto.operations_pb2.Handle(id=file_hash)
        )

        log.info("Training finished: {}".format(file_hash))

        return response

    def RegisterEmbedding(self, request, context):
        """Registers a w2v embedding from the request."""

        log.info("Request: RegisterEmbedding {}".format(request.uri))

        # If the embedding file cannot be found, set status to NOT_FOUND.
        try:
            embedding, embedding_hash = _read_embedding(request.uri)
        except OSError:
            context.set_code(grpc.StatusCode.NOT_FOUND)
            return proto.embedding_pb2.RegisterEmbeddingResponse()
        except ValueError:
            context.set_code(grpc.StatusCode.INVALID_ARGUMENT)
            return proto.embedding_pb2.RegisterEmbeddingResponse()

        if embedding_hash not in self.registered_embeddings:
            self.registered_embeddings[embedding_hash] = embedding

        response = proto.embedding_pb2.RegisterEmbeddingResponse(
            embedding_id=proto.operations_pb2.Handle(id=embedding_hash),
        )

        log.info("Registration Finished: {}".format(embedding_hash))

        return response

    def GetSimilarity(self, request, context):
        """Get the similarity between two labels in request."""

        log.info("Request: Similarity between {} and {}".format(
            request.label1.label, request.label2.label))

        # If the request embedding id is not found in the registered embeddings
        # return an empty response and set status to NOT_FOUND.
        try:
            embedding = self.registered_embeddings[request.embedding_id.id]
        except KeyError:
            context.set_code(grpc.StatusCode.NOT_FOUND)
            return proto.embedding_pb2.GetSimilarityResponse()

        similarity = embedding.similarity(
            request.label1.label, request.label2.label)

        response = proto.embedding_pb2.GetSimilarityResponse(
            similarity=similarity
        )

        log.info("Calculated similarity: {}".format(similarity))

        return response

    def GetMostSimilar(self, request, context):
        """Get the top-k most similar labels to label in request."""

        log.info("Request: Top-{} similar to {}".format(
            request.top_k, request.label.label))

        # If the request embedding id is not found in the registered embeddings
        # return an empty response and set status to NOT_FOUND.
        try:
            embedding = self.registered_embeddings[request.embedding_id.id]
        except KeyError:
            context.set_code(grpc.StatusCode.NOT_FOUND)
            return proto.embedding_pb2.GetMostSimilarResponse()

        try:
            most_similar = embedding.most_similar(
                positive=request.label.label, topn=request.top_k)
        except AttributeError:
            most_similar = embedding.wv.most_similar(
                positive=request.label.label, topn=request.top_k)

        trimmed_most_similar = list()
        for label, similarity_measure in most_similar:
            if "F2V" in label:
                continue
            trimmed_most_similar.append((label, similarity_measure))
            if len(trimmed_most_similar) >= request.top_k:
                break


        label_messages = []
        similarities = []
        for label, similarity_measure in trimmed_most_similar:
            label_messages.append(proto.get_graph_pb2.Label(
                label=label))
            similarities.append(similarity_measure)

        response = proto.embedding_pb2.GetMostSimilarResponse(
            labels=label_messages,
            similarities=similarities
        )

        log.info("Top-{}: {}".format(request.top_k,
                                     [sim for sim, _ in most_similar]))

        return response

    def GetVocabulary(self, request, context):
        """Get the vocabulary for the supplied embedding ID in request."""

        log.info("Request: GetVocabulary-{}".format(request.embedding_id.id))

        try:
            embedding = self.registered_embeddings[request.embedding_id.id]
        except KeyError:
            context.set_code(grpc.StatusCode.NOT_FOUND)
            return proto.embedding_pb2.GetVocabularyResponse()

        # Depending on how the model is loaded in the embedding object may
        # not have the attribute wv.
        try:
            response = proto.embedding_pb2.GetVocabularyResponse(
                function_labels=list(embedding.index_to_key))
        except AttributeError:
            response = proto.embedding_pb2.GetVocabularyResponse(
                function_labels=list(embedding.wv.index_to_key))


        log.info("GetVocabulary-{} finished!".format(request.embedding_id.id))

        return response

def serve():
    """Starts the gRPC service."""

    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--listen",
        default="localhost:50056",
        help="The host address and port number in the"
             " form <host_address>:<port_number>",
    )
    args = parser.parse_args()

    server = grpc.server(futures.ThreadPoolExecutor(max_workers=10))
    proto.embedding_pb2_grpc.add_EmbeddingServiceServicer_to_server(
        EmbeddingServiceServicer(), server)
    server.add_insecure_port(args.listen)
    print("Listening on {}".format(args.listen))
    server.start()
    server.wait_for_termination()

if __name__ == "__main__":
    serve()
