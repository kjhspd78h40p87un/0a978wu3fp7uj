#!/bin/bash

SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
echo $SCRIPT_DIR
BASE_BC_DIR="file://$(cd ${SCRIPT_DIR}/../testdata/benchmarks/bitcode; pwd)/"
BASE_DK_DIR="$(cd ${SCRIPT_DIR}/../testdata/benchmarks/domain_knowledge; pwd)/"
BASE_EMBED_DIR="file://$(cd ${SCRIPT_DIR}/../testdata/benchmarks/embeddings; pwd)/"
MBEDTLS="${BASE_BC_DIR}mbedtls-reg2mem.bc" 
LFS="${BASE_BC_DIR}littlefs-reg2mem.bc" 
NETDATA="${BASE_BC_DIR}netdata-reg2mem.bc" 
OPENSSL="${BASE_BC_DIR}openssl-reg2mem.bc" 
PIDGIN="${BASE_BC_DIR}pidgin-reg2mem.bc" 
ZLIB="${BASE_BC_DIR}zlib-reg2mem.bc"
BAZEL=bazel-4.1.0

declare -A error_codes=([$(basename $MBEDTLS .bc)]="${BASE_DK_DIR}error-code-mbedtls.txt" 
                        [$(basename $LFS .bc)]="${BASE_DK_DIR}error-code-littlefs.txt" 
                        [$(basename $NETDATA .bc)]="" 
                        [$(basename $OPENSSL .bc)]="${BASE_DK_DIR}error-code-openssl.txt" 
                        [$(basename $PIDGIN .bc)]="" 
                        [$(basename $ZLIB .bc)]="${BASE_DK_DIR}error-code-zlib.txt")
declare -A error_onlys=([$(basename $MBEDTLS .bc)]="${BASE_DK_DIR}error-only-mbedtls.txt" 
                        [$(basename $LFS .bc)]="" 
                        [$(basename $NETDATA .bc)]="" 
                        [$(basename $OPENSSL .bc)]="${BASE_DK_DIR}error-only-openssl.txt" 
                        [$(basename $PIDGIN .bc)]="" 
                        [$(basename $ZLIB .bc)]="")
declare -A initial_specifications=([$(basename $MBEDTLS .bc)]="${BASE_DK_DIR}input-specs-mbedtls.txt" 
                                   [$(basename $LFS .bc)]="${BASE_DK_DIR}input-specs-littlefs.txt" 
                                   [$(basename $NETDATA .bc)]="${BASE_DK_DIR}input-specs-netdata.txt" 
                                   [$(basename $OPENSSL .bc)]="${BASE_DK_DIR}input-specs-openssl.txt" 
                                   [$(basename $PIDGIN .bc)]="${BASE_DK_DIR}input-specs-pidgin.txt" 
                                   [$(basename $ZLIB .bc)]="${BASE_DK_DIR}input-specs-zlib.txt")
declare -A success_codes=([$(basename $MBEDTLS .bc)]="${BASE_DK_DIR}success-code-mbedtls.txt" 
                          [$(basename $LFS .bc)]="${BASE_DK_DIR}success-code-littlefs.txt" 
                          [$(basename $NETDATA .bc)]="" 
                          [$(basename $OPENSSL .bc)]="" 
                          [$(basename $PIDGIN .bc)]="" 
                          [$(basename $ZLIB .bc)]="${BASE_DK_DIR}success-code-zlib.txt" )
declare -A smart_success_code_zeros=([$(basename $MBEDTLS .bc)]="--smart-success-code-zero" 
                                     [$(basename $LFS .bc)]="--smart-success-code-zero" 
                                     [$(basename $NETDATA .bc)]="" 
                                     [$(basename $OPENSSL .bc)]="" 
                                     [$(basename $PIDGIN .bc)]="" 
                                     [$(basename $ZLIB .bc)]="--smart-success-code-zero" )

usage() { echo "Usage: $0 [-d <DB_BASE_NAME>] [-e <EVID_COUNT>]" 1>&2;
          echo "          [-z zlib] [-p pidgin] [-n netdata] [-m mbedtls]" 1>&2;
          echo "          [-l littlefs] [-s openssl] [-o overwrite]" 1>&2; exit 1; }

