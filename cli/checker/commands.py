"""Commands for tasks associated with the checker service.

Commands associated with the checker service tasks include getting violations
for all bitcode files and any individual bitcode file given a bitcode file
URI. Determining the violations requires that specifications and the bitcode
file have been registered and the relevant service response is inserted into
MongoDB.
"""

import operator
import os

from termcolor import colored
import glog as log

import cli.bitcode.db
import cli.bitcode.rpc
import cli.checker.db
import cli.checker.violation_filters as violation_filters
import cli.checker.rpc
import cli.common.log
import cli.common.uri
import cli.eesi.db
import cli.eesi.lattice_helper
import proto.eesi_pb2

VIOLATION_TYPE_TO_STRING = dict({
    proto.eesi_pb2.ViolationType.VIOLATION_TYPE_UNUSED_RETURN_VALUE:
        "unused",
    proto.eesi_pb2.ViolationType.VIOLATION_TYPE_INSUFFICIENT_CHECK:
        "insufficient",
})

STRING_TO_VIOLATION_TYPE = dict({
    v: k for k, v in VIOLATION_TYPE_TO_STRING.items()})

def get_violations_all(database, service_configuration_handler,
                       violation_type, confidence_threshold, overwrite):
    """Gets violations for all bitcode files registered with MongoDB.

    Args:
        database: pymongo databse where bitcode and EESI information is stored.
        service_configuration_handler: ServicesConfigurationHandler with
            services configured for: checker and bitcode services.
        violation_type: String representation of the violation type
            the user is interested in ["unused" | "insufficient"].
        overwrite: If True, will overwrite any GetViolationsResponse
            entries in MongoDB matching the bitcode ID.
    """

    # Getting violation type protobuf message.
    violation_type = STRING_TO_VIOLATION_TYPE[violation_type]
    assert violation_type

    bitcode_uris = cli.bitcode.db.read_uris(database)
    uri_to_specifications = dict() 
    for bitcode_uri in bitcode_uris:
        #Parsing string uri into protobuf message.
        bitcode_id = cli.bitcode.db.read_id_for_uri(database, bitcode_uri)

        if not bitcode_id:
            raise LookupError("{} URI not found in database. Use RegisterBitcode "
                              "command first".format(cli.common.uri.to_str(
                                  bitcode_uri)))

        specifications = cli.eesi.db.read_specifications_response(
            database, bitcode_id)

        if not specifications:
            raise LookupError("Specifications for {} not found in database. Use "
                              "a GetSpecifications command first!".format(
                                  cli.common.uri.to_str(bitcode_uri)))

        uri_to_specifications[cli.common.uri.to_str(bitcode_uri)] = specifications


    get_violations(database, service_configuration_handler, uri_to_specifications,
                   violation_type, confidence_threshold, overwrite)

    cli.common.log.log_finished("GetViolationsAll", True, False)

def get_violations_uri(database, service_configuration_handler, bitcode_uri,
                       violation_type, specifications_confidence_threshold, overwrite):
    """Gets violations for an individual bitcode file given a URI.

    Args:
        database: pymongo databse where bitcode and EESI information is stored.
        service_configuration_handler: ServicesConfigurationHandler with
            services configured for: checker and bitcode services.
        bitcode_uri: String representation of the bitcode file URI.
        violation_type: String representation of the violation type
            the user is interested in ["unused" | "insufficient"].
        overwrite: If True, will overwrite any GetViolationsResponse
            entries in MongoDB matching the bitcode ID.
    """

    # Getting the violation type protobuf message.
    violation_type = STRING_TO_VIOLATION_TYPE[violation_type]
    assert violation_type

    #Parsing string uri into protobuf message.
    bitcode_uri = cli.common.uri.parse(bitcode_uri)
    bitcode_id = cli.bitcode.db.read_id_for_uri(database, bitcode_uri)

    if not bitcode_id:
        raise LookupError("{} URI not found in database. Use RegisterBitcode "
                          "command first".format(cli.common.uri.to_str(
                              bitcode_uri)))

    specifications = cli.eesi.db.read_specifications_response(
        database, bitcode_id)

    if not specifications:
        raise LookupError("Specifications for {} not found in database. Use "
                          "a GetSpecifications command first!".format(
                              cli.common.uri.to_str(bitcode_uri)))

    get_violations(service_configuration_handler,
                   {cli.common.uri.to_str(bitcode_uri): specifications},
                   violation_type, confidence_threshold, overwrite)

    cli.common.log.log_finished("GetViolationsUri", True, False)

