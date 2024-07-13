#include <iostream>

#include <vector>
#include <stdexcept>
#include <cstdint>

std::vector<uint8_t> utf16_to_utf8(const std::u16string &utf16) {
    std::vector<uint8_t> utf8;

    for (size_t i = 0; i < utf16.size(); ++i) {
        uint16_t w1 = utf16[i];

        // If the character is in the range of U+D800 to U+DBFF, it is a high surrogate
        if (w1 >= 0xD800 && w1 <= 0xDBFF) {
            // Ensure there is another character
            if (i + 1 >= utf16.size()) {
                throw std::runtime_error("Invalid UTF-16 sequence");
            }

            uint16_t w2 = utf16[++i];

            // Check if it is a low surrogate
            if (w2 < 0xDC00 || w2 > 0xDFFF) {
                throw std::runtime_error("Invalid UTF-16 sequence");
            }

            // Decode the surrogate pair
            uint32_t code_point = ((w1 - 0xD800) << 10) + (w2 - 0xDC00) + 0x10000;

            // Encode the code point in UTF-8
            utf8.push_back(0xF0 | (code_point >> 18));
            utf8.push_back(0x80 | ((code_point >> 12) & 0x3F));
            utf8.push_back(0x80 | ((code_point >> 6) & 0x3F));
            utf8.push_back(0x80 | (code_point & 0x3F));
        } else if (w1 >= 0xDC00 && w1 <= 0xDFFF) {
            // A low surrogate without a preceding high surrogate is invalid
            throw std::runtime_error("Invalid UTF-16 sequence");
        } else {
            // Encode a single UTF-16 code unit in UTF-8
            if (w1 < 0x80) {
                utf8.push_back(static_cast<uint8_t>(w1));
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

int main() {
    // Example UTF-16 string (with both BMP and supplementary characters)
    std::u16string utf16_string = u"Hello, \u4F60\u597D! \U0001F600";

    try {
        std::vector<uint8_t> utf8 = utf16_to_utf8(utf16_string);

        // Print the UTF-8 string as hex values
        for (uint8_t byte : utf8) {
            std::cout << std::hex << static_cast<int>(byte) << " ";
        }
        std::cout << std::endl;
    } catch (const std::runtime_error &e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }

    return 0;
}
