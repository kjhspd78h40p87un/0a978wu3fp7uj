py_library(
    name = "args",
    srcs = ["args.py"],
    visibility = [
        "//cli:__pkg__",
    ],
    deps = [
        ":commands",
        "//cli/common:service_configuration_handler",
    ],
)

py_library(
    name = "commands",
    srcs = ["commands.py"],
    visibility = [
        "//cli/test/walker:__pkg__",
    ],
    deps = [
        ":rpc",
        "//cli/common:log",
        "//cli/common:uri",
    ],
)

py_library(
    name = "rpc",
    srcs = ["rpc.py"],
    deps = [
        "//cli/operations:wait",
        "//proto:walker_py_proto",
    ],
)
