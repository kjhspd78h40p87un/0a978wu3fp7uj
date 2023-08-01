"""For storing and retrieving embedding information from MongoDB."""

import glog as log

import cli.db.db
import cli.common.uri

def insert_embedding(database, bitcode_id, embedding_id, embedding_uri):
    """Inserts a embedding URI associated with a bitcode ID into MongoDB.

    Args:
        database: Pymongo database object used for storing embedding info.
        bitcode_id: SHA256 of the original bitcode file (NOT EMBEDDING).
        embedding_id: SHA256 of the model file.
        embedding_uri: URI protobuf message representing the model file's URI.
    """
    database.embedding.delete_many({"bitcode_id": bitcode_id})

    embedding_uri = cli.common.uri.to_str(embedding_uri)
    database.embeddings.insert_one({
        "bitcode_id": bitcode_id,
        "embedding_id": embedding_id,
        "embedding_uri": embedding_uri,})
    log.info("Embedding URI inserted into database, associated with bitcode ID {}"
             .format(bitcode_id))

def read_embedding_uri(database, bitcode_id):
    """Returns a URI message for a model file given a bitcode ID."""

    entry = database.embeddings.find_one({"bitcode_id": bitcode_id})
    if not entry:
        return None

    return cli.common.uri.parse(entry["embedding_uri"])
