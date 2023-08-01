"""Commands related to interacting with the GetGraph service.

The major interaction that occurs with the GetGraph service is getting
control flow graphs representing the bitcode file. The two primary
functionalities that a user is concerned with in this module is getting graphs
for an invidivual bitcode file and getting graphs for all registered bitcode
files in MongoDB.
"""

import cli.bitcode.db
import cli.bitcode.rpc
import cli.common.log
import cli.common.uri
import cli.db.db
import cli.getgraph.rpc

def get_graph_all(database, service_configuration_handler,
                  domain_knowledge_handler, remove_cross_folder):
    """Gets control flow graphs for all bitcode files registered in MongoDB.

    Args:
        service_configuration_handler: ServicesConfigurationHandler with
            services configured for: GetGraph and bitcode.
            (if using embeddings for EESI).
        domain_knowledge_handler: DomainKnowledgeHandler containing appropriate
            domain knowledge for a GetSpecifications task.
        remove_cross_folder: Remove cross folder for GetGraph service.
    """

    # List of bitode ID to the output graph URI. The output graph URI is
    # just the original bitcode file URI with ".icfg" appended to the file name.
    bitcode_id_output_uris = []
    for bitcode_uri in list(cli.bitcode.db.read_uris(database)):
        bitcode_id_handle = cli.bitcode.rpc.register_bitcode(
            service_configuration_handler.get_bitcode_stub(), bitcode_uri)
        bitcode_id_handle.authority = \
            service_configuration_handler.bitcode_config.get_full_address()
        output_graph_uri = cli.common.uri.parse(cli.common.uri.to_str(
            bitcode_uri) + ".icfg")
        bitcode_id_output_uris.append((bitcode_id_handle, output_graph_uri))


    # Sending to getgraph.rpc and waiting for GetGraph responses.
    cli.getgraph.rpc.get_graph(
        get_graph_stub=service_configuration_handler.get_get_graph_stub(),
        operations_stub=service_configuration_handler.get_operations_stub(
            "get_graph"),
        bitcode_id_output_uris=bitcode_id_output_uris,
        error_codes=domain_knowledge_handler.error_codes,
        remove_cross_folder=remove_cross_folder,
        max_tasks=service_configuration_handler.max_tasks,
    )

    cli.common.log.log_finished_file_output("GetGraphAll")

def get_graph_uri(database, service_configuration_handler,
                  domain_knowledge_handler, bitcode_uri, output_graph_uri,
                  remove_cross_folder):

    """Get control flow graph for a single bitcode file registered with MongoDB.

    Args:
        database: pymongo database object.
        service_configuration_handler: ServicesConfigurationHandler with
            services configured for: GetGraph and bitcode.
        domain_knowledge_handler: DomainKnowledgeHandler containing appropriate
            domain knowledge for a GetGraph task.
        bitcode_uri: This bitcode URI to generate a graph for.
        output_graph_uri: The desired URI of where to write the graph to.
        remove_cross_folder: Removes the cross folder from consideration during
            the GetGraphService.
    """

    bitcode_uri_msg = cli.common.uri.parse(bitcode_uri)
    output_graph_uri_msg = cli.common.uri.parse(output_graph_uri)
    found_id = cli.bitcode.db.read_id_for_uri(database, bitcode_uri_msg)
    if not found_id:
        raise LookupError("{} URI not found in database.".format(
            bitcode_uri))

    # Getting the bitcode handle.
    bitcode_id_handle = cli.bitcode.rpc.register_bitcode(
        service_configuration_handler.get_bitcode_stub(), bitcode_uri_msg)
    bitcode_id_handle.authority = \
        service_configuration_handler.bitcode_config.get_full_address()

    # Sending to getgraph.rpc, which sends off request to bitcode service.
    cli.getgraph.rpc.get_graph(
        get_graph_stub=service_configuration_handler.get_get_graph_stub(),
        operations_stub=service_configuration_handler.get_operations_stub(
            "get_graph"),
        bitcode_id_output_uris=[(bitcode_id_handle, output_graph_uri_msg)],
        error_codes=domain_knowledge_handler.error_codes,
        remove_cross_folder=remove_cross_folder,
        max_tasks=service_configuration_handler.max_tasks,
    )

    cli.common.log.log_finished_file_output("GetGraphUri")
