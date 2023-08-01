# The initial setup, registering, called functions, defined functions,
# and registering model.

if [ "$#" -ne 1 ]; then
    echo "Usage: ./benchmark_results.sh <output-directory-path>"
    exit 1
fi

declare -a similarities=(0.5 0.7 0.9)
declare -a evidence_counts=(3 9)
declare -a dk_configs=(all initial_specifications error_codes error_only)
output_directory=$(readlink -m $1)

for dk_config in ${dk_configs[@]}; do
    bazel run //cli:main -- --db-name no_embedding_${dk_config} eesi SpecificationsTableToCsv --output-path $output_directory/no_embedding_${dk_config}.csv 
    for similarity in ${similarities[@]}; do
        stripped_similarity=${similarity//./}
        for evidence_count in ${evidence_counts[@]}; do
            bazel run //cli:main -- --db-name evidence${evidence_count}_similarity${stripped_similarity}_${dk_config} eesi SpecificationsTableToCsv --output-path $output_directory/evidence${evidence_count}_similarity${stripped_similarity}_${dk_config}.csv 
        done
    done
done
