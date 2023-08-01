"""Sends requests to the checker service."""

import functools

import cli.checker.db
import cli.operations.wait
import proto.checker_pb2

def get_violations(database, operations_stub, checker_stub, id_specifications,
                   violation_type, max_tasks):
    """Sends GetViolationsRequests to checker service for all bitcode handles.

    Args:
        database: pymongo database object pointing to a MongoDB with bitcode
            and EESI information stored.
        checker_stub: Checker service stub.
        operations_stub: Operations service stub.
        bitcode_id_handles: Bitcode handles (as defined in operations.proto).
        request_specifications: List of GetSpecificationsRequest and
            GetSpecificationsResponse tuples.
        violation_type: Protobuf violation type user wants to get violations
            for.
        max_tasks: Maximum number of tasks to send to checker service at once.
    """

    # Bitcode id to violations request dictionary.
    id_vio_requests = {}
    for bitcode_id, specifications in id_specifications:
        # If matched create a violations request.
        id_vio_requests[bitcode_id.id] = proto.checker_pb2.GetViolationsRequest(
            bitcode_id=bitcode_id,
            specifications=specifications,
            violation_type=violation_type,
        )


    # Passing requests to wait_for_operations.
    return cli.operations.wait.wait_for_operations(
        operations_stub=operations_stub,
        id_requests=id_vio_requests,
        request_function=checker_stub.GetViolations,
        max_tasks=max_tasks,
        notify=functools.partial(cli.checker.db.insert_violations, database),
    )
