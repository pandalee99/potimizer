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
std::string utf16_to_utf8(const std::u16string &utf16) {
    std::string utf8;

    for (size_t i = 0; i < utf16.size(); ++i) {
        uint16_t w1 = utf16[i];

        if (w1 >= 0xD800 && w1 <= 0xDBFF) {
            if (i + 1 >= utf16.size()) {
                throw std::runtime_error("Invalid UTF-16 sequence");
            }

            uint16_t w2 = utf16[++i];

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

// Accelerate UTF-16 to UTF-8 conversion using AVX2
std::string utf16_to_utf8_avx2(const std::u16string &utf16) {
    std::string utf8;
    size_t i = 0;

    for (; i + 8 <= utf16.size(); i += 8) {
        __m128i chunk = _mm_loadu_si128(reinterpret_cast<const __m128i*>(&utf16[i]));

        for (int j = 0; j < 8; ++j) {
            uint16_t w1 = reinterpret_cast<uint16_t*>(&chunk)[j];

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

    for (; i < utf16.size(); ++i) {
        uint16_t w1 = utf16[i];

        if (w1 >= 0xD800 && w1 <= 0xDBFF) {
            if (i + 1 >= utf16.size()) {
                throw std::runtime_error("Invalid UTF-16 sequence");
            }

            uint16_t w2 = utf16[++i];

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

    // Test conversion without using AVX2
    try {
        auto start = std::chrono::high_resolution_clock::now();
        for (const auto &str : test_strings) {
            std::string utf8 = utf16_to_utf8(str);
        }
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> elapsed = end - start;
        std::cout << "Conversion without AVX2 took: " << elapsed.count() << " seconds" << std::endl;
    } catch (const std::exception &e) {
        std::cerr << "Caught exception: " << e.what() << std::endl;
    }

    // Test conversion using AVX2
    try {
        auto start = std::chrono::high_resolution_clock::now();
        for (const auto &str : test_strings) {
            std::string utf8 = utf16_to_utf8_avx2(str);
        }
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> elapsed = end - start;
        std::cout << "Conversion using AVX2 took: " << elapsed.count() << " seconds" << std::endl;
    } catch (const std::exception &e) {
        std::cerr << "Caught exception: " << e.what() << std::endl;
    }

    return 0;
}
