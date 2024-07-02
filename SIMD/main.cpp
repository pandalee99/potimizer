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


// 创建一个查找表，用于将字符映射到其分类值
alignas(32) const char latin_lookup[32] = {
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // 0-15: 拉丁字符
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1  // 16-31: 拉丁字符
};

// 使用AVX2的vpshufb指令实现拉丁字符检测
bool isLatin_AVX2_vpshufb(const std::string& str) {
    const char* data = str.data();
    size_t len = str.size();

    // 用于查找的查表掩码
    __m256i mask = _mm256_setr_epi8(
            0, 1, 2, 3, 4, 5, 6, 7,
            8, 9, 10, 11, 12, 13, 14, 15,
            16, 17, 18, 19, 20, 21, 22, 23,
            24, 25, 26, 27, 28, 29, 30, 31
    );

    // 查找表
    __m256i latin_table = _mm256_load_si256(reinterpret_cast<const __m256i*>(latin_lookup));

    // 处理32个字符一组的数据
    size_t i = 0;
    for (; i + 32 <= len; i += 32) {
        // 加载32个字符到AVX2寄存器中
        __m256i chars = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(data + i));

        // 使用vpshufb进行查表操作
        __m256i result = _mm256_shuffle_epi8(latin_table, chars);

        // 检查结果，如果有任何字符不是拉丁字符，则返回false
        if (!_mm256_testz_si256(result, result)) {
            return false;
        }
    }

    // 处理剩余的字符
    for (; i < len; ++i) {
        if (static_cast<unsigned char>(data[i]) >= 128) {
            return false;
        }
    }

    return true;
}

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

int main() {
    // Generate a random Latin-only string
    std::string testStr = generateRandomString(100000);

    // Measure AVX2
    auto start_time = std::chrono::high_resolution_clock::now();
    bool result_avx2 = isLatin_AVX2(testStr);
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration_avx2 = std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - start_time).count();
    std::cout << "AVX2 Result: " << (result_avx2 ? "Latin-only" : "Not Latin-only") << std::endl;
    std::cout << "AVX2 Running Time: " << duration_avx2 << " ns" << std::endl;

    // Measure SSE2
    start_time = std::chrono::high_resolution_clock::now();
    bool result_sse2 = isLatin_SSE2(testStr);
    end_time = std::chrono::high_resolution_clock::now();
    auto duration_sse2 = std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - start_time).count();
    std::cout << "SSE2 Result: " << (result_sse2 ? "Latin-only" : "Not Latin-only") << std::endl;
    std::cout << "SSE2 Running Time: " << duration_sse2 << " ns" << std::endl;

    // Measure Baseline
    start_time = std::chrono::high_resolution_clock::now();
    bool result_baseline = isLatin_Baseline(testStr);
    end_time = std::chrono::high_resolution_clock::now();
    auto duration_baseline = std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - start_time).count();
    std::cout << "Baseline Result: " << (result_baseline ? "Latin-only" : "Not Latin-only") << std::endl;
    std::cout << "Baseline Running Time: " << duration_baseline << " ns" << std::endl;


    // 测量isLatin_AVX2_vpshufb的运行时间
     start_time = std::chrono::high_resolution_clock::now();
    bool result_avx2_vpshufb = isLatin_AVX2_vpshufb(testStr);
     end_time = std::chrono::high_resolution_clock::now();
    auto duration_avx2_vpshufb = std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - start_time).count();
    std::cout << "AVX2_vpshufb Result: " << (result_avx2_vpshufb ? "Latin-only" : "Not Latin-only") << std::endl;
    std::cout << "AVX2_vpshufb Running Time: " << duration_avx2_vpshufb << " ns" << std::endl;


    std::cin.get();
    return 0;
}
