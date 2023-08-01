"""Handles arguments related to interacting with the GetGraph service."""

import sys

import glog as log

import cli.common.service_configuration_handler as service_handler
import cli.db.db
import cli.eesi.domain_knowledge_handler as dk_handler
import cli.getgraph.commands

def add_arguments(service_parsers):
    """Adds all command-line args for GetGraph to main CLI service parser."""
    # GetGraph service
    get_graph_service_parsers = service_parsers.add_parser(
        "getgraph",
        help="Commands for RPC calls to the getgraph service",
    )
    get_graph_parser = get_graph_service_parsers.add_subparsers(
        title="command", dest='command')
    get_graph_parser.required = True

    ## GetGraph service: GetGraphAll
    get_graph_all_parser = get_graph_parser.add_parser(
        "GetGraphAll", help='GetGraph RPC call')
    get_graph_all_parser.add_argument(
        "--error-codes",
        default=None,
        help="Absolute path to the error codes file"
    )

    ## GetGraph service: GetGraphUri
    get_graph_uri_parser = get_graph_parser.add_parser(
        "GetGraphUri",
        help="GetGraph RPC call for a single bitcode file from URI",
    )
    get_graph_uri_parser.add_argument(
        "--error-codes",
        default=None,
        help="Absolute path to the error codes file",
    )
    get_graph_uri_parser.add_argument(
        "--bitcode-uri",
        default=None,
        required=True,
        help="URI of bitcode file.",
    )
    get_graph_uri_parser.add_argument(
        "--output-graph-uri",
        default=None,
        required=True,
        help="URI of the desired output graph file URI.",
    )
    get_graph_uri_parser.add_argument(
        "--remove-cross-folder",
        default=False,
        action="store_true",
        help="Removes cross folder from consideration during GetGraph.",
    )

def parse_args(args):
    """Calls the argument parsing function related to args.rpc."""

    assert args.service.lower() == "getgraph"

    parser_function = dict({
        "getgraphall": parse_get_graph_all_args,
        "getgraphuri": parse_get_graph_uri_args,
    })

    try:
        return parser_function[args.command.lower()](args)
    except KeyError:
        log.error("Incorrect command {} provided!".format(args.command))
        sys.exit(1)

def parse_get_graph_all_args(args):
    """Parses command-line arguments for GetGraphAll."""

    database = cli.db.db.connect(args.db_name, args.db_host, args.db_port)
    service_config_handler = service_handler.ServiceConfigurationHandler(
        get_graph_address=args.get_graph_address,
        get_graph_port=args.get_graph_port,
        bitcode_address=args.bitcode_address, bitcode_port=args.bitcode_port,
        max_tasks=args.max_tasks,)
    domain_knowledge_handler = dk_handler.DomainKnowledgeHandler(
        initial_specifications_path=None,
        error_only_path=None,
        error_codes_path=args.error_codes,
        success_codes_path=None)

    command = cli.getgraph.commands.get_graph_all
    command_kwargs = dict()
    command_kwargs["database"] = database
    command_kwargs["service_configuration_handler"] = \
        service_config_handler
    command_kwargs["domain_knowledge_handler"] = domain_knowledge_handler
    command_kwargs["remove_cross_folder"] = args.remove_cross_folder

    return command, command_kwargs

def parse_get_graph_uri_args(args):
    """Parses command-line arguments for GetGraphUri"""

    database = cli.db.db.connect(args.db_name, args.db_host, args.db_port)
    service_config_handler = service_handler.ServiceConfigurationHandler(
        get_graph_address=args.get_graph_address,
        get_graph_port=args.get_graph_port,
        bitcode_address=args.bitcode_address, bitcode_port=args.bitcode_port,
        max_tasks=args.max_tasks,)
    domain_knowledge_handler = dk_handler.DomainKnowledgeHandler(
        initial_specifications_path=None,
        error_only_path=None,
        error_codes_path=args.error_codes,
        success_codes_path=None)

    command = cli.getgraph.commands.get_graph_uri
    command_kwargs = dict()
    command_kwargs["database"] = database
    command_kwargs["bitcode_uri"] = args.bitcode_uri
    command_kwargs["output_graph_uri"] = args.output_graph_uri
    command_kwargs["service_configuration_handler"] = \
        service_config_handler
    command_kwargs["domain_knowledge_handler"] = domain_knowledge_handler
    command_kwargs["remove_cross_folder"] = args.remove_cross_folder

    return command, command_kwargs
