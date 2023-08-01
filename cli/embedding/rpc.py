""" Interacts with the embedding RPC service."""

import functools
import sys

import glog as log

import cli.embedding.db
import cli.operations.wait

#TODO: Utilize wait_for_operations() eventually?
def register_embedding(
        embedding_stub, register_embedding_request):
    """Interacts with the RegisterEmbedding gRPC command."""

    register_embedding_response = embedding_stub.RegisterEmbedding(
        register_embedding_request)
    embedding_id = register_embedding_response.embedding_id

    return embedding_id

def train_embedding(embedding_stub, train_request):
    """Interacts with the Train gRPC request."""

    embedding_id = None
    try:
        embedding_id = embedding_stub.Train(train_request)
    except Exception as e:
        log.error("Error: Training failed!")
        log.error(f"Exception: {e}")
        sys.exit(1)

    return embedding_id

def get_most_similar(
        embedding_stub, get_most_similar_request):
    """Interacts with the GetMostSimilar gRPC command."""

    get_most_similar_response = embedding_stub.GetMostSimilar(
        get_most_similar_request)

    return get_most_similar_response
