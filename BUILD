load("@bazel_python//:bazel_python.bzl", "bazel_python_interpreter")

bazel_python_interpreter(
    python_version = "3.7.4",
    requirements_file = "requirements.txt",
)

filegroup(
    name = "testdata_bitcode",
    srcs = glob([
        "testdata/programs/*.ll",
        "testdata/embeddings/*.walks",
        "testdata/embeddings/*.embedding",
        "testdata/domain_knowledge/*.txt",
        "testdata/external/bitcode/*.bc",
    ]),
    visibility = [
        "//bitcode/test:__pkg__",
        "//checker/test:__pkg__",
        "//cli/test/bitcode:__pkg__",
        "//cli/test/checker:__pkg__",
        "//cli/test/common:__pkg__",
        "//cli/test/eesi:__pkg__",
        "//eesi/test:__pkg__",
        "//embedding/test:__pkg__",
        "//getgraph/test:__pkg__",
    ],
)

# Limiting the amount of test programs available to the CLI tests, otherwise
# unusual behavior occurs only in the bazel test environment.
filegroup(
    name = "cli_testdata_bitcode",
    srcs = glob([
        "testdata/domain_knowledge/*.txt",
        "testdata/programs/fopen.ll",
        "testdata/programs/fopen-reg2mem.ll",
        "testdata/programs/error_only_function_ptr.ll",
        "testdata/programs/error_only_function_ptr-reg2mem.ll",
        "testdata/programs/multireturn.ll",
        "testdata/programs/multireturn-reg2mem.ll",
        "testdata/programs/hello_twice.ll",
        "testdata/programs/hello_twice-reg2mem.ll",
    ]),
    visibility = [
        "//cli/test/bitcode:__pkg__",
        "//cli/test/checker:__pkg__",
        "//cli/test/common:__pkg__",
        "//cli/test/eesi:__pkg__",
        "//cli/test/getgraph:__pkg__",
    ],
)

filegroup(
    name = "testdata_sourcecode",
    srcs = glob([
        "testdata/programs/*.c",
    ]),
    visibility = [
        "//cli/test/checker:__pkg__",
    ],
)
