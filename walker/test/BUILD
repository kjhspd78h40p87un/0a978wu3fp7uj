cc_test(
    name = "lpds_test",
    srcs = ["lpds_test.cc"],
    copts = ["-Iexternal/gtest/include"],
    includes = ["include"],
    deps = [
        "//walker:lpds",
        "@com_github_grpc_grpc//:grpc++",
        "@gtest//:main",
    ],
)

cc_test(
    name = "walker_test",
    srcs = ["walker_test.cc"],
    copts = ["-Iexternal/gtest/include"],
    flaky = 1,
    includes = ["include"],
    deps = [
        "//walker",
        "//walker:lpds",
        "@com_github_grpc_grpc//:grpc++",
        "@gtest//:main",
    ],
)
