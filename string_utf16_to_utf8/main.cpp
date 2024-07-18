#include <iostream>
#include <vector>
#include <string>
#include <random>
#include <chrono>
#include <stdexcept>
#include <algorithm>
#include <immintrin.h>
#include <locale>
#include <codecvt>

// Convert UTF-16 encoded string to UTF-8 encoded string without using AVX2
std::string utf16_to_utf8(const std::u16string &utf16, bool is_little_endian) {
    std::string utf8;

    for (size_t i = 0; i < utf16.size(); ++i) {
        uint16_t w1 = utf16[i];
        if (!is_little_endian) {
            w1 = (w1 >> 8) | (w1 << 8); // Swap bytes for big endian
        }

        if (w1 >= 0xD800 && w1 <= 0xDBFF) {
            if (i + 1 >= utf16.size()) {
                throw std::runtime_error("Invalid UTF-16 sequence");
            }

            uint16_t w2 = utf16[++i];
            if (!is_little_endian) {
                w2 = (w2 >> 8) | (w2 << 8); // Swap bytes for big endian
            }

            if (w2 < 0xDC00 || w2 > 0xDFFF) {
                throw std::runtime_error("Invalid UTF-16 sequence");
            }

            uint32_t code_point = ((w1 - 0xD800) << 10) + (w2 - 0xDC00) + 0x10000;

            utf8.push_back(0xF0 | (code_point >> 18));
            utf8.push_back(0x80 | ((code_point >> 12) & 0x3F));
            utf8.push_back(0x80 | ((code_point >> 6) & 0x3F));
            utf8.push_back(0x80 | (code_point & 0x3F));
        } else if (w1 >= 0xDC00 && w1 <= 0xDFFF) {
            throw std::runtime_error("Invalid UTF-16 sequence");
        } else {
            if (w1 < 0x80) {
                utf8.push_back(static_cast<char>(w1));
            } else if (w1 < 0x800) {
                utf8.push_back(0xC0 | (w1 >> 6));
                utf8.push_back(0x80 | (w1 & 0x3F));
            } else {
                utf8.push_back(0xE0 | (w1 >> 12));
                utf8.push_back(0x80 | ((w1 >> 6) & 0x3F));
                utf8.push_back(0x80 | (w1 & 0x3F));
            }
        }
    }

    return utf8;
}

// Convert a single UTF-16 code unit to UTF-8 bytes
// This function assumes valid UTF-16 input.
inline void utf16_to_utf8(uint16_t utf16, char *&utf8) {
    if (utf16 < 0x80) {
        *utf8++ = static_cast<char>(utf16);
    } else if (utf16 < 0x800) {
        *utf8++ = static_cast<char>((utf16 >> 6) | 0xC0);
        *utf8++ = static_cast<char>((utf16 & 0x3F) | 0x80);
    } else {
        *utf8++ = static_cast<char>((utf16 >> 12) | 0xE0);
        *utf8++ = static_cast<char>(((utf16 >> 6) & 0x3F) | 0x80);
        *utf8++ = static_cast<char>((utf16 & 0x3F) | 0x80);
    }
}

// Swap bytes to convert from big endian to little endian
inline uint16_t swap_bytes(uint16_t value) {
    return (value >> 8) | (value << 8);
}

