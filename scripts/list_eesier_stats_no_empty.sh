SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
BASE_BC_DIR="file://$(cd ${SCRIPT_DIR}/../testdata/benchmarks/bitcode; pwd)/"
GT_DIR=$( cd ${SCRIPT_DIR}/../testdata/benchmarks/ground_truth; pwd )
evidence_counts=( 3 )

mkdir ${SCRIPT_DIR}/../testdata/benchmarks/stats/no_emptyset
for e_count in ${evidence_counts[@]}; do
    for o in "meet"; do
        # Iterating through the thresholds like this is annoying and redundant, but
        # I do not want to change this ATM.
        for t in 100; do
            for prj in "zlib" "pidgin" "littlefs" "netdata" "mbedtls" "openssl"; do
              bazel run //cli:main -- --db-name eesier_evid${e_count}_${o} eesi ListStatisticsDatabase \
                  --bitcode-uri ${BASE_BC_DIR}${prj}-reg2mem.bc \
                  --ground-truth-path ${GT_DIR}/${prj}-gt.txt \
                  --confidence-threshold ${t} \
                  > ${SCRIPT_DIR}/../testdata/benchmarks/stats/no_emptyset/${prj}-${t}-stats.txt
            done
        done
    done
done
