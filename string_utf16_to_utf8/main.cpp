#include <iostream>
#include <vector>
#include <string>
#include <random>
#include <chrono>
#include <stdexcept>
#include <algorithm>
#include <immintrin.h>
#include <locale>

// 将UTF-16编码的字符串转换为UTF-8编码的字符串，手动实现
std::string utf16_to_utf8(const std::u16string &utf16) {
    std::string utf8;
    size_t i = 0;

    while (i < utf16.size()) {
        uint16_t w1 = utf16[i++];

        if (w1 >= 0xD800 && w1 <= 0xDBFF) {
            if (i >= utf16.size()) {
                throw std::runtime_error("无效的UTF-16序列");
            }

            uint16_t w2 = utf16[i++];

            if (w2 < 0xDC00 || w2 > 0xDFFF) {
                throw std::runtime_error("无效的UTF-16序列");
            }

            uint32_t code_point = ((w1 - 0xD800) << 10) + (w2 - 0xDC00) + 0x10000;

            utf8.push_back(0xF0 | (code_point >> 18));
            utf8.push_back(0x80 | ((code_point >> 12) & 0x3F));
            utf8.push_back(0x80 | ((code_point >> 6) & 0x3F));
            utf8.push_back(0x80 | (code_point & 0x3F));
        } else if (w1 >= 0xDC00 && w1 <= 0xDFFF) {
            throw std::runtime_error("无效的UTF-16序列");
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

// 使用AVX2加速UTF-16到UTF-8的转换
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
                throw std::runtime_error("无效的UTF-16序列");
            }

            uint16_t w2 = utf16[++i];

            if (w2 < 0xDC00 || w2 > 0xDFFF) {
                throw std::runtime_error("无效的UTF-16序列");
            }

            uint32_t code_point = ((w1 - 0xD800) << 10) + (w2 - 0xDC00) + 0x10000;

            utf8.push_back(0xF0 | (code_point >> 18));
            utf8.push_back(0x80 | ((code_point >> 12) & 0x3F));
            utf8.push_back(0x80 | ((code_point >> 6) & 0x3F));
            utf8.push_back(0x80 | (code_point & 0x3F));
        } else if (w1 >= 0xDC00 && w1 <= 0xDFFF) {
            throw std::runtime_error("无效的UTF-16序列");
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

// 生成随机UTF-16字符串，确保有效的代理对
std::u16string generate_random_utf16_string(size_t length) {
    std::u16string str;
    std::mt19937 generator(std::random_device{}());
    std::uniform_int_distribution<uint32_t> distribution(0, 0x10FFFF);

    for (size_t i = 0; i < length; ++i) {
        uint32_t code_point = distribution(generator);

        if (code_point <= 0xFFFF) {
            str.push_back(static_cast<char16_t>(code_point));
        } else if (code_point <= 0x10FFFF) {
            code_point -= 0x10000;
            str.push_back(static_cast<char16_t>((code_point >> 10) + 0xD800));
            str.push_back(static_cast<char16_t>((code_point & 0x3FF) + 0xDC00));
            ++i; // 每个代理对算作一个字符，因此 i 需要递增
        } else {
            throw std::runtime_error("无效的UTF-32码点");
        }
    }

    return str;
}

int main() {
    const size_t num_tests = 1000;
    const size_t string_length = 1000;

    // 生成随机UTF-16字符串
    std::vector<std::u16string> test_strings;
    for (size_t i = 0; i < num_tests; ++i) {
        test_strings.push_back(generate_random_utf16_string(string_length));
    }

    // 测试标准库函数
    auto start = std::chrono::high_resolution_clock::now();
    for (const auto &str : test_strings) {
        try {
            std::string utf8 = utf16_to_utf8(str);
        } catch (const std::exception& e) {
            std::cerr << "Caught exception: " << e.what() << std::endl;
        }
    }
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = end - start;
    std::cout << "不使用AVX2的转换用时: " << elapsed.count() << " 秒" << std::endl;

    // 测试使用AVX2的转换
    start = std::chrono::high_resolution_clock::now();
    for (const auto &str : test_strings) {
        try {
            std::string utf8 = utf16_to_utf8_avx2(str);
        } catch (const std::exception& e) {
            std::cerr << "Caught exception: " << e.what() << std::endl;
        }
    }
    end = std::chrono::high_resolution_clock::now();
    elapsed = end - start;
    std::cout << "使用AVX2的转换用时: " << elapsed.count() << " 秒" << std::endl;

    return 0;
}
