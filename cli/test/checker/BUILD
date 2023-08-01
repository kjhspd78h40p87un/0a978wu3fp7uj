py_test(
    name = "test_get_violations",
    srcs = ["test_get_violations.py"],
    data = [
        "//:testdata_bitcode",
    ],
    deps = [
        ":helpers",
        "//cli/bitcode:commands",
        "//cli/checker:commands",
        "//cli/common:service_configuration_handler",
        "//cli/test/common",
        "//proto:eesi_py_proto",
    ],
)

py_test(
    name = "test_violation_filters",
    srcs = ["test_violation_filters.py"],
    data = [
        "//:testdata_sourcecode",
    ],
    deps = [
        "//cli/checker:violation_filters",
        "//proto:bitcode_py_proto",
        "//proto:domain_knowledge_py_proto",
        "//proto:eesi_py_proto",
        "//proto:operations_py_proto",
        "//proto:status_py_proto",
    ],
)

py_library(
    name = "helpers",
    srcs = ["helpers.py"],
)
