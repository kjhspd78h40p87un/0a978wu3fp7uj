cc_library(
    name = "checker_common",
    srcs = [
        "include/checker_common.h",
        "src/checker_common.cc",
    ],
    includes = ["include"],
    deps = [
        "//proto:checker_cc_grpc",
        "//eesi:eesi_llvm_passes",
        "@com_github_google_glog//:glog",
    ],
)

cc_library(
    name = "unused_calls_pass",
    srcs = [
        "include/unused_calls_pass.h",
        "src/unused_calls_pass.cc",
    ],
    visibility = ["//checker/test:__pkg__"],
    deps = [
        "checker_common",
        "//common:llvm",
        "//proto:checker_cc_grpc",
    ],
)

cc_library(
    name = "insufficient_checks_pass",
    srcs = [
        "include/insufficient_checks_pass.h",
        "src/insufficient_checks_pass.cc",
    ],
    includes = ["include"],
    visibility = ["//checker/test:__pkg__"],
    deps = [
        "checker_common",
        "//common:llvm",
        "//eesi:eesi_llvm_passes",
        "//proto:checker_cc_grpc",
    ],
)

cc_library(
    name = "service",
    srcs = [
        "src/checker_server.cc",
    ],
    hdrs = [
        "include/checker_server.h",
    ],
    includes = ["include"],
    visibility = ["//checker/test:__pkg__"],
    deps = [
        ":insufficient_checks_pass",
        ":unused_calls_pass",
        "//common:llvm",
        "//common:operations",
        "//common:servers",
        "@com_github_01org_tbb//:tbb",
        "@com_github_google_glog//:glog",
        "@org_llvm//:LLVMBitWriter",
        "@org_llvm//:LLVMIRReader",
    ],
)

cc_binary(
    name = "main",
    srcs = [
        "src/main.cc",
    ],
    visibility = ["//cli/test/common:__pkg__"],
    deps = [
        ":service",
        "//common:servers",
        "@com_github_google_glog//:glog",
        "@com_google_absl//absl/flags:flag",
        "@com_google_absl//absl/flags:parse",
    ],
)
