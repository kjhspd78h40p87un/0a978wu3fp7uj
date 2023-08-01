#!/usr/bin/env bash

# query-specs.bash reads function names from stdin -- one per line -- and
# outputs the corresponding specifications obtained from ListSpecifications.
#
# query-specs.bash BITCODE_PATH [--db-name DB_NAME] [--threshold THRESHOLD] [--db-port DB_PORT]
#
# BITCODE_PATH: The path to the bitcode file
# DB_NAME: The name of the mongo database to query
# THRESHOLD: The confidence threshold hold to use (default: 0.5)
# DB_PORT: The mongo port to use

strip_colors() {
    # https://superuser.com/questions/380772/removing-ansi-color-codes-from-text-stream
    sed "s/\x1b\[[0-9;]*m//g"
}

# Parse arguments
ls_specs_invocation=(eesi ListSpecifications)
while [[ $# -gt 0 ]]; do
    case $1 in
        -d | --db-name)
            ls_specs_invocation=(--db-name "$2" "${ls_specs_invocation[@]}")
            shift 2
            ;;
        -t | --threshold)
            ls_specs_invocation+=(--confidence-threshold "$2")
            shift 2
            ;;
        -p | --db-port)
            ls_specs_invocation=(--db-port "$2" "${ls_specs_invocation[@]}")
            shift 2
            ;;
        *)
            ls_specs_invocation+=(--bitcode-uri "file://$(realpath "$1")")
            shift
            ;;
    esac
done

# Grab ListSpecifications output
tmpfile="$(mktemp)"
bazel run //cli:main -- "${ls_specs_invocation[@]}" | strip_colors >"$tmpfile"

# Grep for each function's specification
while read -r func_name; do
    line="$(grep "^${func_name} " "$tmpfile")"
    if [[ $? = 0 ]]; then
        # A leading single quote is needed for ==0 to be formatted properly in
        # Google Sheets.
        echo "$line" | awk $'{ print "\'"$2 }'
    else
        echo "bottom"
    fi
done

rm "$tmpfile"
