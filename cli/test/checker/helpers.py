"""Contains helper functions/variables for checker CLI tests."""

CHECKER_HOST = "127.0.0.1"
# This gets incremented for each CLI checker test case. This is simply just
# the initial/base port number for the checker service.
BASE_CHECKER_PORT = 70050
BITCODE_HOST = "127.0.0.1"
# This port number is unique to the checker test file. This is also only the 
# base port number, as we utilize multiple port numbers while testing. Each
# test in this file will use a unique port number (incrementing starting from
# this base number). This checker CLI test files should only utlize 70060-70069.
BASE_BITCODE_PORT = 70060
SAVED_RETURN_ID = \
    "db38e7b861ca1875099935f9ce5331cd76eb939a974a298aa2cb094dfa478cc8"

def insert_mock_specifications(database, bitcode_port):
    """Inserts fake specifications for saved_return.ll in the mock database.

    Args:
        database: The mock database to insert specifications into.
        bitcode_port: The port number that the bitcode service is running on for
            testing. This is the only argument, as each test will need to
            utilize a different bitcode port, whereas information such as the
            host is uniform across tests.
    """

    # Inserting a "fake" (but representative of how it really looks) entry
    # for specifications that can be used by the checker.
    specifications = [
        {
            "function": {
                "llvmName": "bar",
                "sourceName": "bar",
            },
            "latticeElement": "SIGN_LATTICE_ELEMENT_ZERO",
        }
    ]
    request = {
        "bitcodeId": {
            "id": SAVED_RETURN_ID,
            "authority": "{}:{}".format(BITCODE_HOST, bitcode_port),
        },
        "initialSpecifications": [
            {
                "function": {
                    "llvmName": "bar",
                    "sourceName": "bar",
                },
                "latticeElement": "SIGN_LATTICE_ELEMENT_ZERO",
            }
        ]
    }

    database.GetSpecificationsResponse.insert(
        {
            "specifications" : specifications,
            "request": request,
        }
    )


# Dictionary for how the violations in the database should appear.
VIOLATIONS = [
    {
        "violationType" : "VIOLATION_TYPE_UNUSED_RETURN_VALUE",
        "message" : "Unused return value.",
        "location" : {
            "file" : "saved_return.c",
            "line" : "13",
        },
        "specification" : {
            "function" : {
                "llvmName" : "bar",
                "sourceName" : "bar",
            },
            "latticeElement" : "SIGN_LATTICE_ELEMENT_ZERO",
        },
        "parentFunction" : {
            "llvmName" : "foo",
            "sourceName" : "foo",
            "returnType" : "FUNCTION_RETURN_TYPE_VOID",
        }
    },
    {
        "violationType" : "VIOLATION_TYPE_UNUSED_RETURN_VALUE",
        "message" : "Unused return value.",
        "location" : {
            "file" : "saved_return.c",
            "line" : "16",
        },
        "specification" : {
            "function" : {
                "llvmName" : "bar",
                "sourceName" : "bar",
            },
            "latticeElement" : "SIGN_LATTICE_ELEMENT_ZERO",
        },
        "parentFunction" : {
            "llvmName" : "foo",
            "sourceName" : "foo",
            "returnType" : "FUNCTION_RETURN_TYPE_VOID",
        }
    }
]

def assert_each_violation_in_response(response, check_not_in=False):
    # If the violations entry in the response does not exist, then we know
    # that none of the test violations can appear in the response. If
    # check_not_in is set to True, then we can just return here, else we should
    #fail.
    if check_not_in and "violations" not in response:
        return
    assert "violations" in response

    violations_response = response["violations"]
    assert len(violations_response) == len(VIOLATIONS)
    for violation in VIOLATIONS:
        if check_not_in:
            assert violation not in violations_response
            continue
        assert violation in violations_response

def get_test_request(bitcode_port):
    """Returns a request map to be used for testing the Checker CLI.

    Args:
        bitcode_port: The port number that the bitcode service is running on for
            testing. This is the only argument, as each test will need to
            utilize a different bitcode port, whereas information such as the
            host is uniform across tests.
    """
    # How the request entry should appear.
    return {
        "bitcodeId" : {
            "id" : SAVED_RETURN_ID,
            "authority" : "{}:{}".format(BITCODE_HOST, bitcode_port),
        },
        "specifications" : [
            {
                "function" : {
                    "llvmName" : "bar",
                    "sourceName" : "bar",
                },
                "latticeElement" : "SIGN_LATTICE_ELEMENT_ZERO",
            }
        ],
        "violationType" : "VIOLATION_TYPE_UNUSED_RETURN_VALUE",
    }
