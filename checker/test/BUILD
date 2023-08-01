cc_test(
    name = "unused_calls_test",
    size = "small",
    srcs = ["unused_calls_test.cc"],
    copts = ["-Iexternal/gtest/include"],
    data = [
        "//:testdata_bitcode",
    ],
    deps = [
        "//checker:unused_calls_pass",
        "//proto:checker_cc_grpc",
        "@com_github_grpc_grpc//:grpc++",
        "@gtest//:main",
        "@org_llvm//:LLVMIRReader",
    ],
)

cc_test(
    name = "insufficient_checks_test",
    size = "small",
    srcs = ["insufficient_checks_test.cc"],
    copts = ["-Iexternal/gtest/include"],
    data = [
        "//:testdata_bitcode",
    ],
    deps = [
        "//checker:insufficient_checks_pass",
        "//proto:checker_cc_grpc",
        "@com_github_grpc_grpc//:grpc++",
        "@gtest//:main",
        "@org_llvm//:LLVMIRReader",
    ],
)
