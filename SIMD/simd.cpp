//
// Created by pandalee on 2024/7/1.
//

#include "simd.h"

//
// Created by pandalee on 2024/6/30.
//

#ifndef CPP_SIMD__H
#define CPP_SIMD__H


#include <iostream>
#include <immintrin.h> // AVX2 header

bool isLatin(const std::string& str) {
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

int main() {
    std::string testStr = "Hello, World!"; // This is a Latin-only string
    bool result = isLatin(testStr);
    std::cout << "The string \"" << testStr << "\" is " << (result ? "Latin-only." : "not Latin-only.") << std::endl;

    testStr = "こんにちは、世界！"; // This string contains non-Latin characters
    result = isLatin(testStr);
    std::cout << "The string \"" << testStr << "\" is " << (result ? "Latin-only." : "not Latin-only.") << std::endl;

    return 0;
}



#endif //CPP_SIMD__H
