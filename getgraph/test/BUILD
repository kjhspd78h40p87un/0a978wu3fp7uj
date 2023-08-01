cc_test(
    name = "get_graph_service_test",
    size = "small",
    srcs = ["get_graph_service_test.cc"],
    copts = ["-Iexternal/gtest/include"],
    data = [
        "//:testdata_bitcode",
    ],
    includes = ["include"],
    deps = [
        "//bitcode:service",
        "//common:servers",
        "//getgraph:service",
        "//proto:bitcode_cc_grpc",
        "//proto:get_graph_cc_grpc",
        "//proto:operations_cc_grpc",
        "@com_github_grpc_grpc//:grpc++",
        "@gtest//:main",
    ],
)
