SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
STAT_DIR=$(cd ${SCRIPT_DIR}/../third_party/eesi/results/artifact/stats && pwd)
awk 'BEGIN { printf "%-30s %10s %10s %10s\n", "File", "Precision", "Recall", "F-score"}'
for f in "$STAT_DIR"/*-stats.txt; do
    precision="$(grep "\bPrecision:" "$f" | awk '{print $NF}')"
    recall="$(grep "\bRecall:" "$f" | awk '{print $NF}')"
    f1="$(grep "\bF1:" "$f" | awk '{print $NF}')"
    awk -v p="$precision" -v r="$recall" -v fs="$f1" -v bfname="$(basename $f)" \
        'BEGIN { printf "%-30s %10s %10s %10s\n", bfname, p, r, fs}'
done
