"""Handles command line arguments related to checker services."""

import sys

import glog as log

import cli.common.service_configuration_handler as service_handler
import cli.db.db
import cli.checker.commands

def add_arguments(service_parsers):
    """Adds arguments and subparses for commands related to checker services.

    Args:
        service_parsers: Main argparse parser that handles multiple, various
            services.
    """

    # Checker service
    checker_service_parsers = service_parsers.add_parser(
        "checker",
        help="RPC calls to the checker service",
    )
    checker_parser = checker_service_parsers.add_subparsers(
        title="command",
        dest="command",
    )
    checker_parser.required = True

    ## Checker service: GetViolationsAll
    checker_get_violations_all_parser = checker_parser.add_parser(
        "GetViolationsAll", help="GetViolations RPC call")
    checker_get_violations_all_parser.add_argument(
        "--violation-type",
        required=True,
        choices=["unused", 'insufficient'],
    )
    checker_get_violations_all_parser.add_argument(
        "--confidence-threshold",
        default=100,
        type=int,
        help="The specification confidence to threshold on when checking for "
             "violations.",
    )
    checker_get_violations_all_parser.add_argument(
        "--overwrite",
        action="store_true",
        help="Flag for overwriting entries already in database.",
        default=False,
    )

    ## Checker service: GetViolationsUri
    checker_get_violations_uri_parser = checker_parser.add_parser(
        "GetViolationsUri",
        help="GetViolations RPC call for single uri.",
    )
    checker_get_violations_uri_parser.add_argument(
        "--violation-type",
        required=True,
        choices=["unused", 'insufficient'],
    )
    checker_get_violations_uri_parser.add_argument(
        "--bitcode-uri",
        required=True,
        help="URI for the single bitcode file",
    )
    checker_get_violations_uri_parser.add_argument(
        "--overwrite",
        action="store_true",
        help="Flag for overwriting entries already in database",
        default=False,
    )

    ## Checker service: ListViolations
    checker_list_violations_parser = checker_parser.add_parser(
        "ListViolations",
        help="Lists violations for bitcode files in MongoDB."
    )
    checker_list_violations_parser.add_argument(
        "--bitcode-uri",
        default=None,
        help="URI for a single bitcode file to filter for.",
    )
    checker_list_violations_parser.add_argument(
        "--confidence-threshold",
        default=100,
        type=int,
        help="The threshold for reporting a violation. This value ranges from "
             "0 to 100. This confidence is based on the confidence of the "
             "error specification that was used to discover the violation.",
    )
    checker_list_violations_parser.add_argument(
        "--source-code-directory",
        default=None,
        help="The base directory where the source code related to the project "
             "being analyzed is stored.",
    )

    ## Checker service: ListViolationsDiff
    checker_list_violations_diff_parser = checker_parser.add_parser(
        "ListViolationsDiff",
        help="Lists violations unique to database1 for a given bitcode ID."
    )
    checker_list_violations_diff_parser.add_argument(
        "--bitcode-uri",
        required=True,
        help="URI for a single bitcode file to filter for.",
    )
    checker_list_violations_diff_parser.add_argument(
        "--confidence-threshold1",
        default=100,
        type=int,
        help="The threshold for reporting a violation for database1. This value ranges from "
             "0 to 100. This confidence is based on the confidence of the "
             "error specification that was used to discover the violation.",
    )
    checker_list_violations_diff_parser.add_argument(
        "--confidence-threshold2",
        default=100,
        type=int,
        help="The threshold for reporting a violation for database2. This value ranges from "
             "0 to 100. This confidence is based on the confidence of the "
             "error specification that was used to discover the violation.",
    )
    checker_list_violations_diff_parser.add_argument(
        "--source-code-directory",
        default=None,
        help="The base directory where the source code related to the project "
             "being analyzed is stored.",
    )
    checker_list_violations_diff_parser.add_argument(
        "--db-1",
        required=True,
        help="Name of the database1.",
    )
    checker_list_violations_diff_parser.add_argument(
        "--db-2",
        required=True,
        help="Name of the database2.",
    )
    checker_list_violations_diff_parser.add_argument(
        "--violation-type",
        required=True,
        choices=["unused", 'insufficient'],
    )

def parse_args(args):
    """Determines which service the user called and parses further arguments."""

    assert args.service.lower() == "checker"

    parser_function = dict({
        "getviolationsall": parse_checker_get_violations_all_args,
        "getviolationsuri": parse_checker_get_violations_uri_args,
        "listviolations": parse_checker_list_violations_args,
        "listviolationsdiff": parse_checker_list_violations_diff_args,
    })

    try:
        return parser_function[args.command.lower()](args)
    except KeyError:
        log.error("Incorrect command {} provided!".format(args.command))
        sys.exit(1)

def parse_checker_get_violations_all_args(args):
    """Parses arguments related to GetViolationsAll service."""

    database = cli.db.db.connect(args.db_name, args.db_host, args.db_port)
    # ServiceConfigurationHandler handles the configuration for the various
    # services that the CLI interacts with.
    service_config_handler = \
        service_handler.ServiceConfigurationHandler.from_args(args)

    command = cli.checker.commands.get_violations_all
    command_kwargs = dict({
        "database": database,
        "service_configuration_handler": service_config_handler,
        "violation_type": args.violation_type,
        "confidence_threshold": args.confidence_threshold,
        "overwrite": args.overwrite,
    })

    return command, command_kwargs

def parse_checker_get_violations_uri_args(args):
    """Parses arguments related to GetViolationsUri service."""

    database = cli.db.db.connect(args.db_name, args.db_host, args.db_port)
    # ServiceConfigurationHandler handles the configuration for the various
    # services that the CLI interacts with.
    service_config_handler = \
        service_handler.ServiceConfigurationHandler.from_args(args)

    command = cli.checker.commands.get_violations_uri
    command_kwargs = dict({
        "database": database,
        "service_configuration_handler": service_config_handler,
        "violation_type": args.violation_type,
        "bitcode_uri": args.bitcode_uri,
        "overwrite": args.overwrite,
    })

    return command, command_kwargs

def parse_checker_list_violations_args(args):
    """Parses arguments related to ListViolatiosn service."""

    database = cli.db.db.connect(args.db_name, args.db_host, args.db_port)

    command = cli.checker.commands.list_violations
    command_kwargs = dict({
        "database": database,
        "bitcode_uri_filter": args.bitcode_uri,
        "confidence_threshold": args.confidence_threshold,
        "source_code_directory": args.source_code_directory,
    })

    return command, command_kwargs

def parse_checker_list_violations_diff_args(args):
    """Parses arguments related to ListViolationsDiff service."""

    database1 = cli.db.db.connect(args.db_1, args.db_host, args.db_port)
    database2 = cli.db.db.connect(args.db_2, args.db_host, args.db_port)

    command = cli.checker.commands.list_violations_diff
    command_kwargs = dict({
        "database1": database1,
        "database2": database2,
        "bitcode_uri": args.bitcode_uri,
        "source_code_directory": args.source_code_directory,
        "violation_type": args.violation_type,
    })

    return command, command_kwargs