std::string utf16_to_utf8_avx2(const std::u16string &utf16, bool is_little_endian) {
    std::string utf8;
    utf8.reserve(utf16.size() * 3); // Reserve enough space to avoid frequent reallocations

    const __m256i one = _mm256_set1_epi16(1);
    const __m256i limit1 = _mm256_set1_epi16(0x80);
    const __m256i limit2 = _mm256_set1_epi16(0x800);

    char buffer[48]; // Buffer to hold temporary UTF-8 bytes
    char *output = buffer;

    size_t i = 0;
    size_t n = utf16.size();

    if (is_little_endian) {
        while (i + 16 <= n) {
            __m256i in = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(utf16.data() + i));
            __m256i mask1 = _mm256_cmpgt_epi16(in, limit1);
            __m256i mask2 = _mm256_cmpgt_epi16(in, limit2);

            if (_mm256_testz_si256(mask1, mask1)) {
                for (int j = 0; j < 16; ++j) {
                    *output++ = static_cast<char>(utf16[i + j]);
                }
            } else if (_mm256_testz_si256(mask2, mask2)) {
                for (int j = 0; j < 16; ++j) {
                    utf16_to_utf8(utf16[i + j], output);
                }
            } else {
                for (int j = 0; j < 16; ++j) {
                    utf16_to_utf8(utf16[i + j], output);
                }
            }

            utf8.append(buffer, output - buffer);
            output = buffer; // Reset output buffer pointer
            i += 16;
        }
    } else {
        while (i + 16 <= n) {
            __m256i in = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(utf16.data() + i));
            in = _mm256_or_si256(_mm256_srli_epi16(in, 8), _mm256_slli_epi16(in, 8)); // Swap bytes for big endian

            __m256i mask1 = _mm256_cmpgt_epi16(in, limit1);
            __m256i mask2 = _mm256_cmpgt_epi16(in, limit2);

            if (_mm256_testz_si256(mask1, mask1)) {
                for (int j = 0; j < 16; ++j) {
                    *output++ = static_cast<char>(swap_bytes(utf16[i + j]));
                }
            } else if (_mm256_testz_si256(mask2, mask2)) {
                for (int j = 0; j < 16; ++j) {
                    utf16_to_utf8(swap_bytes(utf16[i + j]), output);
                }
            } else {
                for (int j = 0; j < 16; ++j) {
                    utf16_to_utf8(swap_bytes(utf16[i + j]), output);
                }
            }

            utf8.append(buffer, output - buffer);
            output = buffer; // Reset output buffer pointer
            i += 16;
        }
    }

    while (i < n) {
        uint16_t code_unit = utf16[i++];
        if (!is_little_endian) {
            code_unit = swap_bytes(code_unit);
        }
        utf16_to_utf8(code_unit, output);
    }
    utf8.append(buffer, output - buffer);

    return utf8;
}

// Generate random UTF-16 string ensuring valid surrogate pairs
std::u16string generate_random_utf16_string(size_t length) {
    std::u16string str;
    std::mt19937 generator(std::random_device{}());
    std::uniform_int_distribution<uint32_t> distribution(0, 0x10FFFF);

    while (str.size() < length) {
        uint32_t code_point = distribution(generator);

        if (code_point <= 0xD7FF || (code_point >= 0xE000 && code_point <= 0xFFFF)) {
            str.push_back(static_cast<char16_t>(code_point));
        } else if (code_point >= 0x10000 && code_point <= 0x10FFFF) {
            code_point -= 0x10000;
            str.push_back(static_cast<char16_t>((code_point >> 10) + 0xD800));
            str.push_back(static_cast<char16_t>((code_point & 0x3FF) + 0xDC00));
        }
    }

    return str;
}

int main() {
    const size_t num_tests = 1000;
    const size_t string_length = 1000;

    // Generate random UTF-16 strings
    std::vector<std::u16string> test_strings;
    for (size_t i = 0; i < num_tests; ++i) {
        test_strings.push_back(generate_random_utf16_string(string_length));
    }

    // Test standard library conversion
    try {
        auto start = std::chrono::high_resolution_clock::now();
        for (const auto &str : test_strings) {
            std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> convert;
            std::string utf8 = convert.to_bytes(str);
        }
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> elapsed = end - start;
        std::cout << "Standard library conversion took: " << elapsed.count() << " seconds" << std::endl;
    } catch (const std::exception &e) {
        std::cerr << "Caught exception: " << e.what() << std::endl;
    }
    int i=1;
    // Test conversion without using AVX2
    try {
        auto start = std::chrono::high_resolution_clock::now();
        for (const auto &str : test_strings) {
            std::string utf8 = utf16_to_utf8(str, true);
        }
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> elapsed = end - start;
        std::cout << "Conversion without AVX2 took: " << elapsed.count() << " seconds" << std::endl;
    } catch (const std::exception &e) {
        std::cerr << "Caught exception: " << e.what() << std::endl;
    }

    // Test conversion with AVX2
    try {
        auto start = std::chrono::high_resolution_clock::now();
        for (const auto &str : test_strings) {
            std::string utf8 = utf16_to_utf8_avx2(str, true);
        }
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> elapsed = end - start;
        std::cout << "Conversion with AVX2 took: " << elapsed.count() << " seconds" << std::endl;
    } catch (const std::exception &e) {
        std::cerr << "Caught exception: " << e.what() << std::endl;
    }

    return 0;
}
