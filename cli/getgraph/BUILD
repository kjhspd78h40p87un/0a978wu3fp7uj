py_library(
    name = "args",
    srcs = ["args.py"],
    visibility = [
        "//cli:__pkg__",
    ],
    deps = [
        ":commands",
        "//cli/common:service_configuration_handler",
        "//cli/db",
        "//cli/eesi:domain_knowledge_handler",
    ],
)

py_library(
    name = "commands",
    srcs = ["commands.py"],
    visibility = [
        "//cli/test/getgraph:__pkg__",
    ],
    deps = [
        ":rpc",
        "//cli/bitcode:db",
        "//cli/bitcode:rpc",
        "//cli/common:log",
        "//cli/common:uri",
        "//cli/db",
    ],
)

py_library(
    name = "rpc",
    srcs = ["rpc.py"],
    deps = [
        "//cli/operations:wait",
        "//proto:get_graph_py_proto",
    ],
)
