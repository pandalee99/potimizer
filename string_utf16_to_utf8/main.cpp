#include <iostream>
#include <string>
#include <cstring>
#include <cassert>
#include <chrono>
#include <locale>
#include <codecvt>
#include <random>

// 自定义 UTF-16 到 UTF-8 转换函数
std::string utf16_to_utf8(const std::u16string& utf16_str) {
    std::string utf8_str;
    for (size_t i = 0; i < utf16_str.size(); ++i) {
        char16_t ch = utf16_str[i];
        if (ch < 0x80) {
            utf8_str.push_back(static_cast<char>(ch));
        } else if (ch < 0x800) {
            utf8_str.push_back(static_cast<char>(0xC0 | (ch >> 6)));
            utf8_str.push_back(static_cast<char>(0x80 | (ch & 0x3F)));
        } else {
            utf8_str.push_back(static_cast<char>(0xE0 | (ch >> 12)));
            utf8_str.push_back(static_cast<char>(0x80 | ((ch >> 6) & 0x3F)));
            utf8_str.push_back(static_cast<char>(0x80 | (ch & 0x3F)));
        }
    }
    return utf8_str;
}

// 随机生成 UTF-16 字符串
std::u16string generate_random_utf16_string(size_t length) {
    std::u16string random_str;
    std::mt19937 rng(std::random_device{}());
    std::uniform_int_distribution<uint16_t> dist(0x4E00, 0x9FFF); // 常用汉字范围

    for (size_t i = 0; i < length; ++i) {
        random_str.push_back(static_cast<char16_t>(dist(rng)));
    }

    return random_str;
}

int main() {
    // 测试字符串
    const char16_t test_string[] = u"Hello, 你好！";
    const char* expected_utf8_output = "Hello, 你好！";
    std::u16string utf16_str = test_string;

    // 自定义转换
    auto start = std::chrono::high_resolution_clock::now();
    std::string utf8_output = utf16_to_utf8(utf16_str);
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed_custom = end - start;

    // 标准库转换
    start = std::chrono::high_resolution_clock::now();
    std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> convert;
    std::string utf8_output_lib = convert.to_bytes(utf16_str);
    end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed_lib = end - start;

    // 打印输出
    std::cout << "utf8_output: " << utf8_output << std::endl;
    std::cout << "expected_utf8_output: " << expected_utf8_output << std::endl;
    std::cout << "utf8_output_lib: " << utf8_output_lib << std::endl;

    // 断言测试
    assert(strcmp(utf8_output.c_str(), expected_utf8_output) == 0);
    assert(strcmp(utf8_output_lib.c_str(), expected_utf8_output) == 0);

    // 打印性能对比
    std::cout << "Custom conversion time: " << elapsed_custom.count() << " seconds" << std::endl;
    std::cout << "Library conversion time: " << elapsed_lib.count() << " seconds" << std::endl;

    // 生成随机 UTF-16 字符串
    std::u16string random_utf16_str = generate_random_utf16_string(100); // 100 个字符

    // 转换并测试
    utf8_output_lib = convert.to_bytes(random_utf16_str);
    utf8_output = utf16_to_utf8(random_utf16_str);

    // 打印输出
    std::cout << "Random utf8_output: " << utf8_output << std::endl;
    std::cout << "Random utf8_output_lib: " << utf8_output_lib << std::endl;

    // 断言测试
    assert(utf8_output == utf8_output_lib);

    // 程序结束前暂停
    std::cout << "Press any key to exit...";
    std::cin.get();

    return 0;
}
