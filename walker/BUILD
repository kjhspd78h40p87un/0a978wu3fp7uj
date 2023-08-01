cc_library(
    name = "lpds",
    srcs = [
        "include/lpds.h",
        "src/lpds.cc",
    ],
    includes = ["include"],
    visibility = ["//walker/test:__pkg__"],
    deps = [
        "//proto:walker_cc_grpc",
        "@com_github_01org_tbb//:tbb",
        "@com_github_google_glog//:glog",
    ],
)

cc_library(
    name = "walker",
    srcs = [
        "include/walker.h",
        "src/walker.cc",
    ],
    includes = ["include"],
    visibility = [
        "//walker/test:__pkg__",
    ],
    deps = [
        "lpds",
        "//common:servers",
        "//proto:func2vec_legacy_cc_proto",
        "//proto:walker_cc_grpc",
        "@com_github_01org_tbb//:tbb",
        "@com_github_google_glog//:glog",
        "@com_github_googleapis_google_cloud_cpp//google/cloud/storage:storage_client",
    ],
)

cc_library(
    name = "service",
    srcs = [
        "src/walker_server.cc",
    ],
    hdrs = [
        "include/walker_server.h",
    ],
    includes = ["include"],
    visibility = [
        "//walker/test:__pkg__",
    ],
    deps = [
        "walker",
        "//common:operations",
        "//common:servers",
        "//proto:operations_cc_grpc",
        "//proto:walker_cc_grpc",
        "@com_github_01org_tbb//:tbb",
    ],
)

cc_binary(
    name = "main",
    srcs = [
        "src/main.cc",
    ],
    deps = [
        ":service",
        "//common:servers",
        "@com_github_google_glog//:glog",
        "@com_google_absl//absl/flags:flag",
        "@com_google_absl//absl/flags:parse",
    ],
)

py_binary(
    name = "cli",
    srcs = ["src/cli.py"],
    deps = [
        "//proto:bitcode_py_proto",
        "//proto:get_graph_py_proto",
        "//proto:operations_py_grpc",
        "//proto:status_py_grpc",
        "//proto:walker_py_grpc",
    ],
)
