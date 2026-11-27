#include "font.hpp"

namespace chessforge {
namespace {

char32_t upper(char32_t c) {
    if (c >= U'a' && c <= U'z') return c - 32;
    if (c >= 0x0430 && c <= 0x044F) return c - 0x20;  // а-я
    switch (c) {
        case 0x0451: return 0x0401;  // ё
        case 0x0454: return 0x0404;  // є
        case 0x0456: return 0x0406;  // і
        case 0x0457: return 0x0407;  // ї
        case 0x0491: return 0x0490;  // ґ
        default: return c;
    }
}

}  // namespace

std::vector<char32_t> decodeUtf8(const std::string& text) {
    std::vector<char32_t> result;
    result.reserve(text.size());
    std::size_t i = 0;
    while (i < text.size()) {
        const unsigned char lead = static_cast<unsigned char>(text[i]);
        char32_t code = 0;
        std::size_t length = 1;
        if (lead < 0x80) {
            code = lead;
        } else if ((lead & 0xE0U) == 0xC0U) {
            code = lead & 0x1FU;
            length = 2;
        } else if ((lead & 0xF0U) == 0xE0U) {
            code = lead & 0x0FU;
            length = 3;
        } else if ((lead & 0xF8U) == 0xF0U) {
            code = lead & 0x07U;
            length = 4;
        } else {
            ++i;
            continue;
        }
        if (i + length > text.size()) break;
        bool valid = true;
        for (std::size_t k = 1; k < length; ++k) {
            const unsigned char follow = static_cast<unsigned char>(text[i + k]);
            if ((follow & 0xC0U) != 0x80U) { valid = false; break; }
            code = (code << 6) | (follow & 0x3FU);
        }
        i += valid ? length : 1;
        if (valid) result.push_back(upper(code));
    }
    return result;
}

bool glyphFor(char32_t codePoint, Glyph& out) {
    out = Glyph{};
    const auto set = [&](unsigned char a, unsigned char b, unsigned char c, unsigned char d, unsigned char e, unsigned char f, unsigned char g,
                         unsigned char mark = 0) {
        out.rows = {a, b, c, d, e, f, g};
        out.mark = mark;
        return true;
    };
    switch (codePoint) {
        // Latin
        case U'A': case 0x0410: return set(14, 17, 17, 31, 17, 17, 17);
        case U'B': case 0x0412: return set(30, 17, 17, 30, 17, 17, 30);
        case U'C': case 0x0421: return set(15, 16, 16, 16, 16, 16, 15);
        case U'D': return set(30, 17, 17, 17, 17, 17, 30);
        case U'E': case 0x0415: return set(31, 16, 16, 30, 16, 16, 31);
        case U'F': return set(31, 16, 16, 30, 16, 16, 16);
        case U'G': return set(15, 16, 16, 23, 17, 17, 14);
        case U'H': case 0x041D: return set(17, 17, 17, 31, 17, 17, 17);
        case U'I': case 0x0406: return set(31, 4, 4, 4, 4, 4, 31);
        case U'J': return set(7, 2, 2, 2, 18, 18, 12);
        case U'K': case 0x041A: return set(17, 18, 20, 24, 20, 18, 17);
        case U'L': return set(16, 16, 16, 16, 16, 16, 31);
        case U'M': case 0x041C: return set(17, 27, 21, 17, 17, 17, 17);
        case U'N': return set(17, 25, 21, 19, 17, 17, 17);
        case U'O': case 0x041E: return set(14, 17, 17, 17, 17, 17, 14);
        case U'P': case 0x0420: return set(30, 17, 17, 30, 16, 16, 16);
        case U'Q': return set(14, 17, 17, 17, 21, 18, 13);
        case U'R': return set(30, 17, 17, 30, 20, 18, 17);
        case U'S': return set(15, 16, 16, 14, 1, 1, 30);
        case U'T': case 0x0422: return set(31, 4, 4, 4, 4, 4, 4);
        case U'U': return set(17, 17, 17, 17, 17, 17, 14);
        case U'V': return set(17, 17, 17, 17, 17, 10, 4);
        case U'W': return set(17, 17, 17, 17, 21, 27, 17);
        case U'X': case 0x0425: return set(17, 17, 10, 4, 10, 17, 17);
        case U'Y': case 0x0423: return set(17, 17, 10, 4, 4, 4, 4);
        case U'Z': return set(31, 1, 2, 4, 8, 16, 31);
        // Cyrillic only
        case 0x0401: return set(10, 31, 16, 30, 16, 16, 31);                // Ё
        case 0x0404: return set(14, 17, 16, 28, 16, 17, 14);                // Є
        case 0x0407: return set(31, 4, 4, 4, 4, 4, 31, 2);                  // Ї
        case 0x0490: return set(31, 16, 16, 16, 16, 16, 16, 3);             // Ґ
        case 0x0411: return set(31, 16, 16, 30, 17, 17, 30);                // Б
        case 0x0413: return set(31, 16, 16, 16, 16, 16, 16);                // Г
        case 0x0414: return set(15, 9, 9, 9, 9, 31, 17);                    // Д
        case 0x0416: return set(21, 21, 21, 14, 21, 21, 21);                // Ж
        case 0x0417: return set(14, 17, 1, 6, 1, 17, 14);                   // З
        case 0x0418: return set(17, 17, 19, 21, 25, 17, 17);                // И
        case 0x0419: return set(17, 17, 19, 21, 25, 17, 17, 1);             // Й
        case 0x041B: return set(7, 9, 9, 9, 9, 17, 17);                     // Л
        case 0x041F: return set(31, 17, 17, 17, 17, 17, 17);                // П
        case 0x0424: return set(4, 14, 21, 21, 21, 14, 4);                  // Ф
        case 0x0426: return set(17, 17, 17, 17, 17, 31, 1);                 // Ц
        case 0x0427: return set(17, 17, 17, 15, 1, 1, 1);                   // Ч
        case 0x0428: return set(21, 21, 21, 21, 21, 21, 31);                // Ш
        case 0x0429: return set(21, 21, 21, 21, 21, 31, 1);                 // Щ
        case 0x042A: return set(24, 8, 8, 14, 9, 9, 14);                    // Ъ
        case 0x042B: return set(17, 17, 17, 25, 21, 21, 25);                // Ы
        case 0x042C: return set(16, 16, 16, 30, 17, 17, 30);                // Ь
        case 0x042D: return set(14, 17, 1, 7, 1, 17, 14);                   // Э
        case 0x042E: return set(18, 21, 21, 29, 21, 21, 18);                // Ю
        case 0x042F: return set(15, 17, 17, 15, 5, 9, 17);                  // Я
        // Digits and punctuation
        case U'0': return set(14, 17, 19, 21, 25, 17, 14);
        case U'1': return set(4, 12, 4, 4, 4, 4, 14);
        case U'2': return set(14, 17, 1, 2, 4, 8, 31);
        case U'3': return set(30, 1, 1, 14, 1, 1, 30);
        case U'4': return set(2, 6, 10, 18, 31, 2, 2);
        case U'5': return set(31, 16, 16, 30, 1, 1, 30);
        case U'6': return set(14, 16, 16, 30, 17, 17, 14);
        case U'7': return set(31, 1, 2, 4, 8, 8, 8);
        case U'8': return set(14, 17, 17, 14, 17, 17, 14);
        case U'9': return set(14, 17, 17, 15, 1, 1, 14);
        case U':': return set(0, 4, 4, 0, 4, 4, 0);
        case U'-': return set(0, 0, 0, 31, 0, 0, 0);
        case U'/': return set(1, 2, 2, 4, 8, 8, 16);
        case U'.': return set(0, 0, 0, 0, 0, 6, 6);
        case U',': return set(0, 0, 0, 0, 6, 4, 8);
        case U'+': return set(0, 4, 4, 31, 4, 4, 0);
        case U' ': return set(0, 0, 0, 0, 0, 0, 0);
        case U'!': return set(4, 4, 4, 4, 4, 0, 4);
        case U'?': return set(14, 17, 1, 2, 4, 0, 4);
        case U'(': return set(2, 4, 8, 8, 8, 4, 2);
        case U')': return set(8, 4, 2, 2, 2, 4, 8);
        case U'=': return set(0, 0, 31, 0, 31, 0, 0);
        case U'<': return set(2, 4, 8, 16, 8, 4, 2);
        case U'>': return set(8, 4, 2, 1, 2, 4, 8);
        case U'%': return set(25, 25, 2, 4, 8, 19, 19);
        case U'*': return set(0, 21, 14, 31, 14, 21, 0);
        case U'_': return set(0, 0, 0, 0, 0, 0, 31);
        case U'\'': return set(4, 4, 8, 0, 0, 0, 0);
        default: return false;
    }
}

}  // namespace chessforge