def get_violations(database, service_configuration_handler, uri_to_specifications,
                   violation_type, confidence_threshold, overwrite):

    id_specifications = []
    for bitcode_uri, specifications in uri_to_specifications.items():
        bitcode_uri = cli.common.uri.parse(bitcode_uri)
        # Ensuring that the bitcode file is registered with the Bitcode service.
        bitcode_id_handle = cli.bitcode.rpc.register_bitcode(
            service_configuration_handler.get_bitcode_stub(), bitcode_uri)
        bitcode_id_handle.authority = \
            service_configuration_handler.bitcode_config.get_full_address()

        if overwrite:
            cli.checker.db.delete_violations(
                database=database,
                bitcode_id=bitcode_id_handle.id,
                violation_type=VIOLATION_TYPE_TO_STRING[violation_type],
            )

        # Checking if an entry already exists for bitcode ID
        # and violation type. The unique_id is being set
        # in such a way that exploits the query in cli.db.db.
        # We do this because collection_contains_id is typically
        # used for a single id_type to id, however, we need to
        # check both the bitcode ID and violation type.
        entry_exists = cli.db.db.collection_contains_id(
            database=database,
            id_type="$and",
            unique_id=[{"request.bitcodeId.id": bitcode_id_handle.id},
                       {"request.violationType": cli.checker.db
                                                 .STR_VIOLATION_TYPE_TO_STR_ENTRY[
                                                     VIOLATION_TYPE_TO_STRING[
                                                         violation_type]]}],
            collection="GetViolationsResponse",
        )

        if entry_exists:
            log.info("Collection GetViolationsResponse already has an entry for "
                     "bitcode id {} and {} violation types".format(
                         bitcode_id_handle.id,
                         VIOLATION_TYPE_TO_STRING[violation_type]))
            return

        threshold_specifications = []
        for specification in specifications:
            lattice_element = cli.eesi.lattice_helper.get_lattice_element_from_confidence(
                specification, confidence_threshold)
            # No need to send specifications whose lattice element is
            # the emptyset or bottom.
            if lattice_element in (
                    "emptyset", 
                    proto.eesi_pb2.SignLatticeElement.SIGN_LATTICE_ELEMENT_BOTTOM):
                continue
            # Not setting the confidence here since it will not be utilized
            # by the checker. This is okay since we apply thresholding before
            # sending the request. This should also reduce the gRPC payload.
            threshold_specifications.append(
                proto.eesi_pb2.Specification(
                    function=specification.function,
                    lattice_element=lattice_element))
        id_specifications.append((bitcode_id_handle, threshold_specifications))
        print(f"id: {bitcode_id_handle.id} specifications size: {len(threshold_specifications)}")

    #exit()
    # Sending off bitcode handles and specifications to checker.rpc
    finished = cli.checker.rpc.get_violations(
        checker_stub=service_configuration_handler.get_checker_stub(),
        operations_stub=service_configuration_handler.get_operations_stub(
            "checker"),
        id_specifications=id_specifications,
        violation_type=violation_type,
        max_tasks=service_configuration_handler.max_tasks,
        database=database,
    )

    # If something went wrong when communicating with the service, but we
    # did not catch it, but we ended up getting no finished responses.
    if not finished:
        log.info("Error with Checker RPC communication!!! Could not complete!")
        return

