# Command-line Interface: GetGraph

## Commands

These commands are related to generating interprocedural control-flow graph
(ICFG) representations of programs given some bitcode file. For these commands
to be successful, the bitcode file must already be registered with the bitcode
service and database. To view the commands associated with these requirements,
go [here](../bitcode/README.md).

### GetGraph

#### GetGraph for a single bitcode file

To generate a graph for a single bitcode file, you must supply the bitcode
URI of the file to generate a graph for, along with the URI of the output
graph file. You may also pass optional error codes for consideration when
generating the graph using the option `--error-codes`.

Example:

```bash

bazel run //cli:main -- getgraph GetGraphUri --bitcode-uri \
    file:///<PATH_TO_BITCODE> --output-graph-uri file:///<PATH_TO_OUTPUT> \
    --error-codes <PATH_TO_ERROR_CODES>
```

#### GetGraph for all bitcode files registered

To generate a graph for all bitcode files registered with the service/database,
you must use the `GetGraphAll` command. This automatically generates a graph
for each bitcode file registered in the database, generating graph files that
share the same name as the bitcode file with `.icfg` appended. You may also
pass optional error codes for consideration when generating the graph using
the option `--error-codes`.

Example:


```bash
bazel run //cli:main -- getgraph GetGraphAll --error-codes <PATH_TO_ERROR_CODES>
```
