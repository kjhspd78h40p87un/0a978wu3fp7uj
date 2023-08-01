"""Tests for GetGraph commands."""

import os
import sys

import pytest

import cli.bitcode.commands
import cli.common.service_configuration_handler as service_handler
import cli.getgraph.commands
import cli.eesi.domain_knowledge_handler as dk_handler
import cli.test.common.common as common

GET_GRAPH_HOST = "127.0.0.1"
BASE_GET_GRAPH_PORT = 90050
BITCODE_HOST = "127.0.0.1"
# This port number is unique to this test file. This is done so that when
# services are launched for testing purposes, there are no conflicting port
# numbers.
BASE_BITCODE_PORT = 90060

def test_get_graph_uri():
    """Tests getting a control flow graph (CFG) for a single bitcode file."""
    # Getting a unique bitcode port and eesi port for this test, starting from
    # BASE_BITCODE_PORT and BASE_EESI_PORT respectively.
    bitcode_port = str(BASE_BITCODE_PORT)
    get_graph_port = str(BASE_GET_GRAPH_PORT)
    # Getting a mock database and launching the bitcode service.
    database = common.setup_database()
    # Starting up both the bitcode and EESI service.
    common.setup_service(BITCODE_HOST, bitcode_port, "bitcode")
    common.setup_service(GET_GRAPH_HOST, get_graph_port, "getgraph")

    service_configuration_handler = service_handler.ServiceConfigurationHandler(
        bitcode_address=BITCODE_HOST, bitcode_port=bitcode_port,
        get_graph_address=GET_GRAPH_HOST, get_graph_port=get_graph_port,)
    domain_knowledge_handler = dk_handler.DomainKnowledgeHandler(
        initial_specifications_path=None,
        error_only_path=None,
        error_codes_path=None,
        success_codes_path=None,)

    # String representation of the bitcode file URI in the test dataset.
    bitcode_uri = os.path.join(
        common.STR_DATASET_URI, "error_only_function_ptr.ll")

    # String representation of the output graph URI.
    graph_uri = os.path.join(
        common.STR_DATASET_URI, "error_only_function_ptr.graph")

    # Registering error_only_function_ptr.ll with the bitcode service.
    cli.bitcode.commands.register_bitcode(
        database,
        service_configuration_handler,
        bitcode_uri,
        False,
    )

    cli.getgraph.commands.get_graph_uri(
        database=database,
        service_configuration_handler=service_configuration_handler,
        domain_knowledge_handler=domain_knowledge_handler,
        bitcode_uri=bitcode_uri,
        output_graph_uri=graph_uri,
        remove_cross_folder=False,
    )

    assert os.path.isfile(os.path.abspath(
        "testdata/programs/error_only_function_ptr.graph"))

    os.remove(os.path.abspath(
        "testdata/programs/error_only_function_ptr.graph"))

def test_get_graph_all():
    """Tests getting CFGs for all bitcode files registered with database."""
    # Getting a unique bitcode port and eesi port for this test, starting from
    # BASE_BITCODE_PORT and BASE_EESI_PORT respectively.
    bitcode_port = str(BASE_BITCODE_PORT + 1)
    get_graph_port = str(BASE_GET_GRAPH_PORT + 1)
    # Getting a mock database and launching the bitcode service.
    database = common.setup_database()
    # Assigning a unique port for the test.
    common.setup_service(BITCODE_HOST, bitcode_port, "bitcode")
    common.setup_service(GET_GRAPH_HOST, get_graph_port, "getgraph")

    # Setting up handlers.
    service_configuration_handler = service_handler.ServiceConfigurationHandler(
        bitcode_address=BITCODE_HOST, bitcode_port=bitcode_port,
        get_graph_address=GET_GRAPH_HOST, get_graph_port=get_graph_port,
        max_tasks=20)
    domain_knowledge_handler = dk_handler.DomainKnowledgeHandler(
        initial_specifications_path=None,
        error_only_path=None,
        error_codes_path=None,
        success_codes_path=None,)

    # Registering all bitcode in test dataset with the bitcode service.
    cli.bitcode.commands.register_local_dataset(
        database=database,
        service_configuration_handler=service_configuration_handler,
        uri=common.STR_DATASET_URI,
        overwrite=False,
    )

    cli.getgraph.commands.get_graph_all(
        database=database,
        service_configuration_handler=service_configuration_handler,
        domain_knowledge_handler=domain_knowledge_handler,
        remove_cross_folder=False,
    )

    # Check that a graph file was created for each test program.
    testdata_full_paths = [os.path.join("testdata/programs/", bc_file)
        for bc_file, _ in common.TESTDATA_INFO]
    assert all(
        [os.path.isfile(f"{bc_file}.icfg")
         for bc_file in testdata_full_paths])

    # Removing graph files generated for test.
    for bc_file in testdata_full_paths:
        os.remove(f"{bc_file}.icfg")

if __name__ == "__main__":
    # To view standard output, add a "-s" to args.
    sys.exit(pytest.main(args=[__file__]))
