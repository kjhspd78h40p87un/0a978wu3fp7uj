#!/bin/bash


usage() { echo "Usage: $0 <functions_to_filter_file> <specifications_file>" 1>&2;
          exit 1; }

if [ "$#" -ne 2 ]; then
    usage
fi
while read function_name; do
    grep -w ${function_name} $2 | awk 'BEGIN { rc = 1} (NF>1) { rc = 0; print $NF } END { exit rc }'
    if [ $? -ne 0 ]; then
        echo "bottom"
    fi
done < $1
