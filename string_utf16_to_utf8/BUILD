cc_binary(
    name = "utf",
    srcs = ["main.cpp"],
        copts = ["-mavx2"],  # Enable AVX2 support
        linkopts = ["-mavx2"],  # Ensure linker also knows about AVX2
    deps = [

    "@com_github_google_benchmark//:benchmark",
    ],
)
