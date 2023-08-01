SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
STAT_DIR=$(cd ${SCRIPT_DIR}/../testdata/benchmarks/stats/evid3 && pwd)
for o in "meet" "join" "max"; do
    echo "---Operator ${o}---"
    awk 'BEGIN { printf "%-50s %15s %15s %20s %20s\n", "File", "Precision", "Recall", "Unique Precision", "3rd Precision"}'
    for f in "$STAT_DIR"/${o}/*-1-stats.txt; do
        precision="$(grep "\bPrecision:" "$f" | awk '{print $NF}')"
        recall="$(grep "\bRecall:" "$f" | awk '{print $NF}')"
        delta="$(grep "\bDelta Precision over baseline:" "$f" | awk '{print $NF}')"
        tp="$(grep "\bpartyPrecision:" "$f" | awk '{print $NF}')"
        awk -v p="$precision" -v r="$recall" -v fs="$delta" -v tp=$tp -v bfname="$(basename $f)" \
            'BEGIN { printf "%-50s %15s %15s %20s %20s\n", bfname, p, r, fs, tp}'
    done
done
echo "---Baseline Analysis---"
awk 'BEGIN { printf "%-50s %15s %15s \n", "File", "Precision", "Recall"}'
for f in "$STAT_DIR"/max/*-100-stats.txt; do
    precision="$(grep "\bPrecision:" "$f" | awk '{print $NF}')"
    recall="$(grep "\bRecall:" "$f" | awk '{print $NF}')"
    awk -v p="$precision" -v r="$recall" -v bfname="$(basename $f)" \
        'BEGIN { printf "%-50s %15s %15s \n", bfname, p, r}'
done
