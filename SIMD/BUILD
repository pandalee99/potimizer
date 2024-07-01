cc_binary(
    name = "simd",
    srcs = ["simd.cpp"],
    copts = ["-mavx2"],  # Enable AVX2 support
    linkopts = ["-mavx2"],  # Ensure linker also knows about AVX2
)
