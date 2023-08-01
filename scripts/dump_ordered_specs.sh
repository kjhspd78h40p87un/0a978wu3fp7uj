#!/bin/bash

./scripts/list_specifications.sh

SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
BENCHMARKS=( "littlefs" "pidgin" "openssl" "mbedtls" "netdata" "zlib")
expansion_operations=( "meet" "join" "max" )

for operation in ${expansion_operations[@]}; do
    # Hacky, but we only need to dump the analysis-only results once.
    dumped_all_no_expand=false
    for benchmark in ${BENCHMARKS[@]}; do
        ${SCRIPT_DIR}/ordered_specifications.sh \
            ${SCRIPT_DIR}/../testdata/ordered_functions_of_interest/${benchmark}-functions.txt \
            ${SCRIPT_DIR}/../testdata/specifications/${operation}/${benchmark}-specs.txt \
            > ${SCRIPT_DIR}/../testdata/sheets_utility/ordered_specifications/${operation}/${benchmark}-ordered.txt
        if [ $dumped_all_no_expand = false ]; then
            ${SCRIPT_DIR}/ordered_specifications.sh \
                ${SCRIPT_DIR}/../testdata/ordered_functions_of_interest/${benchmark}-functions.txt \
                ${SCRIPT_DIR}/../testdata/specifications/no_expand/${benchmark}-specs.txt \
                > ${SCRIPT_DIR}/../testdata/sheets_utility/ordered_specifications/no_expand/${benchmark}-ordered.txt
        fi
    done
done
