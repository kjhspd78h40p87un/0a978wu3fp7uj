"""Interacts with MongoDB entries relevant to violations."""

import json

import glog as log
import google.protobuf.json_format

import cli.db.db
import proto.eesi_pb2

# This is a string to string mapping (NOT a protobuf)!!!
# Currently insufficient-checks violations aren't supported. We leave the
# mapping as is in case these violations are added back to EESIER.
STR_VIOLATION_TYPE_TO_STR_ENTRY = {
    "unused": "VIOLATION_TYPE_UNUSED_RETURN_VALUE",
    "insufficient": "VIOLATION_TYPE_INSUFFICIENT_CHECK",
}

def insert_violations(database, request, unpacked_response):
    """Inserts violations for a bitcode file into the database.

    Args:
        db: The mongo db object.
        request: GetViolationsRequest
        unpacked_response: Unpacked GetViolationsResponse

    Returns:
        None
    """

    finished_response = proto.checker_pb2.GetViolationsResponse()
    # Unpacking the response
    unpacked_response.response.Unpack(finished_response)
    cli.db.db.insert_request_response_pair(database, request, finished_response)

    log.info("Violations for {} stored in database."
             .format(request.bitcode_id.id))

def read_all_violations(database, violation_type):
    """Returns all GetSpecificationsRequests to violations in database."""

    # If insufficient-checks violations are added back, we should filter by
    # types. As for now we can just delete violation_type.
    del violation_type

    request_vios = []
    for entry in database.GetSpecificationsResponse.find():
        request = entry["request"]
        entry.pop("request")
        entry.pop("_id")

        request = google.protobuf.json_format.ParseDict(
            request, proto.eesi_pb2.GetSpecificationsRequest())
        vio = google.protobuf.json_format.ParseDict(
            entry, proto.eesi_pb2.GetSpecificationsResponse()).violations

        request_vios.append((request, vio))

    return request_vios

def read_violations_response(database, bitcode_id, violation_type):
    """Retrieves the list of violations from a GetSpecificationsResponse."""

    assert violation_type
    violation_type = STR_VIOLATION_TYPE_TO_STR_ENTRY[violation_type]

    # Filtering with $and query operator as it is likely that more filters will
    # be applied.
    entry = database.GetViolationsResponse.find_one({
        "$and": [
            {"request.bitcodeId.id": bitcode_id},
            {"request.violationType": violation_type},
        ]
    })

    if not entry:
        return None

    entry.pop("request")
    entry.pop("_id")
    entry_json = json.dumps(entry)
    get_violations_response = proto.checker_pb2.GetViolationsResponse()
    google.protobuf.json_format.Parse(entry_json, get_violations_response)

    return get_violations_response.violations

def delete_violations(database, bitcode_id, violation_type):
    """Deletes all violations associated with bitcode_id in datbase."""

    assert violation_type in ("unused", "insufficient")
    violation_type = STR_VIOLATION_TYPE_TO_STR_ENTRY[violation_type]

    database.GetViolationsResponse.delete_many({
        "$and": [
            {"request.bitcodeId.id": bitcode_id},
            {"request.violationType": violation_type},
        ]
    })
