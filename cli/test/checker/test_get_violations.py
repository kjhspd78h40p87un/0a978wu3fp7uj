"""Tests for Checker CLI.

For tests that are related to the actual analysis that the checker performs, see
the tests provided in checker/test.
"""

import os
import sys

import pytest

import cli.bitcode.commands
import cli.common.service_configuration_handler as service_handler
import cli.checker.commands
import cli.test.common.common as common
import cli.test.checker.helpers as helper

CHECKER_HOST = helper.CHECKER_HOST
BASE_CHECKER_PORT = helper.BASE_CHECKER_PORT
BITCODE_HOST = helper.BITCODE_HOST
BASE_BITCODE_PORT = helper.BASE_BITCODE_PORT

def test_get_violations_uri():
    """Tests getting violations for saved_return.ll"""
    # Getting a unique bitcode port and eesi port for this test, starting from
    # BASE_BITCODE_PORT and BASE_EESI_PORT respectively.
    bitcode_port = str(BASE_BITCODE_PORT)
    checker_port = str(BASE_CHECKER_PORT)
    # Getting a mock database and launching the bitcode/checker services.
    database = common.setup_services([
        {"host": BITCODE_HOST, "port": bitcode_port, "service": "bitcode"},
        {"host": CHECKER_HOST, "port": checker_port, "service": "checker"},
    ])
    service_configuration_handler = service_handler.ServiceConfigurationHandler(
        bitcode_address=BITCODE_HOST, bitcode_port=bitcode_port,
        checker_address=CHECKER_HOST, checker_port=checker_port,)

    # String representation of the bitcode file URI in the test dataset.
    bitcode_uri = os.path.join(
        common.STR_DATASET_URI, "saved_return.ll")

    # Registering saved_return.ll with the bitcode service.
    cli.bitcode.commands.register_bitcode(
        database,
        service_configuration_handler,
        bitcode_uri,
        False,
    )

    # Inserting mock specifications for the checker to use.
    helper.insert_mock_specifications(database, bitcode_port)

    cli.checker.commands.get_violations_uri(
        database=database,
        service_configuration_handler=service_configuration_handler,
        bitcode_uri=bitcode_uri,
        violation_type="unused",
        overwrite=False,
    )

    assert database.GetViolationsResponse.count() == 1
    print(database.GetViolationsResponse.find_one())
    response = database.GetViolationsResponse.find_one(
            {"request": helper.get_test_request(bitcode_port)}
    )
    helper.assert_each_violation_in_response(response)

    # Dropping database and inserting a fake entry with the bitcode ID.
    # This is to test the overwrite functionality and ensure an overwrite
    # of the fake entry does not happen.
    result = database.GetViolationsResponse.delete_one(
            {"request": helper.get_test_request(bitcode_port)}
    )

    assert result.deleted_count == 1

    database.GetViolationsResponse.insert(
        {"fake": "thisisfake", "request": helper.get_test_request(bitcode_port)})

    # Testing a duplicate command and ensuring it doesn't duplicate entries.
    cli.checker.commands.get_violations_uri(
        database=database,
        service_configuration_handler=service_configuration_handler,
        bitcode_uri=bitcode_uri,
        violation_type="unused",
        overwrite=False,
    )

    assert database.GetViolationsResponse.count() == 1
    assert database.GetViolationsResponse.count() == 1
    response = database.GetViolationsResponse.find_one(
            {"request": helper.get_test_request(bitcode_port)}
    )
    helper.assert_each_violation_in_response(response, check_not_in=True)

    # Testing out overwrite.
    cli.checker.commands.get_violations_uri(
        database=database,
        service_configuration_handler=service_configuration_handler,
        bitcode_uri=bitcode_uri,
        violation_type="unused",
        overwrite=True,
    )

    assert database.GetViolationsResponse.count() == 1
    response = database.GetViolationsResponse.find_one(
            {"request": helper.get_test_request(bitcode_port)}
    )
    helper.assert_each_violation_in_response(response)


def test_get_violations_all():
    """Tests getting violations for all bitcode in dataset."""
    # Getting a unique bitcode port and eesi port for this test, starting from
    # BASE_BITCODE_PORT and BASE_EESI_PORT respectively.
    bitcode_port = str(BASE_BITCODE_PORT + 1)
    checker_port = str(BASE_CHECKER_PORT + 1)
    # Getting a mock database and launching the bitcode/checker services.
    database = common.setup_services([
        {"host": BITCODE_HOST, "port": bitcode_port, "service": "bitcode"},
        {"host": CHECKER_HOST, "port": checker_port, "service": "checker"},
    ])
    service_configuration_handler = service_handler.ServiceConfigurationHandler(
        bitcode_address=BITCODE_HOST, bitcode_port=bitcode_port,
        checker_address=CHECKER_HOST, checker_port=checker_port,
        max_tasks=1)

    # Registering all bitcode in test dataset with the bitcode service.
    cli.bitcode.commands.register_local_dataset(
        database=database,
        service_configuration_handler=service_configuration_handler,
        uri=common.STR_DATASET_URI,
        overwrite=False
    )

    helper.insert_mock_specifications(database, bitcode_port)

    cli.checker.commands.get_violations_all(
        database=database,
        service_configuration_handler=service_configuration_handler,
        violation_type="unused",
        overwrite=False,
    )

    # There should only be 1 valid specification in database. This
    # also means that we should only have one 1 violation entry.
    assert database.GetViolationsResponse.count() == 1
    response = database.GetViolationsResponse.find_one(
            {"request": helper.get_test_request(bitcode_port)}
    )
    helper.assert_each_violation_in_response(response)

if __name__ == "__main__":
    # To view standard output, add a "-s" to args.
    sys.exit(pytest.main(args=[__file__, "-s"]))
