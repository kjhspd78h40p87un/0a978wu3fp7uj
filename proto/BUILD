package(
    default_visibility = ["//visibility:public"],
)

load("@com_github_grpc_grpc//bazel:cc_grpc_library.bzl", "cc_grpc_library")
load("@com_github_grpc_grpc//bazel:python_rules.bzl", "py_grpc_library", "py_proto_library")

proto_library(
    name = "bitcode_proto",
    srcs = ["bitcode.proto"],
    deps = [":operations_proto"],
)

cc_proto_library(
    name = "bitcode_cc_proto",
    deps = ["bitcode_proto"],
)

# See https://github.com/grpc/grpc/pull/18955
cc_grpc_library(
    name = "bitcode_cc_grpc",
    srcs = [":bitcode_proto"],
    grpc_only = True,
    deps = [":bitcode_cc_proto"],
)

py_proto_library(
    name = "bitcode_py_proto",
    deps = [":bitcode_proto"],
)

py_grpc_library(
    name = "bitcode_py_grpc",
    srcs = [":bitcode_proto"],
    deps = [":bitcode_py_proto"],
)

proto_library(
    name = "domain_knowledge_proto",
    srcs = ["domain_knowledge.proto"],
    deps = [":bitcode_proto"],
)

cc_proto_library(
    name = "domain_knowledge_cc_proto",
    deps = ["domain_knowledge_proto"],
)

py_proto_library(
    name = "domain_knowledge_py_proto",
    deps = [":domain_knowledge_proto"],
)

proto_library(
    name = "eesi_proto",
    srcs = ["eesi.proto"],
    deps = [
        ":bitcode_proto",
        ":domain_knowledge_proto",
        ":operations_proto",
    ],
)

py_proto_library(
    name = "eesi_py_proto",
    deps = [":eesi_proto"],
)

py_grpc_library(
    name = "eesi_py_grpc",
    srcs = [":eesi_proto"],
    deps = [":eesi_py_proto"],
)

cc_proto_library(
    name = "eesi_cc_proto",
    deps = ["eesi_proto"],
)

cc_grpc_library(
    name = "eesi_cc_grpc",
    srcs = [":eesi_proto"],
    grpc_only = True,
    deps = [":eesi_cc_proto"],
)

proto_library(
    name = "ehnfer_proto",
    srcs = ["ehnfer.proto"],
    deps = [
        ":bitcode_proto",
        ":mining_proto",
        ":operations_proto",
    ],
)

proto_library(
    name = "checker_proto",
    srcs = ["checker.proto"],
    deps = [
        ":bitcode_proto",
        ":eesi_proto",
        ":ehnfer_proto",
        ":operations_proto",
    ],
)

cc_proto_library(
    name = "checker_cc_proto",
    deps = ["checker_proto"],
)

# See https://github.com/grpc/grpc/pull/18955
cc_grpc_library(
    name = "checker_cc_grpc",
    srcs = [":checker_proto"],
    grpc_only = True,
    deps = [":checker_cc_proto"],
)

py_proto_library(
    name = "checker_py_proto",
    deps = [":checker_proto"],
)

py_grpc_library(
    name = "checker_py_grpc",
    srcs = [":checker_proto"],
    deps = [":checker_py_proto"],
)

proto_library(
    name = "mining_proto",
    srcs = ["mining.proto"],
    deps = [
        ":bitcode_proto",
        ":eesi_proto",
        ":operations_proto",
    ],
)

py_proto_library(
    name = "ehnfer_py_proto",
    deps = [":ehnfer_proto"],
)

py_grpc_library(
    name = "ehnfer_py_grpc",
    srcs = [":ehnfer_proto"],
    deps = [":ehnfer_py_proto"],
)

proto_library(
    name = "get_graph_proto",
    srcs = ["get_graph.proto"],
    deps = [
        ":bitcode_proto",
        ":domain_knowledge_proto",
        ":operations_proto",
    ],
)

py_proto_library(
    name = "get_graph_py_proto",
    deps = [":get_graph_proto"],
)

py_grpc_library(
    name = "get_graph_py_grpc",
    srcs = [":get_graph_proto"],
    deps = [":get_graph_py_proto"],
)

cc_proto_library(
    name = "get_graph_cc_proto",
    deps = ["get_graph_proto"],
)

cc_grpc_library(
    name = "get_graph_cc_grpc",
    srcs = [":get_graph_proto"],
    grpc_only = True,
    deps = [":get_graph_cc_proto"],
)

proto_library(
    name = "walker_proto",
    srcs = ["walker.proto"],
    deps = [
        ":get_graph_proto",
        ":operations_proto",
    ],
)

cc_proto_library(
    name = "walker_cc_proto",
    deps = ["walker_proto"],
)

cc_grpc_library(
    name = "walker_cc_grpc",
    srcs = [":walker_proto"],
    grpc_only = True,
    deps = [":walker_cc_proto"],
)

py_proto_library(
    name = "walker_py_proto",
    deps = [":walker_proto"],
)

py_grpc_library(
    name = "walker_py_grpc",
    srcs = [":walker_proto"],
    deps = [":walker_py_proto"],
)

proto_library(
    name = "func2vec_legacy_proto",
    srcs = ["func2vec_legacy.proto"],
)

cc_proto_library(
    name = "func2vec_legacy_cc_proto",
    deps = ["func2vec_legacy_proto"],
)

proto_library(
    name = "embedding_proto",
    srcs = ["embedding.proto"],
    deps = [
        ":get_graph_proto",
        ":operations_proto",
        ":walker_proto",
    ],
)

py_proto_library(
    name = "embedding_py_proto",
    deps = ["embedding_proto"],
)

py_grpc_library(
    name = "embedding_py_grpc",
    srcs = [":embedding_proto"],
    deps = [":embedding_py_proto"],
)

cc_proto_library(
    name = "embedding_cc_proto",
    deps = ["embedding_proto"],
)

cc_grpc_library(
    name = "embedding_cc_grpc",
    srcs = [":embedding_proto"],
    grpc_only = True,
    deps = ["embedding_cc_proto"],
)

proto_library(
    name = "status_proto",
    srcs = ["status.proto"],
    deps = [
        "@com_google_protobuf//:any_proto",
    ],
)

py_proto_library(
    name = "status_py_proto",
    deps = [":status_proto"],
)

py_grpc_library(
    name = "status_py_grpc",
    srcs = [":status_proto"],
    deps = [":status_py_proto"],
)

proto_library(
    name = "operations_proto",
    srcs = ["operations.proto"],
    deps = [
        ":status_proto",
        "@com_google_protobuf//:any_proto",
        "@com_google_protobuf//:empty_proto",
    ],
)

py_proto_library(
    name = "operations_py_proto",
    deps = [":operations_proto"],
)

py_grpc_library(
    name = "operations_py_grpc",
    srcs = [":operations_proto"],
    deps = [":operations_py_proto"],
)

cc_proto_library(
    name = "operations_cc_proto",
    deps = ["operations_proto"],
)

cc_grpc_library(
    name = "operations_cc_grpc",
    srcs = [":operations_proto"],
    grpc_only = True,
    deps = [":operations_cc_proto"],
)
