#include <iostream>
#include <immintrin.h> // AVX2 header
#include <emmintrin.h> // SSE2 header
#include <chrono>
#include <string>
#include <random>

// AVX2 implementation
bool isLatin_AVX2(const std::string& str) {
    const char* data = str.data();
    size_t len = str.size();

    // Process 32 characters at a time with AVX2
    size_t i = 0;
    __m256i latin_mask = _mm256_set1_epi8(0x80);
    for (; i + 32 <= len; i += 32) {
        __m256i chars = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(data + i));
        __m256i result = _mm256_and_si256(chars, latin_mask);
        if (!_mm256_testz_si256(result, result)) {
            return false;
        }
    }

    // Process remaining characters
    for (; i < len; ++i) {
        if (static_cast<unsigned char>(data[i]) >= 128) {
            return false;
        }
    }

    return true;
}

// SSE2 implementation
bool isLatin_SSE2(const std::string& str) {
    const char* data = str.data();
    size_t len = str.size();

    // Process 16 characters at a time with SSE2
    size_t i = 0;
    __m128i latin_mask = _mm_set1_epi8(0x80);
    for (; i + 16 <= len; i += 16) {
        __m128i chars = _mm_loadu_si128(reinterpret_cast<const __m128i*>(data + i));
        __m128i result = _mm_and_si128(chars, latin_mask);
        if (!_mm_testz_si128(result, result)) {
            return false;
        }
    }

    // Process remaining characters
    for (; i < len; ++i) {
        if (static_cast<unsigned char>(data[i]) >= 128) {
            return false;
        }
    }

    return true;
}

// Baseline implementation
bool isLatin_Baseline(const std::string& str) {
    for (char c : str) {
        if (static_cast<unsigned char>(c) >= 128) {
            return false;
        }
    }
    return true;
}








alignas(32) const uint8_t latin_lookup[32] = {
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1
};


// Random string generator
std::string generateRandomString(size_t length) {
    const char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    std::default_random_engine rng(std::random_device{}());
    std::uniform_int_distribution<> dist(0, sizeof(charset) - 2); // size-2 to exclude null terminator

    std::string result;
    result.reserve(length);
    for (size_t i = 0; i < length; ++i) {
        result += charset[dist(rng)];
    }

    return result;
}


bool isLatin_AVX512(const std::string& str) {
    const char* data = str.data();
    size_t len = str.size();

    // Process 64 characters at a time with AVX-512
    size_t i = 0;
    __m512i latin_mask = _mm512_set1_epi8(0x80);
    for (; i + 64 <= len; i += 64) {
        __m512i chars = _mm512_loadu_si512(reinterpret_cast<const __m512i*>(data + i));
        __mmask64 result_mask = _mm512_test_epi8_mask(chars, latin_mask);
        if (result_mask != 0) {
            return false;
        }
    }

    // Process remaining characters
    for (; i < len; ++i) {
        if (static_cast<unsigned char>(data[i]) >= 128) {
            return false;
        }
    }

    return true;
}

int main() {
    // Generate a random Latin-only string
    std::string testStr = generateRandomString(100000);

    // Measure Baseline
    auto start_time = std::chrono::high_resolution_clock::now();
    bool result_baseline = isLatin_Baseline(testStr);
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration_baseline = std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - start_time).count();
    std::cout << "Baseline Result: " << (result_baseline ? "Latin-only" : "Not Latin-only") << std::endl;
    std::cout << "Baseline Running Time: " << duration_baseline << " ns" << std::endl;

    // Measure SSE2
    start_time = std::chrono::high_resolution_clock::now();
    bool result_sse2 = isLatin_SSE2(testStr);
    end_time = std::chrono::high_resolution_clock::now();
    auto duration_sse2 = std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - start_time).count();
    std::cout << "SSE2 Result: " << (result_sse2 ? "Latin-only" : "Not Latin-only") << std::endl;
    std::cout << "SSE2 Running Time: " << duration_sse2 << " ns" << std::endl;

    // Measure AVX2
     start_time = std::chrono::high_resolution_clock::now();
    bool result_avx2 = isLatin_AVX2(testStr);
     end_time = std::chrono::high_resolution_clock::now();
    auto duration_avx2 = std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - start_time).count();
    std::cout << "AVX2 Result: " << (result_avx2 ? "Latin-only" : "Not Latin-only") << std::endl;
    std::cout << "AVX2 Running Time: " << duration_avx2 << " ns" << std::endl;








    std::cin.get();
    return 0;
}
