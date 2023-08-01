"""Tests for Checker CLI.

For tests that are related to the actual analysis that the checker performs, see
the tests provided in checker/test.
"""

import os
import sys

import pytest

import cli.checker.violation_filters as violation_filters
import proto.eesi_pb2
import proto.bitcode_pb2

def test_ignore_casted_void():
    """Tests filtering out code snippets that contain void casts."""

    # Only inserting fields that are necessary for testing violations currently.
    casted_violation = proto.eesi_pb2.Violation(
        message="test violation",
        location=proto.bitcode_pb2.Location(
            file="void_cast_violation.c",
            line=5,
        ),
        confidence=100,
    )

    assert violation_filters.ignore_violation(
        casted_violation, os.path.abspath("testdata/programs"))

def test_not_ignore_casted_char():
    """Tests that the filter doesn't filter non-void casts."""

    casted_violation = proto.eesi_pb2.Violation(
        message="test violation",
        location=proto.bitcode_pb2.Location(
            file="void_cast_violation.c",
            line=6,
        ),
        confidence=100,
    )

    assert not violation_filters.ignore_violation(
        casted_violation, os.path.abspath("testdata/programs"))

def test_not_ignore_call():
    """Tests that the filter doesn't filter out normal function calls."""

    casted_violation = proto.eesi_pb2.Violation(
        message="test violation",
        location=proto.bitcode_pb2.Location(
            file="void_cast_violation.c",
            line=7,
        ),
        confidence=100,
    )

    assert not violation_filters.ignore_violation(
        casted_violation, os.path.abspath("testdata/programs"))

def test_fail_ignore_casted_void_no_source():
    """Tests that violations with no source code file will not be filtered."""

    # Violation with no location.
    no_source_violation = proto.eesi_pb2.Violation(
        message="test violation",
        confidence=100,
    )

    assert not violation_filters.ignore_violation(
        no_source_violation, os.path.abspath("testdata/programs"))

if __name__ == "__main__":
    # To view standard output, add a "-s" to args.
    sys.exit(pytest.main(args=[__file__, "-s"]))
