cc_library(
    name = "get_graph_llvm_passes",
    srcs = [
        "include/control_flow_pass.h",
        "include/flow_graph.h",
        "include/instruction_labels_pass.h",
        "include/location.h",
        "include/names_pass.h",
        "include/utility.h",
        "include/var_name.h",
        "src/control_flow_pass.cc",
        "src/instruction_labels_pass.cc",
        "src/names_pass.cc",
        "src/utility.cc",
    ],
    includes = ["include"],
    deps = [
        "//common:llvm",
        "//eesi:eesi_llvm_passes",
        "//proto:domain_knowledge_cc_proto",
        "//proto:get_graph_cc_grpc",
        "@com_github_google_glog//:glog",
        "@org_llvm//:LLVMAnalysis",
        "@org_llvm//:LLVMCore",
        "@org_llvm//:LLVMIRReader",
        "@org_llvm//:LLVMSupport",
    ],
)

cc_library(
    name = "service",
    srcs = [
        "src/get_graph_server.cc",
    ],
    hdrs = [
        "include/get_graph_server.h",
    ],
    includes = ["include"],
    visibility = ["//getgraph/test:__pkg__"],
    deps = [
        ":get_graph_llvm_passes",
        "//common:llvm",
        "//common:operations",
        "//common:servers",
        "//proto:get_graph_cc_grpc",
        "@com_github_01org_tbb//:tbb",
        "@org_llvm//:LLVMAnalysis",
        "@org_llvm//:LLVMBitWriter",
        "@org_llvm//:LLVMCore",
        "@org_llvm//:LLVMIRReader",
        "@org_llvm//:LLVMSupport",
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
