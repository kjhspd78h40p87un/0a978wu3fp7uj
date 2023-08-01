#!/usr/bin/env python3

"""Copy fields from one violations sheet to another.

Usage:
    ./reannotate_violations.py [-h] [--keys KEYS] [--copy COPY] src_csv dest_csv

This script copies fields (denoted by the COPY argument) from src_csv to
dest_csv, printing the result to standard output.  The KEYS argument specifies
fields to use to determine row equivalency when copying fields over.
"""

import argparse
import csv
import sys


def parse_args():
    """Parse the script's command line arguments."""

    parser = argparse.ArgumentParser()
    parser.add_argument(
        "src_csv", type=str, help="Path to the CSV file to copy data from"
    )
    parser.add_argument(
        "dest_csv", type=str, help="Path to the CSV file to copy data to"
    )
    parser.add_argument(
        "--keys",
        type=str,
        default=",".join(
            [
                "Violated Specification Function",
                "Parent Function",
                "File",
                "Line number",
            ]
        ),
        help="Comma-separated list of fields to use to determine row equivalency",
    )
    parser.add_argument(
        "--copy",
        type=str,
        default=",".join(["Inspector", "Result", "Notes"]),
        help="Comma-separated list of fields to copy",
    )

    parsed_args = parser.parse_args()
    parsed_args.keys = parsed_args.keys.split(",")
    parsed_args.copy = parsed_args.copy.split(",")

    return parsed_args


def validate_fields(src_header, dest_header, key_fields, copy_fields):
    """Verify that the key fields and copy fields exist in the CSV headers.

    If a field is not found, print a message to stderr and exit.
    """

    for fields_to_check, header, header_type in [
        (key_fields, src_header, "source"),
        (key_fields, dest_header, "destination"),
        (copy_fields, src_header, "source"),
        (copy_fields, dest_header, "destination"),
    ]:
        for field in fields_to_check:
            if field not in header:
                print(
                    "'{}' not found in {} header".format(field, header_type),
                    file=sys.stderr,
                )
                sys.exit(1)


def load_csv(filepath):
    """Load a CSV file into a (header, body) pair.

    This function also strips whitespace from cells, as Google Sheets features
    may occasionally insert extraneous whitespace (e.g., when splitting certain
    pasted text into columns).
    """

    with open(filepath, "r", newline="") as csv_file:
        rows = [[x.strip() for x in row] for row in csv.reader(csv_file)]
        return rows[0], rows[1:]


def make_keyset_map(csv_header, csv_data, key_fields):
    """Generate a map from tuples of CSV key fields to CSV data rows.

    Args:
        csv_header: The CSV file's header as a list of strings.
        csv_data: The CSV file's data as a 2D list of strings.
        key_fields: Names of CSV fields to use as map keys.

    Returns:
        A map from tuples of CSV key fields (plus an occurrence counter) to
            their corresponding CSV data rows.
    """

    counters = {}
    keyset_lookup = {}
    key_columns = [csv_header.index(col) for col in key_fields]

    for row in csv_data:
        keyset = tuple(row[i] for i in key_columns)
        counter = counters.get(keyset, 0)

        counters[keyset] = counter + 1
        keyset_lookup[keyset + (counter,)] = row  # copy by reference

    return keyset_lookup


def main():
    """Copy fields from one violations sheet to another."""

    args = parse_args()

    src_header, src_data = load_csv(args.src_csv)
    dest_header, dest_data = load_csv(args.dest_csv)
    key_fields = args.keys
    copy_fields = args.copy

    validate_fields(src_header, dest_header, key_fields, copy_fields)

    src_keysets = make_keyset_map(src_header, src_data, key_fields)
    dest_keysets = make_keyset_map(dest_header, dest_data, key_fields)

    for keyset, data in src_keysets.items():
        if keyset not in dest_keysets:
            print("No match in destination data:", keyset, file=sys.stderr)
            continue

        for field in copy_fields:
            # dest_keysets contains references to rows in dest_data, so
            # dest_data will also be modified.
            dest_keysets[keyset][dest_header.index(field)] = data[
                src_header.index(field)
            ]

    writer = csv.writer(sys.stdout)
    writer.writerow(dest_header)
    writer.writerows(dest_data)


if __name__ == "__main__":
    main()
