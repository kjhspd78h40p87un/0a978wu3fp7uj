## Benchmarks

### benchmarks.sh
This command runs multiple GetSpecifications configurations. These 
configurations include multiple domain knowledge configurations, 
multiple synonym evidence values, and multiple synonym similarity values.
This script is not anything special and expects all types of domain
knowledge to be supplied, however, you can pass a blank file and it will
count the same as not using that type of domain knowledge. All supplied
paths must be absolute paths. Usage:

```bash
benchmarks.sh <BITCODE_URI> <EMBEDDING_URI> <INITIAL_SPECIFICATIONS_PATH>\
              <ERROR_CODES_PATH> <ERROR_ONLY_PATH> [--overwrite]
```

### benchmark_results.sh

This command dumps out the specifications tables for all databases used
in `benchmarks.sh` in a directory supplied by the user. Usage:

```bash
benchmark_results.sh <ABSOLUTE_PATH_TO_DIRECTORY>
```

### tsne_plotter_ground_truth.py

This command generates a TSNE plot for an function embeddings with each function
being highlighted according to the ground-truth specification.

```bash
python3 tsne_plotter_ground_truth.py [-h] --model <MODEL_PATH>
                                     --ground-truth-path <GROUND_TRUTH_PATH>
                                     [--perplexity <PERPLEXITY>]
                                     [--learning-rate <LEARNING_RATE>]
                                     --output-file <OUTPUT_FILE>
```

The `--model` argument must be a path that points to a model that contains
the vectorized representations of functions. The `--ground-truth-path` argument
is a path to a file that contains the ground-truth for specifications of
functions. Note: This does not need to be all functions contained in an
embedding, it is okay to only include a subset. The format should be follow
the example:

```
foo <0
bar >0
baz bottom
```

The `--perplexity` and `--learning-rate` parameters are specific to the TSNE
algorithm. To learn more about what these mean, you can read the 
[scikit-learn documentation](https://scikit-learn.org/stable/modules/generated/sklearn.manifold.TSNE.html).
