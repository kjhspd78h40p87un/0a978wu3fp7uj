""" Interacts with the GetGraph RPC service."""

import cli.operations.wait
import proto.get_graph_pb2

def get_graph(get_graph_stub, operations_stub, bitcode_id_output_uris,
                       remove_cross_folder, error_codes, max_tasks):
    """Creates GetGraphRequests and sends to wait for each request.

    Args:
        get_graph_stub: Stub for GetGraph channel.
        operations_stub: Stub for operations channel.
        bitcode_id_requests: Dictionary from bitcode id to the relevant
            GetGraphRequest.
        max_tasks: The maximum number of tasks to be sent to the EESI service
            at once by operations.wait.
    """

    id_graph_requests = {}
    for bitcode_id_handle, output_uri in bitcode_id_output_uris:
        get_graph_request = proto.get_graph_pb2.GetGraphRequest(
            bitcode_id=bitcode_id_handle,
            output_graph_uri=output_uri,
            remove_cross_folder=remove_cross_folder,
            error_codes=error_codes,
        )
        id_graph_requests[bitcode_id_handle.id] = get_graph_request

    # Sending the GetGraphRequest to the GetGraph service. The results are
    # currently written into a file specified in the request. These GetGraph
    # Edgelist is currently not stored in any database.
    cli.operations.wait.wait_for_operations(
        operations_stub=operations_stub,
        id_requests=id_graph_requests,
        request_function=get_graph_stub.GetGraph,
        max_tasks=max_tasks,
        notify=None
    )
