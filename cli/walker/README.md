# Command-line Interface: Walker

## Commands

These commands are related to walking over labels in the interprocedural
control-flow graph (ICFG) representation of programs. These walks are performed
randomly and ultimately saved out to some desired file. To read about generating
ICFGs go [here](../getgraph/README.md).

### Walker

Currently, the only commands supported are for performing walks over single
ICFG files. In the future, we would like to make it easier to perform walks over
entire directories of ICFGs.

#### Walk

To perform random walks over a single ICFG file, you simply must supply the URI
of the ICFG file using the `--input-icfg-uri` flag. You also must supply a
desired output URI for the file containing the random walks using the
`--output-walks-uri` flag.

The optional flag `--walks-per-label` affects the number of walks that are
performed starting at each label (default 100). The optional flag
`--walk-length` affects the number of labels to walk over before the random
walk is completed (default 100).

Example:

```bash

bazel run //cli:main -- walker Walk --input-icfg-uri \
    file:///<PATH_TO_ICFG> --output-walks-uri file:///<PATH_TO_OUTPUT> \
    --walks-per-label 50 --walk-length 50
```
