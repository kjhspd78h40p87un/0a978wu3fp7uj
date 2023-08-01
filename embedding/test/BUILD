py_test(
    name = "embedding_service_test",
    srcs = ["embedding_service_test.py"],
    data = [
        "//:testdata_bitcode",
    ],
    tags = ["exclusive"],
    deps = [
        "//embedding:service",
        "//proto:domain_knowledge_py_proto",
        "//proto:embedding_py_grpc",
        "//proto:embedding_py_proto",
        "//proto:get_graph_py_proto",
    ],
)
