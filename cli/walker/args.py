"""Handles arguments related to interacting with the Walker service."""

import sys

import glog as log

import cli.common.service_configuration_handler as service_handler
import cli.walker.commands

def add_arguments(service_parsers):
    """Adds all command-line args for Walker to main CLI service parser."""
    # Walker service
    walker_service_parsers = service_parsers.add_parser(
        "walker",
        help="Commands for RPC calls to the Walker service",
    )
    walker_parser = walker_service_parsers.add_subparsers(
        title="command", dest='command')
    walker_parser.required = True

    ## Walker service: Walk
    walk_parser = walker_parser.add_parser(
        "Walk", help='Walk RPC call.')
    walk_parser.add_argument(
        "--input-icfg-uri",
        help="The URI of the ICFG file to walk over.",
        required=True,
    )
    walk_parser.add_argument(
        "--output-walks-uri",
        help="The URI of the output walks file.",
        required=True,
    )
    walk_parser.add_argument(
        "--walks-per-label",
        default=100,
        help="The number of walks to perform starting from each label.",
        type=int,
    )
    walk_parser.add_argument(
        "--walk-length",
        default=100,
        help="The maximum length of each walk starting from some label.",
        type=int,
    )

def parse_args(args):
    """Calls the argument parsing function related to args.rpc."""

    assert args.service.lower() == "walker"

    parser_function = dict({
        "walk": parse_walk_args,
    })

    try:
        return parser_function[args.command.lower()](args)
    except KeyError:
        log.error("Incorrect command {} provided!".format(args.command))
        sys.exit(1)

def parse_walk_args(args):
    """Parses command-line arguments for Walk."""

    service_config_handler = service_handler.ServiceConfigurationHandler(
        walker_address=args.walker_address,
        walker_port=args.walker_port,
        max_tasks=args.max_tasks,)

    command = cli.walker.commands.walk
    command_kwargs = dict()
    command_kwargs["service_configuration_handler"] = \
        service_config_handler
    command_kwargs["input_icfg_uri"] = args.input_icfg_uri
    command_kwargs["output_walks_uri"] = args.output_walks_uri
    command_kwargs["walks_per_label"] = args.walks_per_label
    command_kwargs["walk_length"] = args.walk_length

    return command, command_kwargs
