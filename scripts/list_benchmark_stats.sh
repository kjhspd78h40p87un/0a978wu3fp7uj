#!/bin/bash

SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
echo $SCRIPT_DIR
BASE_BC_DIR="file://$(cd ${SCRIPT_DIR}/../testdata/benchmarks/bitcode; pwd)/"
MBEDTLS="${BASE_BC_DIR}mbedtls-reg2mem.bc" 
LFS="${BASE_BC_DIR}littlefs-reg2mem.bc" 
NETDATA="${BASE_BC_DIR}netdata-reg2mem.bc" 
OPENSSL="${BASE_BC_DIR}openssl-reg2mem.bc" 
PIDGIN="${BASE_BC_DIR}pidgin-reg2mem.bc" 
ZLIB="${BASE_BC_DIR}zlib-reg2mem.bc"

usage() { echo "Usage: $0 [-d <DB_BASE_NAME>] [-s] [-l] [-e <EVID_COUNT>]" 1>&2; exit 1; }

declare -a bc_uris=( "$LFS" "$PIDGIN" "$MBEDTLS" "$OPENSSL" "$ZLIB" "$NETDATA" )
#DBNAME=$(date -d "$D" '+%m')$(date -d "$D" '+%d')
DBNAME="eesier"
declare -a evidence_counts=( 3 )
declare -a thresholds=( 1 100 )
while getopts ":d:s:l:e:" o; do
    case "${o}" in
        d)
            DBNAME=${OPTARG}
            ;;
        s)
            bc_uris=( "$LFS" "$ZLIB" "$PIDGIN" )
            ;;
        l)
            bc_uris=( "$OPENSSL" "$MBEDTLS" "$NETDATA" )
            ;;
        e)
            evidence_counts=( ${OPTARG} )
            ;;
        esac
done

OVERWRITE=$1

for count in ${evidence_counts[@]}; do
    for t in ${thresholds[@]}; do
        for bc_uri in ${bc_uris[@]}; do
        bc_basename="$(basename $bc_uri -reg2mem.bc)"
        gt_path="${SCRIPT_DIR}/../testdata/benchmarks/ground_truth/${bc_basename}-gt.txt"

        bazel run //cli:main -- --db-name ${DBNAME}_evid${count} eesi \
          ListStatisticsDatabase --bitcode-uri $bc_uri \
          --ground-truth-path $gt_path --confidence-threshold ${t} > \
          ${SCRIPT_DIR}/../testdata/benchmarks/stats/${bc_basename}-${t}-stats.txt
        done
    done
done