def list_violations(database, bitcode_uri_filter, confidence_threshold,
                    source_code_directory):
    """Prints out the violations for the bitcode files in MongoDB."""

    #TODO (): Redo this and other print/list functions
    #to be more uniform. Create a helper file in a future PR.
    print("---Violations that appear in database---")
    id_file_dict = cli.bitcode.db.read_id_file_dict(database)
    for bitcode_id, bitcode_uri in id_file_dict.items():
        # Printing headers for each bitcode entry.
        if bitcode_uri_filter and bitcode_uri_filter != bitcode_uri:
            continue

        print("-"*30)
        print(colored(
            f"{'Bitcode ID (last 8 characters):':<40} {'File name:':<75}",
            "red",))
        print(f"{bitcode_id[-8:]:40} {os.path.basename(bitcode_uri):<75}")
        print(colored(
            f"{'Function:':<40} {'Parent Function:':<40}"
            f" {'Violation:':<30} {'Line:':<10}",
            "green",))

        violations_response_unused = cli.checker.db.read_violations_response(
            database, bitcode_id, "insufficient")

        if not violations_response_unused:
            print("NONE FOUND")
            continue

        total_threshold = 0
        # TODO(): If we add insufficient-checks violations back into
        # our response, we will need to handle them separately.
        violations_response_unused.sort(key=operator.attrgetter("confidence"),
                                        reverse=True)

        #checked = defaultdict(int)
        #unchecked = defaultdict(int)
        
        for violation in violations_response_unused:
            if (violation_filters.ignore_violation(violation, source_code_directory)):
                continue

            violation_fname = violation.specification.function.source_name
            # Checking for values because protobuf defaults to 0 for numerical
            # fields and empty strings for strings.
            violation_pname = violation.parent_function.source_name or "N/A"
            line = violation.location.line or "N/A"
            total_threshold += 1

            print("{:<80} {:<80} {:<30} {:<4} {:<30}".format(
                violation_fname, violation_pname, line,
                violation.confidence, violation.message))
            #print(f"{violation.location.file + ':' + violation.location.line:<80} "
            #      f"{violation_fname:<80} {:<30} {:<4} {:<30}".format(
            #    violat, violation_pname, line,
            #    violation.confidence, violation.message))

        print("Violations total: {}".format(total_threshold))

def generate_violations_frequency_confidence(database, 
                                             bitcode_id, violation_type,
                                             source_code_directory):
    violations = cli.checker.db.read_violations_response(
        database, bitcode_id, violation_type)
    called_functions = cli.bitcode.db.read_called_functions_response(
        database, bitcode_id).called_functions
    frequency = {cf.function.source_name: (0, cf.total_call_sites)
                    for cf in called_functions}
    for violation in violations:
        if (violation_filters.ignore_violation(violation, source_code_directory)):
            continue
        fname = violation.specification.function.source_name
        frequency[fname] = (frequency[fname][0] + 1, frequency[fname][1])

    return frequency

def generate_violations_dict(database, bitcode_id, violation_type, source_code_directory):
    violations_response = cli.checker.db.read_violations_response(
        database, bitcode_id, violation_type)
    if not violations_response:
        raise KeyError(f"No violations response found for {bitcode_id}.")

    violation_locations = dict()
    for violation in violations_response:
        if (violation_filters.ignore_violation(violation, source_code_directory)):
            continue
        violation_fname = violation.specification.function.source_name
        violation_pname = violation.parent_function.source_name or "N/A"
        line = violation.location.line or "N/A"
        if violation_fname not in violation_locations:
            violation_locations[violation_fname] = dict()

        if violation_pname not in violation_locations[violation_fname]:
            violation_locations[violation_fname][violation_pname] = dict()

        violation_locations[violation_fname][violation_pname][line] = violation.message

    return violation_locations


def list_violations_diff(database1, database2, bitcode_uri, source_code_directory, violation_type):
    """Prints out the violations unique to database1."""

    #TODO (): Redo this and other print/list functions
    #to be more uniform. Create a helper file in a future PR.
    print(bitcode_uri)
    str_uri = bitcode_uri
    bitcode_uri = cli.common.uri.parse(bitcode_uri)
    id1 = cli.bitcode.db.read_id_for_uri(database1, bitcode_uri)
    id2 = cli.bitcode.db.read_id_for_uri(database2, bitcode_uri)
    if not id1 or not id2 or id1 != id2:
        raise KeyError(f"Bitcode URI {str_uri} not found in databases!")

    violation_locations1 = generate_violations_dict(database1, id1, violation_type, source_code_directory)
    violation_locations2 = generate_violations_dict(database2, id2, violation_type, source_code_directory)
    violation_confidences1 = generate_violations_frequency_confidence(database1, id1, violation_type, source_code_directory)
    
    # We are only printing out the violations that are unique to database 1. We
    # do not care about violations unique to database 2 for this function.
    for violation_fname, violation_loc in violation_locations1.items():
        for violation_pname, locs_messages in violation_loc.items():
            for loc, message in locs_messages.items():
                if (violation_fname not in violation_locations2 or
                    violation_pname not in violation_locations2[violation_fname] or
                    loc not in violation_locations2[violation_fname][violation_pname]):
                        call_count = violation_confidences1[violation_fname][1]
                        vio_count = violation_confidences1[violation_fname][0]
                        print(f"{violation_fname:<50} {violation_pname:<50} {loc:<8} "
                              f"{vio_count:<5} "
                              f"{call_count:<5}")
