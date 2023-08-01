"""Handles arguments related to interacting with the EESI service."""

import sys

import glog as log

import cli.db.db
import cli.common.service_configuration_handler as service_handler
import cli.embedding.commands

def add_arguments(service_parsers):
    """Adds all command-line arguments for embeddings to main CLI service parser."""
    # Embedding service
    embedding_service_parsers = service_parsers.add_parser(
        "embedding",
        help="Commands for RPC calls to the embedding service",
    )
    embedding_parser = embedding_service_parsers.add_subparsers(
        title="command", dest='command')
    embedding_parser.required = True

    ## Embedding service: RegisterEmbeddding
    embedding_register_embedding_parser = embedding_parser.add_parser(
        "RegisterEmbedding", help='RegisterEmbedding RPC call')
    embedding_register_embedding_parser.add_argument(
        "--embedding-uri",
        help="Uri of the embedding file.",
        required=True,
    )
    embedding_register_embedding_parser.add_argument(
        "--bitcode-uri",
        default="Uri of the bitcode file.",
        required=True,
    )
    embedding_register_embedding_parser.add_argument(
        "--overwrite",
        default=False,
        action="store_true",
        help="Overwrite embedding registration entry in MongoDB."
    )

    ## Embedding service: Train
    embedding_train_parser = embedding_parser.add_parser(
        "Train", help="Train gRPC call")
    embedding_train_parser.add_argument(
        "--walks-uri",
        help="URI of random walks file.",
        required=True
    )
    embedding_train_parser.add_argument(
        "--output-uri",
        help="URI of where to save the embedding.",
        required=True,
    )
    embedding_train_parser.add_argument(
        "--dimensions",
        default=300,
        type=int,
    )
    embedding_train_parser.add_argument(
        "--window",
        default=1,
        type=int,
    )
    embedding_train_parser.add_argument(
        "--mincount",
        default=5,
        type=int,
    )
    embedding_train_parser.add_argument(
        "--method",
        choices=["w2v", "fasttext"],
        default="w2v",
        type=str.lower,
    )

def parse_args(args):
    """Calls the argument parsing function related to args.rpc."""

    assert args.service.lower() == "embedding"

    parser_function = dict({
        "registerembedding": parse_embedding_register_embedding_args,
        "train": parse_embedding_train_args,
    })

    try:
        return parser_function[args.command.lower()](args)
    except KeyError:
        log.error("Incorrect command {} provided!".format(args.command))
        sys.exit(1)

def parse_embedding_register_embedding_args(args):
    """Parses command-line arguments for GetSpecificationsAll."""

    database = cli.db.db.connect(args.db_name, args.db_host, args.db_port)
    service_configuration_handler = service_handler.ServiceConfigurationHandler(
        embedding_address=args.embedding_address,
        embedding_port=args.embedding_port,)

    command = cli.embedding.commands.register_embedding
    command_kwargs = dict()
    command_kwargs["database"] = database
    command_kwargs["service_configuration_handler"] = \
        service_configuration_handler
    command_kwargs["embedding_uri"] = args.embedding_uri
    command_kwargs["bitcode_uri"] = args.bitcode_uri
    command_kwargs["overwrite"] = args.overwrite

    return command, command_kwargs

def parse_embedding_train_args(args):
    """Parses command-line arguments for Train."""

    database = cli.db.db.connect(args.db_name, args.db_host, args.db_port)
    service_configuration_handler = service_handler.ServiceConfigurationHandler(
        embedding_address=args.embedding_address,
        embedding_port=args.embedding_port,)

    command = cli.embedding.commands.train
    command_kwargs = dict()
    command_kwargs["database"] = database
    command_kwargs["service_configuration_handler"] = \
        service_configuration_handler
    command_kwargs["walks_uri"] = args.walks_uri
    command_kwargs["output_uri"] = args.output_uri
    command_kwargs["embedding_method"] = args.method
    command_kwargs["dimensions"] = args.dimensions
    command_kwargs["window"] = args.window
    command_kwargs["mincount"] = args.mincount

    return command, command_kwargs