declare -a bc_uris=( "$LFS" "$PIDGIN" "$MBEDTLS" "$OPENSSL" "$ZLIB" "$NETDATA" )
expansion_operations=(  "meet" "join" "max" )
#DBNAME=$(date -d "$D" '+%m')$(date -d "$D" '+%d')
DBNAME="eesier"
OVERWRITE=""
declare -a evidence_counts=( 3 )
while getopts "d:osle:zpnmls" o; do
    case "${o}" in
        d)
            DBNAME=${OPTARG}
            ;;
        e)
            evidence_counts=( ${OPTARG} )
            ;;
        z)
            bc_uris=( "$ZLIB" )
            ;;
        p)
            bc_uris=( "$PIDGIN" )
            ;;
        n)
            bc_uris=( "$NETDATA" )
            ;;
        m)
            bc_uris=( "$MBEDTLS" )
            ;;
        l)
            bc_uris=( "$LFS" )
            ;;
        s)
            bc_uris=( "$OPENSSL" )
            ;;
        o)
            OVERWRITE="--overwrite"
            ;;
        *)
            usage
            ;;
        esac
done

for operation in ${expansion_operations[@]}; do
    for count in ${evidence_counts[@]}; do
      for bc_uri in ${bc_uris[@]}; do
        bc_basename="$(basename $bc_uri .bc)"
        error_code="--error-codes ${error_codes[$bc_basename]}"
        if [ "${error_code}" == "--error-codes " ]; then
          error_code=""
        fi
        error_only="--error-only ${error_onlys[$bc_basename]}"
        if [ "${error_only}" == "--error-only " ]; then
          error_only=""
        fi
        init_specs="--initial-specifications ${initial_specifications[$bc_basename]}"
        if [ "${init_specs}" = "--initial-specifications " ]; then
          init_specs=""
        fi
        success_code="--success-codes ${success_codes[$bc_basename]}"
        if [ "${success_code}" = "--success-codes " ]; then
          success_code=""
        fi
        smart_success_code_zero="${smart_success_code_zeros[$bc_basename]}"
        out_graph_uri="${BASE_EMBED_DIR}$bc_basename.icfg"
        out_walks_uri="${BASE_EMBED_DIR}$bc_basename.walks"
        out_model_uri="${BASE_EMBED_DIR}$bc_basename.model"
        
        ${BAZEL} run //cli:main -- --db-name ${DBNAME}_evid${count}_${operation} bitcode \
          RegisterBitcode --uri $bc_uri
        if [ ! -f "${out_graph_uri#file:\/\/}" ]; then
          ${BAZEL} run //cli:main -- --db-name ${DBNAME}_evid${count}_${operation} \
              getgraph GetGraphUri --bitcode-uri $bc_uri \
              --output-graph-uri $out_graph_uri $error_code 
        fi
        if [ ! -f "${out_walks_uri#file:\/\/}" ]; then
          ${BAZEL} run //cli:main -- --db-name ${DBNAME}_evid${count}_${operation} \
              walker Walk --input-icfg-uri $out_graph_uri \
              --output-walks-uri $out_walks_uri
        fi
        if [ ! -f "${out_model_uri#file:\/\/}" ]; then
          ${BAZEL} run //cli:main -- --db-name ${DBNAME}_evid${count}_${operation} \
              embedding Train --walks-uri $out_walks_uri \
              --output-uri $out_model_uri --window 5
        fi
        ${BAZEL} run //cli:main -- --db-name ${DBNAME}_evid${count}_${operation} \
            embedding RegisterEmbedding --bitcode-uri $bc_uri \
            --embedding-uri $out_model_uri --overwrite
        ${BAZEL} run //cli:main -- --db-name ${DBNAME}_evid${count}_${operation} \
            bitcode GetCalledFunctionsUri --uri $bc_uri
        ${BAZEL} run //cli:main -- --db-name ${DBNAME}_evid${count}_${operation} \
            bitcode GetDefinedFunctionsUri --uri $bc_uri
        ${BAZEL} run //cli:main -- --db-name ${DBNAME}_evid${count}_${operation} \
            eesi GetSpecificationsUri --bitcode-uri $bc_uri $init_specs \
            $error_code $error_only $success_code $smart_success_code_zero \
            --use-embedding --min-synonym-evidence ${count} \
            --expansion-operation ${operation} ${OVERWRITE}
      done
    done
done
