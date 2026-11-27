#pragma once

#include <array>
#include <string>
#include <vector>

namespace chessforge {

// 5x7 bitmap font. Each row uses the lowest five bits (bit 4 = leftmost pixel).
struct Glyph {
    std::array<unsigned char, 7> rows{};
    // Extra marks drawn above the letter: 0 none, 1 breve (Й), 2 diaeresis (Ё Ї), 3 tick (Ґ).
    unsigned char mark = 0;
};

// Splits UTF-8 text into code points (invalid bytes are skipped) and upper-cases
// Latin and Cyrillic letters.
[[nodiscard]] std::vector<char32_t> decodeUtf8(const std::string& text);

// Returns false if the font has no glyph for the code point.
[[nodiscard]] bool glyphFor(char32_t codePoint, Glyph& out);

}  // namespace chessforge
