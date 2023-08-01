""" Interacts with the Walker RPC service."""

import proto.walker_pb2
import cli.operations.wait

def walk(walker_stub, operations_stub, input_icfg_uri, output_walks_uri,
         walk_length, walks_per_label):
    """Creates RandomWalkLegacyIcfgRequests and sends to wait for each request.

    Args:
        walker_stub: Stub for Walker channel.
        operations_stub: Stub for operations channel.
        input_icfg_uri: The URI message pertaining to the ICFG to be walked
            over.
        output_walks_uri: The URI message pertaining to the file where the walks
            will be written.
        walk_length: The walk length from each label.
        walks_per_label: The number of walks to perform for each label.
    """

    id_requests = {
        input_icfg_uri.path : proto.walker_pb2.RandomWalkLegacyIcfgRequest(
            input_icfg_uri=input_icfg_uri,
            output_walks_uri=output_walks_uri,
            walk_length=walk_length,
            walks_per_label=walks_per_label,
        )
    }
    cli.operations.wait.wait_for_operations(
        operations_stub=operations_stub,
        id_requests=id_requests,
        request_function=walker_stub.RandomWalkLegacyIcfgBackground,
        max_tasks=1,
        notify=None,
    )
