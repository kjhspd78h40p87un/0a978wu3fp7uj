SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
BASE_BC_DIR="file://$(cd ${SCRIPT_DIR}/../testdata/benchmarks/bitcode; pwd)/"
GT_DIR=$( cd ${SCRIPT_DIR}/../testdata/benchmarks/ground_truth; pwd )
DK_DIR=$( cd ${SCRIPT_DIR}/../testdata/benchmarks/domain_knowledge; pwd )

mkdir ${SCRIPT_DIR}/../third_party/eesi/results/artifact/stats
for prj in "zlib" "pidgin" "littlefs" "netdata" "mbedtls" "openssl"; do
  bazel run //cli:main -- --db-name eesier_evid3 eesi ListStatisticsFile \
      --specifications ${SCRIPT_DIR}/../third_party/eesi/results/artifact/${prj}-specs.txt \
      --ground-truth-path ${GT_DIR}/${prj}-gt.txt \
      --initial-specifications-path ${DK_DIR}/input-specs-${prj}.txt \
      > ${SCRIPT_DIR}/../third_party/eesi/results/artifact/stats/${prj}-stats.txt
done
