"""Commands related to interacting with the embedding service."""

import glog as log

import cli.common.uri
import cli.embedding.db
import cli.embedding.rpc
import cli.bitcode.db
import proto.embedding_pb2

STRING_TO_EMBEDDING_METHOD = {
    "w2v": proto.embedding_pb2.EmbeddingMethod.EMBEDDING_METHOD_WORD2VEC,
    "fasttext": proto.embedding_pb2.EmbeddingMethod.EMBEDDING_METHOD_FASTTEXT,
}

#TODO (): In Embedding CLI PR utilize overwrite.
def register_embedding(database, bitcode_uri, embedding_uri,
                       service_configuration_handler,
                       overwrite,):
    """Registers the embedding and stores the bitcode ID mapping to URI."""

    log.info("Register embedding {} for bitcode {}".format(
        bitcode_uri, embedding_uri))

    bitcode_uri = cli.common.uri.parse(bitcode_uri)
    bitcode_id = cli.bitcode.db.read_id_for_uri(database, bitcode_uri)
    embedding_uri = cli.common.uri.parse(embedding_uri)

    embedding_id = cli.embedding.rpc.register_embedding(
        service_configuration_handler.get_embedding_stub(),
        proto.embedding_pb2.RegisterEmbeddingRequest(uri=embedding_uri))

    cli.embedding.db.insert_embedding(
        database, bitcode_id, embedding_id.id, embedding_uri)

    log.info("RegisterEmbedding finished!")

def train(database, walks_uri, output_uri, dimensions, window, mincount,
          embedding_method, service_configuration_handler):
    """Trains an embedding representation of functions for the bitcode file."""

    log.info("Training embedding for walks file {}.".format(
        walks_uri))

    walks_uri_message = cli.common.uri.parse(walks_uri)
    output_uri_message = cli.common.uri.parse(output_uri)

    embedding_request = proto.embedding_pb2.TrainRequest(
        random_walks_uri=walks_uri_message,
        output_embedding_uri=output_uri_message,
        dimensions=dimensions,
        window=window,
        mincount=mincount,
        embedding_method=STRING_TO_EMBEDDING_METHOD[embedding_method],
    )

    cli.embedding.rpc.train_embedding(
        service_configuration_handler.get_embedding_stub(),
        embedding_request,
    )

    log.info(f"Train finished for {walks_uri}!")
