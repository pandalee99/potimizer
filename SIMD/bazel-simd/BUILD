cc_library(
    name = "simd_lib",
    srcs = ["simd.cpp"],
    hdrs = ["simd.h"],
    copts = ["-mavx2"],  # Enable AVX2 support
)

cc_binary(
    name = "simd",
    srcs = ["main.cpp"],
    deps = [":simd_lib"],
    copts = ["-mavx2"],  # Enable AVX2 support
    linkopts = ["-mavx2"],  # Ensure linker also knows about AVX2
)
