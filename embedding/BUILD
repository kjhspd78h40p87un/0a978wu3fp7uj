py_binary(
    name = "service",
    srcs = ["service.py"],
    visibility = ["//embedding/test:__pkg__"],
    deps = [
        "//proto:bitcode_py_proto",
        "//proto:domain_knowledge_py_proto",
        "//proto:embedding_py_grpc",
        "//proto:get_graph_py_proto",
        "//proto:operations_py_proto",
        "//proto:status_py_proto",
    ],
)
