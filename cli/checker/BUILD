py_library(
    name = "args",
    srcs = ["args.py"],
    visibility = ["//cli:__pkg__"],
    deps = [
        ":commands",
        "//cli/common:service_configuration_handler",
        "//cli/db",
    ],
)

py_library(
    name = "commands",
    srcs = ["commands.py"],
    visibility = ["//cli/test/checker:__pkg__"],
    deps = [
        ":db",
        ":rpc",
        ":violation_filters",
        "//cli/bitcode:db",
        "//cli/bitcode:rpc",
        "//cli/common:log",
        "//cli/common:uri",
        "//cli/eesi:db",
        "//proto:checker_py_proto",
    ],
)

py_library(
    name = "db",
    srcs = ["db.py"],
    deps = [
        "//cli/db",
        "//proto:checker_py_proto",
    ],
)

py_library(
    name = "rpc",
    srcs = ["rpc.py"],
    deps = [
        ":db",
        "//cli/operations:wait",
    ],
)

py_library(
    name = "violation_filters",
    srcs = ["violation_filters.py"],
    visibility = ["//cli/test/checker:__pkg__"],
    deps = [
        "//proto:bitcode_py_proto",
        "//proto:eesi_py_proto",
    ],
)
