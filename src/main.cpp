#include "chess.hpp"
#include "engine.hpp"
#include "font.hpp"
#include "i18n.hpp"
#include "input_map.hpp"
#include "pieces3d.hpp"

#include <SDL2/SDL.h>

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <filesystem>
#include <fstream>
#include <functional>
#include <initializer_list>
#include <optional>
#include <sstream>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

namespace {

constexpr int ScreenWidth = 640;
constexpr int ScreenHeight = 480;
constexpr int BoardLeft = 16;
constexpr int BoardTop = 36;
constexpr int SquareSize = 52;
constexpr int BoardPixels = SquareSize * 8;
constexpr float Pi = 3.14159265358979F;

using chessforge::Action;
using chessforge::Board;
using chessforge::Color;
using chessforge::ComputerPlayer;
using chessforge::GameState;
using chessforge::InputMap;
using chessforge::Move;
using chessforge::Piece;
using chessforge::PieceType;
using chessforge::Str;
using chessforge::tr;

struct Theme {
    SDL_Color background;
    SDL_Color panel;
    SDL_Color lightSquare;
    SDL_Color darkSquare;
    SDL_Color accent;
    SDL_Color highlight;
    SDL_Color whitePiece;
    SDL_Color blackPiece;
    SDL_Color text;
};

constexpr std::array<Theme, 4> Themes{{
    {{20, 28, 34, 255}, {29, 42, 51, 255}, {224, 202, 164, 255}, {126, 88, 61, 255}, {65, 185, 161, 255}, {245, 196, 75, 255}, {248, 242, 225, 255}, {44, 52, 60, 255}, {239, 244, 247, 255}},
    {{20, 17, 30, 255}, {39, 32, 57, 255}, {228, 217, 235, 255}, {116, 87, 139, 255}, {208, 88, 175, 255}, {85, 216, 197, 255}, {250, 245, 255, 255}, {35, 26, 48, 255}, {248, 243, 255, 255}},
    {{15, 33, 29, 255}, {20, 53, 44, 255}, {217, 228, 197, 255}, {77, 119, 91, 255}, {93, 201, 128, 255}, {241, 197, 89, 255}, {253, 251, 231, 255}, {22, 47, 39, 255}, {234, 250, 238, 255}},
    {{25, 25, 27, 255}, {43, 43, 47, 255}, {196, 196, 196, 255}, {75, 75, 80, 255}, {240, 164, 58, 255}, {98, 184, 244, 255}, {250, 250, 250, 255}, {36, 36, 39, 255}, {246, 246, 246, 255}},
}};

// Board colours of the Lichess "Brown", "Blue", "Green" and "Purple" boards inside Lichess' dark site colours.
// Theme numbers: 0-3 built-in, 4 = custom (editable), 5.. = Lichess boards.
constexpr int CustomTheme = 4;
constexpr std::array<Theme, 4> LichessThemes{{
    {{22, 21, 18, 255}, {38, 36, 33, 255}, {240, 217, 181, 255}, {181, 136, 99, 255}, {98, 153, 36, 255}, {155, 199, 0, 255}, {250, 250, 250, 255}, {46, 46, 50, 255}, {210, 210, 205, 255}},
    {{22, 21, 18, 255}, {38, 36, 33, 255}, {222, 227, 230, 255}, {140, 162, 173, 255}, {98, 153, 36, 255}, {155, 199, 0, 255}, {250, 250, 250, 255}, {46, 46, 50, 255}, {210, 210, 205, 255}},
    {{22, 21, 18, 255}, {38, 36, 33, 255}, {255, 255, 221, 255}, {134, 166, 102, 255}, {98, 153, 36, 255}, {155, 199, 0, 255}, {250, 250, 250, 255}, {46, 46, 50, 255}, {210, 210, 205, 255}},
    {{22, 21, 18, 255}, {38, 36, 33, 255}, {159, 144, 176, 255}, {125, 74, 141, 255}, {98, 153, 36, 255}, {155, 199, 0, 255}, {250, 250, 250, 255}, {46, 46, 50, 255}, {210, 210, 205, 255}},
}};
constexpr int ThemeCount = static_cast<int>(CustomTheme + 1 + LichessThemes.size());

// First launch: pick the interface language from the system locale (LANG=ru_RU... -> Russian).
int detectLanguage() {
    for (const char* name : {"LC_ALL", "LC_MESSAGES", "LANG", "LANGUAGE"}) {
        const char* value = std::getenv(name);
        if (value == nullptr || std::strlen(value) < 2) continue;
        const int index = chessforge::languageFromCode(std::string(value, 2));
        if (index >= 0) return index;
    }
    return 0;
}

std::string trimmed(std::string value) {
    while (!value.empty() && (value.back() == '\r' || value.back() == ' ' || value.back() == '\n')) value.pop_back();
    return value;
}

struct Settings {
    int theme = 0;
    int texture = 1;
    bool threeDimensional = true;
    int elo = 1200;
    bool playerWhite = true;
    float yaw = 0.0F;
    float pitch = 0.85F;
    int language = detectLanguage();
    bool twoPlayer = false;   // two humans share the device
    bool autoFlip = true;     // two-player: turn the board towards the side to move
    std::string pieceSet;     // empty = built-in vector pieces, otherwise folder name inside pieces/
    SDL_Color customLight{224, 202, 164, 255};
    SDL_Color customDark{126, 88, 61, 255};
    SDL_Color customAccent{65, 185, 161, 255};
    SDL_Color customBackground{20, 28, 34, 255};

    void load() {
        std::ifstream input("saves/settings.cfg");
        std::string key;
        std::string value;
        while (std::getline(input, key, '=') && std::getline(input, value)) {
            try {
                if (key == "piece_set") pieceSet = trimmed(value);
                if (key == "language") {
                    const int index = chessforge::languageFromCode(trimmed(value));
                    if (index >= 0) language = index;
                }
                if (key == "two_player") twoPlayer = trimmed(value) == "1";
                if (key == "auto_flip") autoFlip = trimmed(value) == "1";
                if (key == "theme") theme = std::clamp(std::stoi(value), 0, ThemeCount - 1);
                if (key == "texture") texture = std::clamp(std::stoi(value), 0, 4);
                if (key == "three_dimensional") threeDimensional = trimmed(value) == "1";
                if (key == "elo") elo = std::clamp(std::stoi(value), 400, 3200);
                if (key == "player_white") playerWhite = trimmed(value) == "1";
                if (key == "yaw") yaw = std::clamp(std::stof(value), -3.14F, 3.14F);
                if (key == "pitch") pitch = std::clamp(std::stof(value), 0.30F, 1.10F);
                if (key == "custom_light_r") customLight.r = static_cast<Uint8>(std::clamp(std::stoi(value), 0, 255));
                if (key == "custom_light_g") customLight.g = static_cast<Uint8>(std::clamp(std::stoi(value), 0, 255));
                if (key == "custom_light_b") customLight.b = static_cast<Uint8>(std::clamp(std::stoi(value), 0, 255));
                if (key == "custom_dark_r") customDark.r = static_cast<Uint8>(std::clamp(std::stoi(value), 0, 255));
                if (key == "custom_dark_g") customDark.g = static_cast<Uint8>(std::clamp(std::stoi(value), 0, 255));
                if (key == "custom_dark_b") customDark.b = static_cast<Uint8>(std::clamp(std::stoi(value), 0, 255));
                if (key == "custom_accent_r") customAccent.r = static_cast<Uint8>(std::clamp(std::stoi(value), 0, 255));
                if (key == "custom_accent_g") customAccent.g = static_cast<Uint8>(std::clamp(std::stoi(value), 0, 255));
                if (key == "custom_accent_b") customAccent.b = static_cast<Uint8>(std::clamp(std::stoi(value), 0, 255));
                if (key == "custom_background_r") customBackground.r = static_cast<Uint8>(std::clamp(std::stoi(value), 0, 255));
                if (key == "custom_background_g") customBackground.g = static_cast<Uint8>(std::clamp(std::stoi(value), 0, 255));
                if (key == "custom_background_b") customBackground.b = static_cast<Uint8>(std::clamp(std::stoi(value), 0, 255));
            } catch (const std::exception&) {
            }
        }
    }

    void save() const {
        std::filesystem::create_directories("saves");
        std::ofstream output("saves/settings.cfg", std::ios::trunc);
        output << "language=" << chessforge::languageCode(language) << '\n';
        output << "two_player=" << (twoPlayer ? 1 : 0) << '\n';
        output << "auto_flip=" << (autoFlip ? 1 : 0) << '\n';
        output << "theme=" << theme << '\n';
        output << "piece_set=" << pieceSet << '\n';
        output << "texture=" << texture << '\n';
        output << "three_dimensional=" << (threeDimensional ? 1 : 0) << '\n';
        output << "elo=" << elo << '\n';
        output << "player_white=" << (playerWhite ? 1 : 0) << '\n';
        output << "yaw=" << yaw << '\n';
        output << "pitch=" << pitch << '\n';
        output << "custom_light_r=" << static_cast<int>(customLight.r) << '\n';
        output << "custom_light_g=" << static_cast<int>(customLight.g) << '\n';
        output << "custom_light_b=" << static_cast<int>(customLight.b) << '\n';
        output << "custom_dark_r=" << static_cast<int>(customDark.r) << '\n';
        output << "custom_dark_g=" << static_cast<int>(customDark.g) << '\n';
        output << "custom_dark_b=" << static_cast<int>(customDark.b) << '\n';
        output << "custom_accent_r=" << static_cast<int>(customAccent.r) << '\n';
        output << "custom_accent_g=" << static_cast<int>(customAccent.g) << '\n';
        output << "custom_accent_b=" << static_cast<int>(customAccent.b) << '\n';
        output << "custom_background_r=" << static_cast<int>(customBackground.r) << '\n';
        output << "custom_background_g=" << static_cast<int>(customBackground.g) << '\n';
        output << "custom_background_b=" << static_cast<int>(customBackground.b) << '\n';
    }
};

void setColor(SDL_Renderer* renderer, SDL_Color color) {
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
}

void fill(SDL_Renderer* renderer, SDL_Rect rect, SDL_Color color) {
    setColor(renderer, color);
    SDL_RenderFillRect(renderer, &rect);
}

void outline(SDL_Renderer* renderer, SDL_Rect rect, SDL_Color color, int width = 1) {
    setColor(renderer, color);
    for (int step = 0; step < width; ++step) {
        SDL_Rect inner{rect.x + step, rect.y + step, rect.w - 2 * step, rect.h - 2 * step};
        SDL_RenderDrawRect(renderer, &inner);
    }
}

// Text is UTF-8; letters of the built-in 5x7 font cover Latin and Cyrillic.
int textWidth(const std::string& text, int scale) {
    return static_cast<int>(chessforge::decodeUtf8(text).size()) * 6 * scale;
}

void drawText(SDL_Renderer* renderer, int x, int y, const std::string& text, int scale, SDL_Color color) {
    setColor(renderer, color);
    int pen = x;
    const auto dot = [&](int column, int row) {
        SDL_Rect pixel{pen + column * scale, y + row * scale, scale, scale};
        SDL_RenderFillRect(renderer, &pixel);
    };
    for (const char32_t character : chessforge::decodeUtf8(text)) {
        chessforge::Glyph glyph;
        if (!chessforge::glyphFor(character, glyph)) glyph.rows = {31, 17, 2, 4, 8, 0, 8};
        for (int row = 0; row < 7; ++row) {
            for (int column = 0; column < 5; ++column) {
                if ((glyph.rows[static_cast<std::size_t>(row)] & (1 << (4 - column))) != 0) dot(column, row);
            }
        }
        if (glyph.mark == 1) { dot(1, -2); dot(3, -2); dot(2, -1); }
        if (glyph.mark == 2) { dot(1, -1); dot(3, -1); }
        if (glyph.mark == 3) { dot(4, -1); dot(3, -1); }
        pen += 6 * scale;
    }
}

// Largest scale (<= requested, >= 1) at which the text still fits into maxWidth pixels.
int fitScale(const std::string& text, int scale, int maxWidth) {
    while (scale > 1 && textWidth(text, scale) > maxWidth) --scale;
    return scale;
}

void drawTextFit(SDL_Renderer* renderer, int x, int y, const std::string& text, int scale, int maxWidth, SDL_Color color) {
    const int used = fitScale(text, scale, maxWidth);
    // Keep small text vertically centred in the line reserved for the requested scale.
    drawText(renderer, x, y + (scale - used) * 7 / 2, text, used, color);
}

void drawTextRight(SDL_Renderer* renderer, int right, int y, const std::string& text, int scale, int maxWidth, SDL_Color color) {
    const int used = fitScale(text, scale, maxWidth);
    drawText(renderer, right - textWidth(text, used), y + (scale - used) * 7 / 2, text, used, color);
}

SDL_Color darken(SDL_Color value, int amount) {
    return {static_cast<Uint8>(std::clamp(static_cast<int>(value.r) - amount, 0, 255)), static_cast<Uint8>(std::clamp(static_cast<int>(value.g) - amount, 0, 255)), static_cast<Uint8>(std::clamp(static_cast<int>(value.b) - amount, 0, 255)), value.a};
}

SDL_Color brighten(SDL_Color value, int amount) {
    return {static_cast<Uint8>(std::clamp(static_cast<int>(value.r) + amount, 0, 255)), static_cast<Uint8>(std::clamp(static_cast<int>(value.g) + amount, 0, 255)), static_cast<Uint8>(std::clamp(static_cast<int>(value.b) + amount, 0, 255)), value.a};
}

// ---------------------------------------------------------------------------
// Colour contrast: the cursor must stay visible on every square colour of every theme.
// ---------------------------------------------------------------------------
double relativeLuminance(SDL_Color color) {
    const auto linear = [](int channel) {
        const double s = static_cast<double>(channel) / 255.0;
        return s <= 0.03928 ? s / 12.92 : std::pow((s + 0.055) / 1.055, 2.4);
    };
    return 0.2126 * linear(color.r) + 0.7152 * linear(color.g) + 0.0722 * linear(color.b);
}

// WCAG contrast ratio, 1 (identical) .. 21 (black on white).
double contrastRatio(SDL_Color a, SDL_Color b) {
    const double la = relativeLuminance(a);
    const double lb = relativeLuminance(b);
    return (std::max(la, lb) + 0.05) / (std::min(la, lb) + 0.05);
}

// "Redmean" colour distance, 0 .. ~765. Catches equal-brightness colours of different hue that the ratio accepts.
double colorDistance(SDL_Color a, SDL_Color b) {
    const double mean = (static_cast<double>(a.r) + static_cast<double>(b.r)) / 2.0;
    const double dr = static_cast<double>(a.r) - static_cast<double>(b.r);
    const double dg = static_cast<double>(a.g) - static_cast<double>(b.g);
    const double db = static_cast<double>(a.b) - static_cast<double>(b.b);
    return std::sqrt((2.0 + mean / 256.0) * dr * dr + 4.0 * dg * dg + (2.0 + (255.0 - mean) / 256.0) * db * db);
}

// Returns the first preferred colour that is clearly visible on `background` (and different from `avoid`);
// otherwise the best colour of a fixed palette of bright, saturated markers.
SDL_Color pickContrast(SDL_Color background, std::initializer_list<SDL_Color> preferred, std::optional<SDL_Color> avoid = std::nullopt) {
    const auto acceptable = [&](SDL_Color candidate) {
        if (avoid && colorDistance(candidate, *avoid) < 110.0) return false;
        return contrastRatio(candidate, background) >= 3.0 && colorDistance(candidate, background) >= 150.0;
    };
    for (const SDL_Color& candidate : preferred) {
        if (acceptable(candidate)) return candidate;
    }
    static const std::array<SDL_Color, 8> pool{{{255, 214, 10, 255}, {0, 229, 255, 255}, {255, 64, 190, 255}, {255, 138, 0, 255},
                                                {130, 255, 60, 255}, {255, 255, 255, 255}, {15, 15, 20, 255}, {255, 60, 60, 255}}};
    SDL_Color best = pool[0];
    double bestScore = -1.0;
    for (const SDL_Color& candidate : pool) {
        if (avoid && colorDistance(candidate, *avoid) < 110.0) continue;
        const double score = std::min(contrastRatio(candidate, background), 8.0) + colorDistance(candidate, background) / 200.0;
        if (score > bestScore) {
            bestScore = score;
            best = candidate;
        }
    }
    return best;
}

// Black or white, whichever separates better from `color` (used for the thin halo around cursor frames).
SDL_Color haloFor(SDL_Color color) {
    return relativeLuminance(color) > 0.45 ? SDL_Color{12, 12, 18, 255} : SDL_Color{250, 250, 252, 255};
}

// Tiny software canvas used to draw the built-in piece icons into an SDL surface.
class PieceCanvas {
public:
    explicit PieceCanvas(int size) : size_(size), surface_(SDL_CreateRGBSurfaceWithFormat(0, size, size, 32, SDL_PIXELFORMAT_RGBA32)) {
        if (surface_ != nullptr) SDL_FillRect(surface_, nullptr, 0);
    }
    ~PieceCanvas() { if (surface_ != nullptr) SDL_FreeSurface(surface_); }
    PieceCanvas(const PieceCanvas&) = delete;
    PieceCanvas& operator=(const PieceCanvas&) = delete;

    SDL_Surface* surface() const { return surface_; }
    SDL_Surface* release() { SDL_Surface* result = surface_; surface_ = nullptr; return result; }

    void color(SDL_Color value) { pixel_ = surface_ != nullptr ? SDL_MapRGBA(surface_->format, value.r, value.g, value.b, 255) : 0; }

    void span(int y, int x1, int x2) {
        if (surface_ == nullptr || y < 0 || y >= size_) return;
        x1 = std::max(x1, 0);
        x2 = std::min(x2, size_ - 1);
        auto* row = reinterpret_cast<Uint32*>(static_cast<Uint8*>(surface_->pixels) + y * surface_->pitch);
        for (int x = x1; x <= x2; ++x) row[x] = pixel_;
    }

    void rect(int x, int y, int w, int h) { for (int row = y; row < y + h; ++row) span(row, x, x + w - 1); }

    void circle(int cx, int cy, int r) {
        for (int dy = -r; dy <= r; ++dy) {
            const int dx = static_cast<int>(std::lround(std::sqrt(static_cast<float>(r * r - dy * dy))));
            span(cy + dy, cx - dx, cx + dx);
        }
    }

    void triangle(SDL_Point a, SDL_Point b, SDL_Point c) {
        const std::array<SDL_Point, 3> p{{a, b, c}};
        const int top = std::min({a.y, b.y, c.y});
        const int bottom = std::max({a.y, b.y, c.y});
        for (int y = top; y <= bottom; ++y) {
            float lo = 1e9F;
            float hi = -1e9F;
            for (std::size_t i = 0; i < 3; ++i) {
                const SDL_Point& from = p[i];
                const SDL_Point& to = p[(i + 1) % 3];
                if (from.y == to.y) {
                    if (from.y == y) { lo = std::min(lo, static_cast<float>(std::min(from.x, to.x))); hi = std::max(hi, static_cast<float>(std::max(from.x, to.x))); }
                    continue;
                }
                if (y < std::min(from.y, to.y) || y > std::max(from.y, to.y)) continue;
                const float x = static_cast<float>(from.x) + static_cast<float>(y - from.y) * static_cast<float>(to.x - from.x) / static_cast<float>(to.y - from.y);
                lo = std::min(lo, x);
                hi = std::max(hi, x);
            }
            if (hi >= lo) span(y, static_cast<int>(std::floor(lo)), static_cast<int>(std::ceil(hi)));
        }
    }

    void quad(SDL_Point a, SDL_Point b, SDL_Point c, SDL_Point d) { triangle(a, b, c); triangle(a, c, d); }

    void fan(const std::vector<SDL_Point>& points, SDL_Point center) {
        for (std::size_t i = 0; i < points.size(); ++i) triangle(center, points[i], points[(i + 1) % points.size()]);
    }

    void line(int x1, int y1, int x2, int y2, int radius) {
        const int steps = std::max(std::abs(x2 - x1), std::abs(y2 - y1));
        for (int step = 0; step <= steps; ++step) {
            const float t = steps == 0 ? 0.0F : static_cast<float>(step) / static_cast<float>(steps);
            circle(x1 + static_cast<int>(std::lround(t * static_cast<float>(x2 - x1))), y1 + static_cast<int>(std::lround(t * static_cast<float>(y2 - y1))), radius);
        }
    }

private:
    int size_;
    SDL_Surface* surface_;
    Uint32 pixel_ = 0;
};

// Draws the silhouette of one piece type (96x96 design grid), shifted by (ox, oy).
void drawPieceShape(PieceCanvas& c, PieceType type, int ox, int oy) {
    const auto P = [&](int x, int y) { return SDL_Point{x + ox, y + oy}; };
    const auto R = [&](int x, int y, int w, int h) { c.rect(x + ox, y + oy, w, h); };
    const auto C = [&](int x, int y, int r) { c.circle(x + ox, y + oy, r); };
    const auto Q = [&](int x1, int y1, int x2, int y2, int x3, int y3, int x4, int y4) { c.quad(P(x1, y1), P(x2, y2), P(x3, y3), P(x4, y4)); };
    const auto T = [&](int x1, int y1, int x2, int y2, int x3, int y3) { c.triangle(P(x1, y1), P(x2, y2), P(x3, y3)); };

    R(22, 78, 52, 9);
    R(28, 72, 40, 8);
    switch (type) {
        case PieceType::Pawn:
            C(48, 26, 12);
            Q(40, 34, 56, 34, 62, 50, 34, 50);
            Q(34, 48, 62, 48, 68, 74, 28, 74);
            break;
        case PieceType::Rook:
            R(26, 12, 12, 16); R(42, 12, 12, 16); R(58, 12, 12, 16);
            R(26, 24, 44, 8);
            Q(32, 30, 64, 30, 62, 66, 34, 66);
            Q(30, 64, 66, 64, 70, 74, 26, 74);
            break;
        case PieceType::Bishop:
            C(48, 12, 5);
            C(48, 30, 14);
            Q(38, 38, 58, 38, 66, 74, 30, 74);
            R(32, 52, 32, 7);
            break;
        case PieceType::Knight: {
            const std::vector<SDL_Point> shape{P(28, 76), P(36, 54), P(24, 46), P(22, 40), P(34, 22), P(38, 12), P(46, 18), P(60, 24), P(70, 46), P(68, 76)};
            c.fan(shape, P(50, 52));
            break;
        }
        case PieceType::Queen:
            Q(38, 38, 58, 38, 66, 74, 30, 74);
            T(24, 14, 28, 36, 40, 36); T(48, 10, 36, 36, 60, 36); T(72, 14, 56, 36, 68, 36);
            R(28, 34, 40, 8);
            C(24, 12, 4); C(48, 8, 4); C(72, 12, 4);
            break;
        case PieceType::King:
            Q(36, 34, 60, 34, 66, 74, 30, 74);
            C(48, 38, 14);
            R(44, 4, 8, 28);
            R(35, 12, 26, 8);
            break;
        case PieceType::None: break;
    }
}

void drawPieceDetails(PieceCanvas& c, PieceType type) {
    switch (type) {
        case PieceType::Bishop: c.line(47, 22, 55, 34, 1); break;
        case PieceType::Knight: c.circle(38, 28, 2); break;
        case PieceType::Queen: c.rect(34, 58, 28, 2); break;
        case PieceType::King: c.rect(36, 58, 24, 2); break;
        case PieceType::Rook: c.rect(34, 40, 28, 2); break;
        default: break;
    }
}

SDL_Surface* renderBuiltinPiece(PieceType type, SDL_Color fill, SDL_Color edge) {
    PieceCanvas canvas(96);
    if (canvas.surface() == nullptr) return nullptr;
    canvas.color(edge);
    constexpr std::array<std::array<int, 2>, 8> offsets{{{{-3, 0}}, {{3, 0}}, {{0, -3}}, {{0, 3}}, {{-2, -2}}, {{2, -2}}, {{-2, 2}}, {{2, 2}}}};
    for (const auto& offset : offsets) drawPieceShape(canvas, type, offset[0], offset[1]);
    canvas.color(fill);
    drawPieceShape(canvas, type, 0, 0);
    canvas.color(edge);
    drawPieceDetails(canvas, type);
    return canvas.release();
}


struct Vec3 { float x; float y; float z; };

class ChessApp {
public:
    ~ChessApp() {
        destroyPieceTextures();
        destroyModelTextures();
    }

    ChessApp(SDL_Renderer* renderer, InputMap& input)
        : renderer_(renderer), input_(input), computer_(std::filesystem::path("bin") / "stockfish") {
        settings_.load();
        chessforge::setLanguage(settings_.language);
        loadAutosave();
        resetCursors();
        modeApplied_ = settings_.twoPlayer;
        if (!settings_.twoPlayer && board_.sideToMove() != playerColor()) makeComputerMove();
    }

    // A pad button (see padid in input_map.hpp) went down or up.
    void onPad(int id, bool down, bool repeat) {
        if (captureAction_ >= 0) {
            if (!down || repeat) return;
            const bool swapped = input_.assign(static_cast<Action>(captureAction_), id);
            input_.save("saves/controls.cfg");
            captureAction_ = -1;
            notice_ = tr(swapped ? Str::NSwapped : Str::NBound);
            return;
        }
        if (!down) return;
        if (const auto action = input_.lookup(id)) perform(*action);
    }

    void handleEvent(const SDL_Event& event) {
        if (event.type == SDL_QUIT) running_ = false;
        if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT && !settingsOpen_ && !helpOpen_) handlePointer(event.button.x, event.button.y);
        if (event.type != SDL_KEYDOWN || event.key.repeat != 0) return;
        const SDL_Keycode key = event.key.keysym.sym;
        if (key == SDLK_ESCAPE) {
            escape();
            return;
        }
        if (captureAction_ >= 0) return;
        if (const auto action = keyAction(key)) perform(*action);
    }

    // Once per frame: the "press a button" prompt gives up after a few seconds.
    void tick() {
        if (captureAction_ >= 0 && SDL_GetTicks() >= captureDeadline_) {
            captureAction_ = -1;
            notice_ = tr(Str::NCancelled);
        }
    }

    bool running() const { return running_; }

    void draw() {
        const Theme palette = currentTheme();
        if (!settings_.threeDimensional) ensurePieceTextures(palette);
        fill(renderer_, {0, 0, ScreenWidth, ScreenHeight}, palette.background);
        if (settings_.threeDimensional) drawBoard3D(palette);
        else drawBoard2D(palette);
        if (!settingsOpen_ && !helpOpen_) drawSidebar(palette);
        if (settingsOpen_) drawSettings(palette);
        if (settingsOpen_ && controlsOpen_) drawControls(palette);
        if (helpOpen_) drawHelp(palette);
    }

private:
    enum class Row { Language, Mode, Elo, Side, Flip, Theme, Texture, Camera, Pieces, Light, Dark, Accent, Background, Controls };

    struct ModelInfo {
        int width = 0;
        int height = 0;
        float anchorX = 0.0F;
        float anchorY = 0.0F;
        float pixelsPerUnit = 1.0F;
    };

    static constexpr float CameraDistance = 13.0F;
    static constexpr int ModelPixelsPerUnit = 56;
    static constexpr int SettingsVisibleRows = 9;
    static constexpr int CaptureTimeoutMs = 5000;

    SDL_Renderer* renderer_;
    InputMap& input_;
    Settings settings_;
    Board board_;
    ComputerPlayer computer_;
    std::vector<Board> history_;
    int cursor_ = Board::square(4, 1);
    std::array<int, 2> cursors_{{Board::square(4, 1), Board::square(4, 6)}};  // two-player: remembered cursor of white / black
    Color cursorOwner_ = Color::White;
    bool modeApplied_ = false;
    int selected_ = -1;
    bool settingsOpen_ = false;
    bool helpOpen_ = false;
    bool controlsOpen_ = false;
    int settingIndex_ = 0;
    int controlsIndex_ = 0;
    int captureAction_ = -1;
    Uint32 captureDeadline_ = 0;
    int colorEdit_ = -1;
    int colorChannel_ = 0;
    bool running_ = true;
    std::string notice_;
    std::array<SDL_Texture*, 12> pieceTextures_{};
    bool piecesBuilt_ = false;
    std::string builtSet_;
    SDL_Color builtWhite_{};
    SDL_Color builtBlack_{};
    std::array<SDL_Texture*, 12> modelTextures_{};
    SDL_Texture* shadowTexture_ = nullptr;
    ModelInfo modelInfo_;
    bool modelsBuilt_ = false;
    float builtYaw_ = 0.0F;
    float builtPitch_ = 0.0F;
    SDL_Color builtModelWhite_{};
    SDL_Color builtModelBlack_{};
    float camScale_ = 100.0F;
    float camCx_ = 226.0F;
    float camCy_ = 251.0F;

    // ------------------------------------------------------------------ keys and actions

    static std::optional<Action> keyAction(SDL_Keycode key) {
        switch (key) {
            case SDLK_UP: return Action::Up;
            case SDLK_DOWN: return Action::Down;
            case SDLK_LEFT: return Action::Left;
            case SDLK_RIGHT: return Action::Right;
            case SDLK_SPACE: case SDLK_a: case SDLK_RETURN: return Action::Confirm;
            case SDLK_b: case SDLK_BACKSPACE: case SDLK_u: return Action::Back;
            case SDLK_TAB: case SDLK_x: return Action::ToggleView;
            case SDLK_s: case SDLK_y: return Action::Settings;
            case SDLK_h: case SDLK_F1: return Action::Help;
            case SDLK_n: return Action::NewGame;
            case SDLK_q: return Action::CamLeft;
            case SDLK_e: return Action::CamRight;
            default: return std::nullopt;
        }
    }

    void escape() {
        if (captureAction_ >= 0) {
            captureAction_ = -1;
            notice_ = tr(Str::NCancelled);
            return;
        }
        if (colorEdit_ >= 0) {
            colorEdit_ = -1;
            settings_.save();
            return;
        }
        if (controlsOpen_) {
            controlsOpen_ = false;
            return;
        }
        selected_ = -1;
        helpOpen_ = false;
    }

    void perform(Action action) {
        switch (action) {
            case Action::Quit: running_ = false; return;
            case Action::ToggleView: settings_.threeDimensional = !settings_.threeDimensional; settings_.save(); return;
            case Action::Settings:
                if (settingsOpen_) {
                    closeSettings();
                } else {
                    settingsOpen_ = true;
                    helpOpen_ = false;
                    colorEdit_ = -1;
                    controlsOpen_ = false;
                }
                return;
            case Action::Help:
                if (settingsOpen_) closeSettings();
                helpOpen_ = !helpOpen_;
                return;
            case Action::NewGame:
                if (!settingsOpen_ && !helpOpen_) newGame();
                return;
            default: break;
        }
        if (settingsOpen_) {
            handleSettingsAction(action);
            return;
        }
        if (helpOpen_) {
            if (action == Action::Back || action == Action::Confirm) helpOpen_ = false;
            return;
        }
        switch (action) {
            case Action::CamLeft: settings_.yaw -= 0.10F; settings_.save(); break;
            case Action::CamRight: settings_.yaw += 0.10F; settings_.save(); break;
            case Action::Back:
                if (selected_ >= 0) selected_ = -1;
                else undoTurn();
                break;
            case Action::Left: moveCursor(-1, 0); break;
            case Action::Right: moveCursor(1, 0); break;
            case Action::Up: moveCursor(0, -1); break;
            case Action::Down: moveCursor(0, 1); break;
            case Action::Confirm:
                if (gameOver()) newGame();
                else activateCursor();
                break;
            default: break;
        }
    }

    // ------------------------------------------------------------------ game state

    [[nodiscard]] Color playerColor() const { return settings_.playerWhite ? Color::White : Color::Black; }
    // Whose pieces the person holding the device moves right now.
    [[nodiscard]] Color controlColor() const { return settings_.twoPlayer ? board_.sideToMove() : playerColor(); }
    [[nodiscard]] bool humanTurn() const { return settings_.twoPlayer || board_.sideToMove() == playerColor(); }
    [[nodiscard]] bool gameOver() const { return board_.gameState() != GameState::Playing; }
    [[nodiscard]] bool flipped() const {
        if (settings_.twoPlayer) return settings_.autoFlip && board_.sideToMove() == Color::Black;
        return !settings_.playerWhite;
    }
    static std::size_t sideIndex(Color color) { return color == Color::White ? 0U : 1U; }

    [[nodiscard]] Theme currentTheme() const {
        if (settings_.theme < CustomTheme) return Themes[static_cast<std::size_t>(settings_.theme)];
        if (settings_.theme > CustomTheme) return LichessThemes[static_cast<std::size_t>(settings_.theme - CustomTheme - 1)];
        Theme custom = Themes[0];
        custom.background = settings_.customBackground;
        custom.panel = brighten(settings_.customBackground, 18);
        custom.lightSquare = settings_.customLight;
        custom.darkSquare = settings_.customDark;
        custom.accent = settings_.customAccent;
        custom.highlight = brighten(settings_.customAccent, 45);
        custom.whitePiece = brighten(settings_.customLight, 28);
        custom.blackPiece = darken(settings_.customDark, 58);
        custom.text = brighten(settings_.customBackground, 185);
        return custom;
    }

    void saveAutosave() const {
        std::filesystem::create_directories("saves");
        std::ofstream output("saves/autosave.fen", std::ios::trunc);
        output << board_.fen() << '\n';
    }

    void loadAutosave() {
        std::ifstream input("saves/autosave.fen");
        std::string stored;
        if (std::getline(input, stored) && board_.loadFen(stored)) notice_ = tr(Str::NRestored);
    }

    void resetCursors() {
        cursors_ = {Board::square(4, 1), Board::square(4, 6)};
        cursorOwner_ = board_.sideToMove();
        if (settings_.twoPlayer) cursor_ = cursors_[sideIndex(cursorOwner_)];
        else cursor_ = settings_.playerWhite ? Board::square(4, 1) : Board::square(4, 6);
    }

    // Two players keep their own cursor: after a move the cursor jumps to where the opponent left theirs.
    void syncCursorToTurn() {
        if (!settings_.twoPlayer) return;
        const Color side = board_.sideToMove();
        if (side == cursorOwner_) return;
        cursors_[sideIndex(cursorOwner_)] = cursor_;
        cursor_ = cursors_[sideIndex(side)];
        cursorOwner_ = side;
    }

    void newGame() {
        board_.reset();
        history_.clear();
        selected_ = -1;
        notice_ = tr(Str::NNew);
        resetCursors();
        saveAutosave();
        if (!settings_.twoPlayer && board_.sideToMove() != playerColor()) makeComputerMove();
    }

    // Called when the settings close: the mode or side may have changed in the middle of a game.
    void applyGameSettings() {
        selected_ = -1;
        if (settings_.twoPlayer != modeApplied_) {
            modeApplied_ = settings_.twoPlayer;
            cursorOwner_ = board_.sideToMove();
            cursors_ = {Board::square(4, 1), Board::square(4, 6)};
            cursors_[sideIndex(cursorOwner_)] = cursor_;
        }
        if (!settings_.twoPlayer && !gameOver() && board_.sideToMove() != playerColor()) makeComputerMove();
    }

    void moveCursor(int visualX, int visualY) {
        int fileDirection = flipped() ? -visualX : visualX;
        int rankDirection = flipped() ? visualY : -visualY;
        if (settings_.threeDimensional) {
            const int quarter = ((static_cast<int>(std::lround(effectiveYaw() / (Pi / 2.0F))) % 4) + 4) % 4;
            constexpr std::array<int, 4> cosines{{1, 0, -1, 0}};
            constexpr std::array<int, 4> sines{{0, 1, 0, -1}};
            const int cosine = cosines[static_cast<std::size_t>(quarter)];
            const int sine = sines[static_cast<std::size_t>(quarter)];
            const int up = -visualY;
            fileDirection = visualX * cosine + up * sine;
            rankDirection = -visualX * sine + up * cosine;
        }
        const int file = std::clamp(Board::fileOf(cursor_) + fileDirection, 0, 7);
        const int rank = std::clamp(Board::rankOf(cursor_) + rankDirection, 0, 7);
        cursor_ = Board::square(file, rank);
    }

    void activateCursor() {
        if (gameOver() || !humanTurn()) return;
        const Color mine = controlColor();
        const Piece piece = board_.at(cursor_);
        if (selected_ < 0) {
            if (!piece.empty() && piece.color == mine) {
                selected_ = cursor_;
                notice_ = tr(Str::NSelectDest);
            }
            return;
        }
        if (cursor_ == selected_) { selected_ = -1; return; }
        if (const auto move = board_.findLegalMove(selected_, cursor_)) {
            history_.push_back(board_);
            board_.makeMove(*move);
            selected_ = -1;
            saveAutosave();
            syncCursorToTurn();
            if (settings_.twoPlayer) notice_.clear();
            if (!gameOver()) makeComputerMove();
            return;
        }
        if (!piece.empty() && piece.color == mine) {
            selected_ = cursor_;
            notice_ = tr(Str::NPieceChanged);
        } else {
            notice_ = tr(Str::NIllegal);
        }
    }

    void makeComputerMove() {
        if (settings_.twoPlayer || gameOver() || board_.sideToMove() == playerColor()) return;
        notice_ = tr(computer_.hasStockfish() ? Str::NSfThink : Str::NBiThink);
        SDL_RenderPresent(renderer_);
        const int thinkingTime = std::clamp(140 + settings_.elo / 3, 200, 1100);
        if (const auto move = computer_.chooseMove(board_, settings_.elo, thinkingTime)) {
            history_.push_back(board_);
            board_.makeMove(*move);
            notice_ = tr(computer_.hasStockfish() ? Str::NSfMoved : Str::NBiMoved);
            saveAutosave();
        }
    }

    void undoTurn() {
        if (history_.empty()) return;
        const bool afterComputer = !settings_.twoPlayer && board_.sideToMove() == playerColor();
        const std::size_t steps = afterComputer && history_.size() >= 2 ? 2 : 1;
        board_ = history_[history_.size() - steps];
        history_.resize(history_.size() - steps);
        selected_ = -1;
        notice_ = tr(Str::NUndone);
        saveAutosave();
        syncCursorToTurn();
    }

    void handlePointer(int x, int y) {
        if (!settings_.threeDimensional && x >= BoardLeft && x < BoardLeft + BoardPixels && y >= BoardTop && y < BoardTop + BoardPixels) {
            int screenFile = (x - BoardLeft) / SquareSize;
            int screenRank = (y - BoardTop) / SquareSize;
            const int file = flipped() ? 7 - screenFile : screenFile;
            const int rank = flipped() ? screenRank : 7 - screenRank;
            cursor_ = Board::square(file, rank);
            activateCursor();
        }
    }

    // ------------------------------------------------------------------ settings

    [[nodiscard]] std::vector<Row> settingRows() const {
        std::vector<Row> rows{Row::Language, Row::Mode};
        if (settings_.twoPlayer) {
            rows.push_back(Row::Flip);
        } else {
            rows.push_back(Row::Elo);
            rows.push_back(Row::Side);
        }
        for (const Row row : {Row::Theme, Row::Texture, Row::Camera, Row::Pieces, Row::Light, Row::Dark, Row::Accent, Row::Background, Row::Controls}) rows.push_back(row);
        return rows;
    }

    void closeSettings() {
        settingsOpen_ = false;
        controlsOpen_ = false;
        colorEdit_ = -1;
        settings_.save();
        applyGameSettings();
    }

    void handleSettingsAction(Action action) {
        if (controlsOpen_) {
            handleControlsAction(action);
            return;
        }
        if (colorEdit_ >= 0) {
            SDL_Color* target = colorEdit_ == 0 ? &settings_.customLight : colorEdit_ == 1 ? &settings_.customDark : colorEdit_ == 2 ? &settings_.customAccent : &settings_.customBackground;
            if (action == Action::Confirm || action == Action::Back) {
                colorEdit_ = -1;
                settings_.save();
            } else if (action == Action::Up) {
                colorChannel_ = (colorChannel_ + 2) % 3;
            } else if (action == Action::Down) {
                colorChannel_ = (colorChannel_ + 1) % 3;
            } else if (action == Action::Left || action == Action::Right) {
                Uint8* channel = colorChannel_ == 0 ? &target->r : colorChannel_ == 1 ? &target->g : &target->b;
                const int delta = action == Action::Left ? -5 : 5;
                *channel = static_cast<Uint8>(std::clamp(static_cast<int>(*channel) + delta, 0, 255));
                settings_.theme = CustomTheme;
                settings_.save();
            }
            return;
        }
        const std::vector<Row> rows = settingRows();
        const int count = static_cast<int>(rows.size());
        settingIndex_ = std::clamp(settingIndex_, 0, count - 1);
        const Row row = rows[static_cast<std::size_t>(settingIndex_)];
        switch (action) {
            case Action::Confirm:
                if (row >= Row::Light && row <= Row::Background) {
                    colorEdit_ = static_cast<int>(row) - static_cast<int>(Row::Light);
                    colorChannel_ = 0;
                } else if (row == Row::Controls) {
                    controlsOpen_ = true;
                    controlsIndex_ = 0;
                } else {
                    closeSettings();
                }
                break;
            case Action::Back: closeSettings(); break;
            case Action::Up: settingIndex_ = (settingIndex_ + count - 1) % count; break;
            case Action::Down: settingIndex_ = (settingIndex_ + 1) % count; break;
            case Action::Left: adjustSetting(row, -1); break;
            case Action::Right: adjustSetting(row, 1); break;
            default: break;
        }
    }

    void adjustSetting(Row row, int direction) {
        switch (row) {
            case Row::Language:
                settings_.language = (settings_.language + direction + chessforge::LanguageCount) % chessforge::LanguageCount;
                chessforge::setLanguage(settings_.language);
                notice_.clear();
                break;
            case Row::Mode: settings_.twoPlayer = !settings_.twoPlayer; break;
            case Row::Elo: settings_.elo = std::clamp(settings_.elo + direction * 100, 400, 3200); break;
            case Row::Side: settings_.playerWhite = !settings_.playerWhite; break;
            case Row::Flip: settings_.autoFlip = !settings_.autoFlip; break;
            case Row::Theme: settings_.theme = (settings_.theme + direction + ThemeCount) % ThemeCount; break;
            case Row::Texture: settings_.texture = (settings_.texture + direction + 5) % 5; break;
            case Row::Camera: settings_.pitch = std::clamp(settings_.pitch + static_cast<float>(direction) * 0.05F, 0.30F, 1.10F); break;
            case Row::Pieces: {
                const std::vector<std::string> sets = pieceSets();
                std::size_t current = 0;
                for (std::size_t index = 0; index < sets.size(); ++index) if (sets[index] == settings_.pieceSet) current = index;
                const int count = static_cast<int>(sets.size());
                settings_.pieceSet = sets[static_cast<std::size_t>((static_cast<int>(current) + direction + count) % count)];
                break;
            }
            default: return;
        }
        settings_.save();
    }

    void handleControlsAction(Action action) {
        constexpr int Rows = ActionRows + 1;  // every action plus "reset to default"
        switch (action) {
            case Action::Up: controlsIndex_ = (controlsIndex_ + Rows - 1) % Rows; break;
            case Action::Down: controlsIndex_ = (controlsIndex_ + 1) % Rows; break;
            case Action::Back: controlsOpen_ = false; break;
            case Action::Confirm:
                if (controlsIndex_ == ActionRows) {
                    input_.resetDefaults();
                    input_.save("saves/controls.cfg");
                    notice_ = tr(Str::NControlsReset);
                } else {
                    captureAction_ = controlsIndex_;
                    captureDeadline_ = SDL_GetTicks() + CaptureTimeoutMs;
                }
                break;
            default: break;
        }
    }

    static constexpr int ActionRows = chessforge::ActionCount;

    static std::string actionName(Action action) {
        switch (action) {
            case Action::Up: return tr(Str::AUp);
            case Action::Down: return tr(Str::ADown);
            case Action::Left: return tr(Str::ALeft);
            case Action::Right: return tr(Str::ARight);
            case Action::Confirm: return tr(Str::AConfirm);
            case Action::Back: return tr(Str::ABack);
            case Action::ToggleView: return tr(Str::AView);
            case Action::Settings: return tr(Str::HintSettings);
            case Action::NewGame: return tr(Str::HintNew);
            case Action::Help: return tr(Str::HintHelp);
            case Action::CamLeft: return tr(Str::ACamLeft);
            case Action::CamRight: return tr(Str::ACamRight);
            case Action::Quit: return tr(Str::AQuit);
            case Action::Count: break;
        }
        return {};
    }

    [[nodiscard]] std::string hint(Action action, Str text) const { return input_.label(action) + " " + tr(text); }

    // "DPAD" when the four directions are on the D-pad, otherwise the four buttons.
    [[nodiscard]] std::string directionLabel() const {
        bool dpad = true;
        for (const Action action : {Action::Up, Action::Down, Action::Left, Action::Right}) {
            const auto& list = input_.bindings(action);
            if (list.empty() || list.front() < chessforge::padid::ControllerButton + chessforge::padid::DpadUp ||
                list.front() > chessforge::padid::ControllerButton + chessforge::padid::DpadRight) dpad = false;
        }
        if (dpad) return "DPAD";
        return input_.label(Action::Up) + " " + input_.label(Action::Down) + " " + input_.label(Action::Left) + " " + input_.label(Action::Right);
    }

    // ------------------------------------------------------------------ board geometry and colours

    SDL_Rect squareRect(int square) const {
        const int file = Board::fileOf(square);
        const int rank = Board::rankOf(square);
        const int shownFile = flipped() ? 7 - file : file;
        const int shownRank = flipped() ? rank : 7 - rank;
        return {BoardLeft + shownFile * SquareSize, BoardTop + shownRank * SquareSize, SquareSize, SquareSize};
    }

    bool legalDestination(int square) const {
        if (selected_ < 0) return false;
        for (const Move& move : board_.legalMovesFrom(selected_)) if (move.to == square) return true;
        return false;
    }

    SDL_Color texturedSquareColor(SDL_Color base, int file, int rank) const {
        const int grain = (file * 29 + rank * 17 + file * rank * 7) % 18;
        switch (settings_.texture) {
            case 1: return ((file + rank) & 1) == 0 ? brighten(base, grain / 2) : darken(base, grain / 2);
            case 2: return brighten(base, static_cast<int>(10.0F * std::sin(static_cast<float>(file * 3 + rank * 5))));
            case 3: return ((file + rank) & 1) == 0 ? darken(base, 10) : brighten(base, 6);
            case 4: return brighten(base, ((file * 11 + rank * 19) % 9) - 4);
            default: return base;
        }
    }

    SDL_Color squareColor(int square, const Theme& palette) const {
        const int file = Board::fileOf(square);
        const int rank = Board::rankOf(square);
        const SDL_Color base = ((file + rank) & 1) == 0 ? palette.darkSquare : palette.lightSquare;
        return texturedSquareColor(base, file, rank);
    }

    // The cursor colour is chosen against the real colour of the square under it, so it never blends in.
    SDL_Color cursorColor(int square, const Theme& palette) const {
        return pickContrast(squareColor(square, palette), {palette.highlight, palette.accent, darken(palette.accent, 100), darken(palette.highlight, 120)});
    }

    SDL_Color selectionColor(int square, const Theme& palette) const {
        std::optional<SDL_Color> avoid;
        if (square == cursor_) avoid = cursorColor(square, palette);
        return pickContrast(squareColor(square, palette), {palette.accent, palette.highlight, darken(palette.accent, 100), darken(palette.highlight, 120)}, avoid);
    }

    SDL_Color markerColor(int square, const Theme& palette) const {
        return pickContrast(squareColor(square, palette), {palette.accent, palette.highlight, darken(palette.accent, 100), darken(palette.highlight, 120)});
    }

    void drawTexture2D(const SDL_Rect& cell, int file, int rank, SDL_Color base) {
        if (settings_.texture == 1) {
            setColor(renderer_, ((file + rank) & 1) == 0 ? darken(base, 12) : brighten(base, 10));
            for (int line = 6; line < cell.h; line += 9) {
                const int shift = (file * 7 + rank * 3 + line) % 5;
                SDL_RenderDrawLine(renderer_, cell.x + 1, cell.y + line, cell.x + cell.w - 2, cell.y + line - 2 + shift);
            }
        } else if (settings_.texture == 2) {
            setColor(renderer_, brighten(base, 18));
            for (int line = 8; line < cell.w; line += 15) {
                SDL_RenderDrawLine(renderer_, cell.x + line, cell.y + 2, cell.x + line - 5, cell.y + cell.h - 3);
            }
        } else if (settings_.texture == 3) {
            setColor(renderer_, darken(base, 18));
            for (int line = -cell.h; line < cell.w; line += 10) {
                SDL_RenderDrawLine(renderer_, cell.x + line, cell.y, cell.x + line + cell.h, cell.y + cell.h);
                SDL_RenderDrawLine(renderer_, cell.x + line + cell.h, cell.y, cell.x + line, cell.y + cell.h);
            }
        } else if (settings_.texture == 4) {
            setColor(renderer_, brighten(base, 9));
            for (int x = 5; x < cell.w; x += 11) {
                for (int y = 5; y < cell.h; y += 11) SDL_RenderDrawPoint(renderer_, cell.x + x, cell.y + y);
            }
        }
    }

    static bool sameColor(SDL_Color a, SDL_Color b) { return a.r == b.r && a.g == b.g && a.b == b.b; }

    static std::size_t pieceIndex(Piece piece) {
        return (piece.color == Color::White ? 0U : 6U) + static_cast<std::size_t>(static_cast<int>(piece.type) - 1);
    }

    [[nodiscard]] std::vector<std::string> pieceSets() const {
        std::vector<std::string> sets{std::string()};
        std::error_code error;
        for (const auto& entry : std::filesystem::directory_iterator("pieces", error)) {
            if (entry.is_directory(error)) sets.push_back(entry.path().filename().string());
        }
        std::sort(sets.begin() + 1, sets.end());
        return sets;
    }

    // ------------------------------------------------------------------ 2D pieces (built-in vector art or BMP sets)

    void destroyPieceTextures() {
        for (SDL_Texture*& texture : pieceTextures_) {
            if (texture != nullptr) SDL_DestroyTexture(texture);
            texture = nullptr;
        }
        piecesBuilt_ = false;
    }

    SDL_Texture* loadCustomPiece(const std::string& set, bool white, std::size_t type) const {
        const std::string path = "pieces/" + set + "/" + (white ? 'w' : 'b') + "PNBRQK"[type] + ".bmp";
        SDL_Surface* loaded = SDL_LoadBMP(path.c_str());
        if (loaded == nullptr) return nullptr;
        if (!SDL_ISPIXELFORMAT_ALPHA(loaded->format->format)) SDL_SetColorKey(loaded, SDL_TRUE, SDL_MapRGB(loaded->format, 255, 0, 255));
        SDL_Surface* converted = SDL_ConvertSurfaceFormat(loaded, SDL_PIXELFORMAT_RGBA32, 0);
        SDL_FreeSurface(loaded);
        if (converted == nullptr) return nullptr;
        SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer_, converted);
        SDL_FreeSurface(converted);
        if (texture != nullptr) SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
        return texture;
    }

    void ensurePieceTextures(const Theme& palette) {
        if (piecesBuilt_ && builtSet_ == settings_.pieceSet && sameColor(builtWhite_, palette.whitePiece) && sameColor(builtBlack_, palette.blackPiece)) return;
        destroyPieceTextures();
        SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1");
        constexpr SDL_Color DarkEdge{22, 22, 28, 255};
        constexpr SDL_Color LightEdge{238, 238, 244, 255};
        constexpr std::array<PieceType, 6> types{{PieceType::Pawn, PieceType::Knight, PieceType::Bishop, PieceType::Rook, PieceType::Queen, PieceType::King}};
        for (std::size_t color = 0; color < 2; ++color) {
            for (std::size_t type = 0; type < 6; ++type) {
                SDL_Texture* texture = settings_.pieceSet.empty() ? nullptr : loadCustomPiece(settings_.pieceSet, color == 0, type);
                if (texture == nullptr) {
                    SDL_Surface* art = renderBuiltinPiece(types[type], color == 0 ? palette.whitePiece : palette.blackPiece, color == 0 ? DarkEdge : LightEdge);
                    if (art != nullptr) {
                        texture = SDL_CreateTextureFromSurface(renderer_, art);
                        SDL_FreeSurface(art);
                        if (texture != nullptr) SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
                    }
                }
                pieceTextures_[color * 6 + type] = texture;
            }
        }
        builtSet_ = settings_.pieceSet;
        builtWhite_ = palette.whitePiece;
        builtBlack_ = palette.blackPiece;
        piecesBuilt_ = true;
    }

    void drawPiece2D(int square, Piece piece, const Theme&) {
        const SDL_Rect cell = squareRect(square);
        SDL_Texture* texture = pieceTextures_[pieceIndex(piece)];
        if (texture == nullptr) return;
        const SDL_Rect target{cell.x + 3, cell.y + 3, cell.w - 6, cell.h - 6};
        SDL_RenderCopy(renderer_, texture, nullptr, &target);
    }

    // Thick frame with a thin black/white halo on both sides so it is visible on any colour.
    void drawFrame2D(SDL_Rect cell, SDL_Color color, int inset, int thickness) {
        const SDL_Color halo = haloFor(color);
        const auto ring = [&](int offset, SDL_Color c, int width) {
            outline(renderer_, {cell.x + offset, cell.y + offset, cell.w - 2 * offset, cell.h - 2 * offset}, c, width);
        };
        ring(inset, halo, 1);
        ring(inset + 1, color, thickness);
        ring(inset + 1 + thickness, halo, 1);
    }

    // Corner brackets mark a square where the selected piece can capture.
    void drawCorners2D(SDL_Rect cell, SDL_Color color) {
        const SDL_Color halo = haloFor(color);
        constexpr int Length = 14;
        constexpr int Thick = 3;
        const auto bracket = [&](int cornerX, int cornerY, int dx, int dy, SDL_Color c, int grow) {
            const int x0 = cornerX;
            const int y0 = cornerY;
            const SDL_Rect horizontal{dx > 0 ? x0 - grow : x0 - Length - grow, dy > 0 ? y0 - grow : y0 - Thick - grow, Length + 2 * grow, Thick + 2 * grow};
            const SDL_Rect vertical{dx > 0 ? x0 - grow : x0 - Thick - grow, dy > 0 ? y0 - grow : y0 - Length - grow, Thick + 2 * grow, Length + 2 * grow};
            fill(renderer_, horizontal, c);
            fill(renderer_, vertical, c);
        };
        for (const SDL_Color& pass : {halo, color}) {
            const int grow = sameColor(pass, halo) ? 1 : 0;
            bracket(cell.x + 3, cell.y + 3, 1, 1, pass, grow);
            bracket(cell.x + cell.w - 3, cell.y + 3, -1, 1, pass, grow);
            bracket(cell.x + 3, cell.y + cell.h - 3, 1, -1, pass, grow);
            bracket(cell.x + cell.w - 3, cell.y + cell.h - 3, -1, -1, pass, grow);
        }
    }

    void drawBoard2D(const Theme& palette) {
        for (int square = 0; square < 64; ++square) {
            const int file = Board::fileOf(square);
            const int rank = Board::rankOf(square);
            const SDL_Color color = squareColor(square, palette);
            const SDL_Rect cell = squareRect(square);
            fill(renderer_, cell, color);
            drawTexture2D(cell, file, rank, color);
            if (legalDestination(square) && board_.at(square).empty()) {
                const SDL_Color marker = markerColor(square, palette);
                fill(renderer_, {cell.x + 18, cell.y + 18, 16, 16}, haloFor(marker));
                fill(renderer_, {cell.x + 19, cell.y + 19, 14, 14}, marker);
            }
        }
        for (int square = 0; square < 64; ++square) {
            const Piece piece = board_.at(square);
            if (!piece.empty()) drawPiece2D(square, piece, palette);
        }
        for (int square = 0; square < 64; ++square) {
            if (legalDestination(square) && !board_.at(square).empty()) drawCorners2D(squareRect(square), markerColor(square, palette));
        }
        if (selected_ >= 0) drawFrame2D(squareRect(selected_), selectionColor(selected_, palette), 0, 3);
        drawFrame2D(squareRect(cursor_), cursorColor(cursor_, palette), cursor_ == selected_ ? 5 : 0, 3);
        for (int column = 0; column < 8; ++column) {
            const int file = flipped() ? 7 - column : column;
            drawText(renderer_, BoardLeft + column * SquareSize + (SquareSize - 10) / 2, 14, std::string(1, static_cast<char>('A' + file)), 2, palette.text);
        }
        for (int row = 0; row < 8; ++row) {
            const int rank = flipped() ? row : 7 - row;
            drawText(renderer_, BoardLeft + BoardPixels + 5, BoardTop + row * SquareSize + (SquareSize - 7) / 2, std::to_string(rank + 1), 1, palette.text);
        }
    }

    // ------------------------------------------------------------------ 3D board

    [[nodiscard]] float effectiveYaw() const { return settings_.yaw + (flipped() ? Pi : 0.0F); }

    // World -> camera space: x right, y up, z = depth (larger is farther away).
    [[nodiscard]] Vec3 toCamera(Vec3 point) const {
        const float yaw = effectiveYaw();
        const float cosineYaw = std::cos(yaw);
        const float sineYaw = std::sin(yaw);
        const float x1 = point.x * cosineYaw - point.z * sineYaw;
        const float z1 = point.x * sineYaw + point.z * cosineYaw;
        const float cosinePitch = std::cos(settings_.pitch);
        const float sinePitch = std::sin(settings_.pitch);
        return {x1, point.y * cosinePitch + z1 * sinePitch, z1 * cosinePitch - point.y * sinePitch};
    }

    // Fits the whole board (with pieces) into the area left of the sidebar for any yaw/pitch.
    void updateCamera() {
        float minX = 1e9F;
        float maxX = -1e9F;
        float minY = 1e9F;
        float maxY = -1e9F;
        for (const float x : {-4.3F, 4.3F}) {
            for (const float z : {-4.3F, 4.3F}) {
                for (const float y : {0.0F, 1.35F}) {
                    const Vec3 c = toCamera({x, y, z});
                    const float inverse = 1.0F / std::max(3.0F, CameraDistance + c.z);
                    minX = std::min(minX, c.x * inverse);
                    maxX = std::max(maxX, c.x * inverse);
                    minY = std::min(minY, -c.y * inverse);
                    maxY = std::max(maxY, -c.y * inverse);
                }
            }
        }
        camScale_ = std::min(430.0F / (maxX - minX), 420.0F / (maxY - minY));
        camCx_ = 226.0F - (minX + maxX) * 0.5F * camScale_;
        camCy_ = 256.0F - (minY + maxY) * 0.5F * camScale_;
    }

    [[nodiscard]] float perspectiveAt(Vec3 point) const {
        return camScale_ / std::max(3.0F, CameraDistance + toCamera(point).z);
    }

    SDL_Point project(Vec3 point) const {
        const Vec3 c = toCamera(point);
        const float perspective = camScale_ / std::max(3.0F, CameraDistance + c.z);
        return {static_cast<int>(std::lround(camCx_ + c.x * perspective)), static_cast<int>(std::lround(camCy_ - c.y * perspective))};
    }

    void fillTriangle(SDL_Point first, SDL_Point second, SDL_Point third, SDL_Color color) {
        std::array<SDL_Point, 3> points{first, second, third};
        std::sort(points.begin(), points.end(), [](const SDL_Point& left, const SDL_Point& right) { return left.y < right.y; });
        const auto drawSpan = [&](int y, float left, float right) {
            if (y < 0 || y >= ScreenHeight) return;
            int x1 = static_cast<int>(std::ceil(std::min(left, right)));
            int x2 = static_cast<int>(std::floor(std::max(left, right)));
            x1 = std::clamp(x1, 0, ScreenWidth - 1);
            x2 = std::clamp(x2, 0, ScreenWidth - 1);
            if (x2 >= x1) SDL_RenderDrawLine(renderer_, x1, y, x2, y);
        };
        const auto edge = [](SDL_Point a, SDL_Point b, int y) {
            if (a.y == b.y) return static_cast<float>(a.x);
            return static_cast<float>(a.x) + static_cast<float>(y - a.y) * static_cast<float>(b.x - a.x) / static_cast<float>(b.y - a.y);
        };
        setColor(renderer_, color);
        for (int y = points[0].y; y <= points[1].y; ++y) drawSpan(y, edge(points[0], points[2], y), edge(points[0], points[1], y));
        for (int y = points[1].y; y <= points[2].y; ++y) drawSpan(y, edge(points[0], points[2], y), edge(points[1], points[2], y));
    }

    void fillQuad(Vec3 a, Vec3 b, Vec3 c, Vec3 d, SDL_Color color) {
        const SDL_Point pa = project(a);
        const SDL_Point pb = project(b);
        const SDL_Point pc = project(c);
        const SDL_Point pd = project(d);
        fillTriangle(pa, pb, pc, color);
        fillTriangle(pa, pc, pd, color);
    }

    void drawTexture3D(int file, int rank, SDL_Color base) {
        const float x = static_cast<float>(file) - 4.0F;
        const float z = static_cast<float>(rank) - 4.0F;
        if (settings_.texture == 1) {
            for (int strip = 1; strip < 5; ++strip) {
                const float offset = static_cast<float>(strip) * 0.19F;
                fillQuad({x, 0.012F, z + offset}, {x + 1.0F, 0.012F, z + offset}, {x + 1.0F, 0.012F, z + offset + 0.018F}, {x, 0.012F, z + offset + 0.018F}, darken(base, 16));
            }
        } else if (settings_.texture == 2) {
            fillQuad({x + 0.18F, 0.012F, z}, {x + 0.22F, 0.012F, z}, {x + 0.48F, 0.012F, z + 1.0F}, {x + 0.44F, 0.012F, z + 1.0F}, brighten(base, 16));
            fillQuad({x + 0.71F, 0.012F, z}, {x + 0.74F, 0.012F, z}, {x + 0.53F, 0.012F, z + 1.0F}, {x + 0.50F, 0.012F, z + 1.0F}, brighten(base, 12));
        } else if (settings_.texture == 3) {
            for (int strip = 1; strip < 4; ++strip) {
                const float offset = static_cast<float>(strip) * 0.25F;
                fillQuad({x + offset, 0.012F, z}, {x + offset + 0.018F, 0.012F, z}, {x + offset + 0.018F, 0.012F, z + 1.0F}, {x + offset, 0.012F, z + 1.0F}, darken(base, 18));
                fillQuad({x, 0.013F, z + offset}, {x + 1.0F, 0.013F, z + offset}, {x + 1.0F, 0.013F, z + offset + 0.018F}, {x, 0.013F, z + offset + 0.018F}, darken(base, 18));
            }
        }
    }

    // One square outline, shrunk by `inset` (in squares), lying slightly above the board.
    void drawSquareOutline3D(int square, float inset, SDL_Color color) {
        const float x = static_cast<float>(Board::fileOf(square)) - 4.0F + inset;
        const float z = static_cast<float>(Board::rankOf(square)) - 4.0F + inset;
        const float size = 1.0F - 2.0F * inset;
        const std::array<SDL_Point, 4> points{{project({x, 0.02F, z}), project({x + size, 0.02F, z}), project({x + size, 0.02F, z + size}), project({x, 0.02F, z + size})}};
        setColor(renderer_, color);
        for (int index = 0; index < 4; ++index) SDL_RenderDrawLine(renderer_, points[static_cast<std::size_t>(index)].x, points[static_cast<std::size_t>(index)].y, points[static_cast<std::size_t>((index + 1) % 4)].x, points[static_cast<std::size_t>((index + 1) % 4)].y);
    }

    // Thick frame (three lines plus a halo on both sides) so the cursor stays visible on every square colour.
    void drawFrame3D(int square, SDL_Color color, float inset) {
        const SDL_Color halo = haloFor(color);
        drawSquareOutline3D(square, inset, halo);
        for (const float step : {0.028F, 0.052F, 0.076F}) drawSquareOutline3D(square, inset + step, color);
        drawSquareOutline3D(square, inset + 0.10F, halo);
    }

    void drawDisc3D(int square, float radius, SDL_Color color) {
        const Vec3 centre{static_cast<float>(Board::fileOf(square)) - 3.5F, 0.02F, static_cast<float>(Board::rankOf(square)) - 3.5F};
        constexpr int Segments = 12;
        const SDL_Point middle = project(centre);
        std::array<SDL_Point, Segments> ring{};
        for (int i = 0; i < Segments; ++i) {
            const float angle = 2.0F * Pi * static_cast<float>(i) / static_cast<float>(Segments);
            ring[static_cast<std::size_t>(i)] = project({centre.x + radius * std::cos(angle), centre.y, centre.z + radius * std::sin(angle)});
        }
        for (int i = 0; i < Segments; ++i) fillTriangle(middle, ring[static_cast<std::size_t>(i)], ring[static_cast<std::size_t>((i + 1) % Segments)], color);
    }

    void drawRing3D(int square, float radius, SDL_Color color) {
        const Vec3 centre{static_cast<float>(Board::fileOf(square)) - 3.5F, 0.02F, static_cast<float>(Board::rankOf(square)) - 3.5F};
        constexpr int Segments = 24;
        setColor(renderer_, color);
        SDL_Point previous = project({centre.x + radius, centre.y, centre.z});
        for (int i = 1; i <= Segments; ++i) {
            const float angle = 2.0F * Pi * static_cast<float>(i) / static_cast<float>(Segments);
            const SDL_Point next = project({centre.x + radius * std::cos(angle), centre.y, centre.z + radius * std::sin(angle)});
            SDL_RenderDrawLine(renderer_, previous.x, previous.y, next.x, next.y);
            previous = next;
        }
    }

    void drawMarker3D(int square, const Theme& palette) {
        const SDL_Color color = markerColor(square, palette);
        const SDL_Color halo = haloFor(color);
        if (board_.at(square).empty()) {
            drawDisc3D(square, 0.17F, halo);
            drawDisc3D(square, 0.125F, color);
        } else {
            for (const float radius : {0.37F, 0.465F}) drawRing3D(square, radius, halo);
            for (const float radius : {0.40F, 0.425F, 0.44F}) drawRing3D(square, radius, color);
        }
    }

    // ------------------------------------------------------------------ 3D piece models (pre-rendered, shaded sprites)

    void destroyModelTextures() {
        for (SDL_Texture*& texture : modelTextures_) {
            if (texture != nullptr) SDL_DestroyTexture(texture);
            texture = nullptr;
        }
        if (shadowTexture_ != nullptr) SDL_DestroyTexture(shadowTexture_);
        shadowTexture_ = nullptr;
        modelsBuilt_ = false;
    }

    SDL_Texture* textureFromSprite(const chessforge::SpriteImage& sprite) const {
        SDL_Surface* surface = SDL_CreateRGBSurfaceWithFormat(0, sprite.width, sprite.height, 32, SDL_PIXELFORMAT_RGBA32);
        if (surface == nullptr) return nullptr;
        const std::size_t rowBytes = static_cast<std::size_t>(sprite.width) * 4U;
        for (int y = 0; y < sprite.height; ++y) {
            std::memcpy(static_cast<Uint8*>(surface->pixels) + static_cast<std::size_t>(y) * static_cast<std::size_t>(surface->pitch),
                        sprite.rgba.data() + static_cast<std::size_t>(y) * rowBytes, rowBytes);
        }
        SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer_, surface);
        SDL_FreeSurface(surface);
        if (texture != nullptr) SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
        return texture;
    }

    // The models are rendered once per camera angle / colour set (not every frame).
    void ensureModels(const Theme& palette) {
        const float yaw = effectiveYaw();
        const float pitch = settings_.pitch;
        if (modelsBuilt_ && std::abs(builtYaw_ - yaw) < 1e-4F && std::abs(builtPitch_ - pitch) < 1e-4F &&
            sameColor(builtModelWhite_, palette.whitePiece) && sameColor(builtModelBlack_, palette.blackPiece)) return;
        destroyModelTextures();
        SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1");
        constexpr std::array<PieceType, 6> types{{PieceType::Pawn, PieceType::Knight, PieceType::Bishop, PieceType::Rook, PieceType::Queen, PieceType::King}};
        for (std::size_t color = 0; color < 2; ++color) {
            const SDL_Color body = color == 0 ? palette.whitePiece : palette.blackPiece;
            const chessforge::Rgb8 edge = relativeLuminance(body) > 0.3 ? chessforge::Rgb8{22, 22, 28} : chessforge::Rgb8{238, 238, 244};
            for (std::size_t type = 0; type < 6; ++type) {
                const chessforge::SpriteImage sprite = chessforge::renderPieceSprite(types[type], color == 0, yaw, pitch, {body.r, body.g, body.b}, edge, ModelPixelsPerUnit);
                modelInfo_ = {sprite.width, sprite.height, sprite.anchorX, sprite.anchorY, sprite.pixelsPerUnit};
                modelTextures_[color * 6 + type] = textureFromSprite(sprite);
            }
        }
        shadowTexture_ = textureFromSprite(chessforge::renderShadowSprite(64));
        builtYaw_ = yaw;
        builtPitch_ = pitch;
        builtModelWhite_ = palette.whitePiece;
        builtModelBlack_ = palette.blackPiece;
        modelsBuilt_ = true;
    }

    void drawPiece3D(int square, Piece piece, bool lifted) {
        SDL_Texture* texture = modelTextures_[pieceIndex(piece)];
        if (texture == nullptr) return;
        const Vec3 centre{static_cast<float>(Board::fileOf(square)) - 3.5F, 0.0F, static_cast<float>(Board::rankOf(square)) - 3.5F};
        const float perspective = perspectiveAt(centre);
        const SDL_Point base = project(centre);
        if (shadowTexture_ != nullptr) {
            const float width = 1.0F * perspective;
            const float height = std::max(4.0F, width * std::sin(settings_.pitch));
            const SDL_Rect shadow{static_cast<int>(std::lround(static_cast<float>(base.x) - width / 2.0F)), static_cast<int>(std::lround(static_cast<float>(base.y) - height / 2.0F)),
                                  static_cast<int>(std::lround(width)), static_cast<int>(std::lround(height))};
            SDL_RenderCopy(renderer_, shadowTexture_, nullptr, &shadow);
        }
        const float scale = perspective / modelInfo_.pixelsPerUnit;
        const float lift = lifted ? 0.22F * perspective * std::cos(settings_.pitch) : 0.0F;
        const SDL_Rect target{static_cast<int>(std::lround(static_cast<float>(base.x) - modelInfo_.anchorX * scale)),
                              static_cast<int>(std::lround(static_cast<float>(base.y) - modelInfo_.anchorY * scale - lift)),
                              static_cast<int>(std::lround(static_cast<float>(modelInfo_.width) * scale)),
                              static_cast<int>(std::lround(static_cast<float>(modelInfo_.height) * scale))};
        SDL_RenderCopy(renderer_, texture, nullptr, &target);
    }

    void drawBoard3D(const Theme& palette) {
        updateCamera();
        ensureModels(palette);
        for (int rank = 0; rank < 8; ++rank) {
            for (int file = 0; file < 8; ++file) {
                const SDL_Color color = squareColor(Board::square(file, rank), palette);
                const float x = static_cast<float>(file) - 4.0F;
                const float z = static_cast<float>(rank) - 4.0F;
                fillQuad({x, 0.0F, z}, {x + 1.0F, 0.0F, z}, {x + 1.0F, 0.0F, z + 1.0F}, {x, 0.0F, z + 1.0F}, color);
                drawTexture3D(file, rank, color);
            }
        }
        // Coordinates along the white edge (files) and the left edge (ranks) of the board.
        for (int index = 0; index < 8; ++index) {
            const SDL_Point fileLabel = project({static_cast<float>(index) - 3.5F, 0.0F, -4.28F});
            drawText(renderer_, fileLabel.x - 3, fileLabel.y - 4, std::string(1, static_cast<char>('A' + index)), 1, palette.text);
            const SDL_Point rankLabel = project({-4.28F, 0.0F, static_cast<float>(index) - 3.5F});
            drawText(renderer_, rankLabel.x - 3, rankLabel.y - 4, std::to_string(index + 1), 1, palette.text);
        }
        for (int square = 0; square < 64; ++square) {
            if (legalDestination(square)) drawMarker3D(square, palette);
        }

        // Painter's algorithm, farthest first. The selection and cursor frames take part in the ordering
        // so that a piece standing in front of the cursor square hides the frame instead of being covered by it.
        struct Item {
            float depth;
            int kind;  // 0 piece, 1 selection frame, 2 cursor frame
            int square;
        };
        std::vector<Item> order;
        for (int square = 0; square < 64; ++square) {
            const Vec3 ground{static_cast<float>(Board::fileOf(square)) - 3.5F, 0.0F, static_cast<float>(Board::rankOf(square)) - 3.5F};
            const Vec3 body{ground.x, 0.3F, ground.z};
            if (!board_.at(square).empty()) order.push_back({toCamera(body).z, 0, square});
            if (square == selected_) order.push_back({toCamera(ground).z + 0.05F, 1, square});
            if (square == cursor_) order.push_back({toCamera(ground).z + 0.06F, 2, square});
        }
        std::sort(order.begin(), order.end(), [](const Item& left, const Item& right) { return left.depth > right.depth; });
        for (const Item& item : order) {
            if (item.kind == 0) drawPiece3D(item.square, board_.at(item.square), item.square == selected_);
            else if (item.kind == 1) drawFrame3D(item.square, selectionColor(item.square, palette), 0.0F);
            else drawFrame3D(item.square, cursorColor(item.square, palette), cursor_ == selected_ ? 0.13F : 0.0F);
        }
        drawText(renderer_, 16, 16, tr(Str::Board3dLabel), 2, palette.text);
    }

    // ------------------------------------------------------------------ panels

    std::string stateLabel() const {
        switch (board_.gameState()) {
            case GameState::Playing: return tr(board_.sideToMove() == Color::White ? Str::StateWhiteMove : Str::StateBlackMove);
            case GameState::WhiteWon: return tr(Str::StateWhiteWins);
            case GameState::BlackWon: return tr(Str::StateBlackWins);
            case GameState::DrawStalemate: return tr(Str::StateStalemate);
            case GameState::DrawFiftyMove: return tr(Str::StateFifty);
        }
        return "";
    }

    void drawSidebar(const Theme& palette) {
        const int x = 450;
        const int inner = 160;
        fill(renderer_, {x, 16, 178, 448}, palette.panel);
        outline(renderer_, {x, 16, 178, 448}, palette.accent, 1);
        drawTextFit(renderer_, x + 14, 26, "CHESSFORGE", 3, inner, palette.text);
        drawText(renderer_, x + 14, 54, "3D", 3, palette.accent);
        drawTextFit(renderer_, x + 14, 92, stateLabel(), 2, inner, palette.highlight);
        if (board_.gameState() == GameState::Playing && board_.inCheck(board_.sideToMove())) {
            drawTextFit(renderer_, x + 14, 112, tr(Str::InCheck), 2, inner, pickContrast(palette.panel, {SDL_Color{255, 112, 92, 255}}));
        }
        if (settings_.twoPlayer) {
            drawTextFit(renderer_, x + 14, 146, tr(Str::TwoPlayers), 2, inner, palette.text);
        } else {
            drawText(renderer_, x + 14, 140, "ELO " + std::to_string(settings_.elo), 3, palette.text);
            drawTextFit(renderer_, x + 14, 170, tr(computer_.hasStockfish() ? Str::EngineStockfish : Str::EngineBuiltin), 2, inner, palette.text);
            drawTextFit(renderer_, x + 14, 196, tr(settings_.playerWhite ? Str::YouWhite : Str::YouBlack), 2, inner, palette.text);
        }
        drawTextFit(renderer_, x + 14, 226, tr(settings_.threeDimensional ? Str::View3d : Str::View2d), 2, inner, palette.text);
        const std::array<std::string, 7> hints{{
            hint(Action::Confirm, Str::HintMove),
            hint(Action::Back, Str::HintUndo),
            hint(Action::ToggleView, Str::HintView),
            hint(Action::Settings, Str::HintSettings),
            hint(Action::NewGame, Str::HintNewShort),
            input_.label(Action::CamLeft) + " " + input_.label(Action::CamRight) + " " + tr(Str::HintCamera),
            hint(Action::Help, Str::HintHelp),
        }};
        for (std::size_t index = 0; index < hints.size(); ++index) {
            drawTextFit(renderer_, x + 14, 262 + static_cast<int>(index) * 24, hints[index], 2, inner, palette.text);
        }
        if (!notice_.empty()) drawTextFit(renderer_, x + 14, 436, notice_, 1, inner, palette.highlight);
    }

    std::string rowLabel(Row row) const {
        switch (row) {
            case Row::Language: return tr(Str::SLanguage);
            case Row::Mode: return tr(Str::SMode);
            case Row::Elo: return tr(Str::SElo);
            case Row::Side: return tr(Str::SSide);
            case Row::Flip: return tr(Str::SFlip);
            case Row::Theme: return tr(Str::STheme);
            case Row::Texture: return tr(Str::STexture);
            case Row::Camera: return tr(Str::SCamera);
            case Row::Pieces: return tr(Str::SPieces);
            case Row::Light: return tr(Str::SLight);
            case Row::Dark: return tr(Str::SDark);
            case Row::Accent: return tr(Str::SAccent);
            case Row::Background: return tr(Str::SBack);
            case Row::Controls: return tr(Str::SControls);
        }
        return {};
    }

    std::string rowValue(Row row) const {
        const auto rgb = [](SDL_Color color) { return std::to_string(color.r) + " " + std::to_string(color.g) + " " + std::to_string(color.b); };
        const std::array<Str, 5> themeNames{{Str::VOak, Str::VViolet, Str::VForest, Str::VMono, Str::VCustom}};
        const std::array<const char*, 4> lichessNames{{"BROWN", "BLUE", "GREEN", "PURPLE"}};
        const std::array<Str, 5> textureNames{{Str::VFlat, Str::VWood, Str::VMarble, Str::VCarbon, Str::VFelt}};
        switch (row) {
            case Row::Language: return chessforge::languageName(settings_.language);
            case Row::Mode: return tr(settings_.twoPlayer ? Str::TwoPlayers : Str::VVsComputer);
            case Row::Elo: return std::to_string(settings_.elo);
            case Row::Side: return tr(settings_.playerWhite ? Str::VWhite : Str::VBlack);
            case Row::Flip: return tr(settings_.autoFlip ? Str::VAutoFlip : Str::VFixed);
            case Row::Theme:
                if (settings_.theme <= CustomTheme) return tr(themeNames[static_cast<std::size_t>(settings_.theme)]);
                return std::string("LICHESS ") + lichessNames[static_cast<std::size_t>(settings_.theme - CustomTheme - 1)];
            case Row::Texture: return tr(textureNames[static_cast<std::size_t>(settings_.texture)]);
            case Row::Camera: return std::to_string(static_cast<int>(settings_.pitch * 100.0F));
            case Row::Pieces: return settings_.pieceSet.empty() ? std::string(tr(Str::VBuiltin)) : settings_.pieceSet;
            case Row::Light: return rgb(settings_.customLight);
            case Row::Dark: return rgb(settings_.customDark);
            case Row::Accent: return rgb(settings_.customAccent);
            case Row::Background: return rgb(settings_.customBackground);
            case Row::Controls: return ">";
        }
        return {};
    }

    // Small triangle used as "more rows above / below" marker.
    void drawArrow(int centreX, int y, bool up, SDL_Color color) {
        setColor(renderer_, color);
        for (int line = 0; line < 5; ++line) {
            const int half = up ? line : 4 - line;
            SDL_RenderDrawLine(renderer_, centreX - half, y + line, centreX + half, y + line);
        }
    }

    // Keeps the selected row inside the window of visible rows.
    static int scrollTop(int selected, int count, int visible) {
        if (count <= visible) return 0;
        return std::clamp(selected - visible / 2, 0, count - visible);
    }

    void drawSettings(const Theme& palette) {
        fill(renderer_, {50, 42, 540, 402}, darken(palette.panel, 4));
        outline(renderer_, {50, 42, 540, 402}, palette.accent, 3);
        drawTextFit(renderer_, 76, 60, tr(Str::Settings), 3, 400, palette.text);
        if (colorEdit_ >= 0) {
            const SDL_Color edited = colorEdit_ == 0 ? settings_.customLight : colorEdit_ == 1 ? settings_.customDark : colorEdit_ == 2 ? settings_.customAccent : settings_.customBackground;
            const std::array<Str, 4> names{{Str::ELight, Str::EDark, Str::EAccent, Str::EBackground}};
            fill(renderer_, {110, 134, 420, 220}, palette.background);
            outline(renderer_, {110, 134, 420, 220}, palette.highlight, 2);
            drawTextFit(renderer_, 138, 158, std::string(tr(Str::EEdit)) + " " + tr(names[static_cast<std::size_t>(colorEdit_)]), 2, 364, palette.text);
            const std::array<Str, 3> channels{{Str::ERed, Str::EGreen, Str::EBlue}};
            const std::array<int, 3> values{{edited.r, edited.g, edited.b}};
            for (int index = 0; index < 3; ++index) {
                const int y = 204 + index * 34;
                const bool active = index == colorChannel_;
                if (active) fill(renderer_, {132, y - 4, 240, 25}, palette.accent);
                drawTextFit(renderer_, 146, y, tr(channels[static_cast<std::size_t>(index)]), 2, 150, active ? palette.background : palette.text);
                drawText(renderer_, 310, y, std::to_string(values[static_cast<std::size_t>(index)]), 2, active ? palette.background : palette.highlight);
            }
            fill(renderer_, {405, 202, 88, 88}, edited);
            outline(renderer_, {405, 202, 88, 88}, palette.text, 2);
            drawTextFit(renderer_, 128, 318, tr(Str::EHelp), 1, 388, palette.text);
            drawTextFit(renderer_, 128, 336, input_.label(Action::Confirm) + ": " + tr(Str::ESave), 1, 388, palette.highlight);
            return;
        }
        const std::vector<Row> rows = settingRows();
        const int count = static_cast<int>(rows.size());
        const int selected = std::clamp(settingIndex_, 0, count - 1);
        const int top = scrollTop(selected, count, SettingsVisibleRows);
        for (int slot = 0; slot < std::min(count, SettingsVisibleRows); ++slot) {
            const int index = top + slot;
            const Row row = rows[static_cast<std::size_t>(index)];
            const int y = 104 + slot * 28;
            const bool active = index == selected;
            if (active) fill(renderer_, {68, y - 4, 504, 24}, palette.accent);
            const SDL_Color labelColor = active ? palette.background : palette.text;
            const SDL_Color valueColor = active ? palette.background : palette.highlight;
            const std::string label = rowLabel(row);
            const int labelScale = fitScale(label, 2, 230);
            drawText(renderer_, 82, y + (2 - labelScale) * 7 / 2, label, labelScale, labelColor);
            const int room = 550 - 82 - textWidth(label, labelScale) - 14;
            drawTextRight(renderer_, 550, y, rowValue(row), 2, std::max(60, room), valueColor);
        }
        if (top > 0) drawArrow(562, 90, true, palette.highlight);
        if (top + SettingsVisibleRows < count) drawArrow(562, 352, false, palette.highlight);
        drawTextFit(renderer_, 74, 366, tr(Str::FChange), 1, 480, palette.text);
        drawTextFit(renderer_, 74, 382, input_.label(Action::Confirm) + ": " + tr(Str::FConfirm), 1, 480, palette.text);
        drawTextFit(renderer_, 74, 398, tr(Str::FPieceFolder), 1, 480, palette.text);
        drawTextFit(renderer_, 74, 414, input_.label(Action::Back) + ": " + tr(Str::FBack), 1, 480, palette.highlight);
    }

    void drawControls(const Theme& palette) {
        fill(renderer_, {40, 36, 560, 414}, darken(palette.panel, 8));
        outline(renderer_, {40, 36, 560, 414}, palette.highlight, 3);
        drawTextFit(renderer_, 66, 52, tr(Str::SControls), 3, 440, palette.text);
        constexpr int Rows = ActionRows + 1;
        const int top = scrollTop(controlsIndex_, Rows, SettingsVisibleRows);
        for (int slot = 0; slot < SettingsVisibleRows; ++slot) {
            const int index = top + slot;
            const int y = 96 + slot * 30;
            const bool active = index == controlsIndex_;
            if (active) fill(renderer_, {58, y - 5, 524, 26}, palette.accent);
            const SDL_Color nameColor = active ? palette.background : palette.text;
            const SDL_Color valueColor = active ? palette.background : palette.highlight;
            if (index == ActionRows) {
                drawTextFit(renderer_, 70, y, tr(Str::CReset), 2, 500, nameColor);
            } else {
                const Action action = static_cast<Action>(index);
                drawTextFit(renderer_, 70, y, actionName(action), 2, 300, nameColor);
                drawTextRight(renderer_, 572, y, input_.label(action), 2, 200, valueColor);
            }
        }
        if (top > 0) drawArrow(570, 84, true, palette.highlight);
        if (top + SettingsVisibleRows < Rows) drawArrow(570, 372, false, palette.highlight);
        drawTextFit(renderer_, 66, 392, tr(Str::CFoot1), 1, 510, palette.text);
        drawTextFit(renderer_, 66, 408, tr(Str::CFoot2), 1, 510, palette.text);
        drawTextFit(renderer_, 66, 424, input_.label(Action::Back) + ": " + tr(Str::FBack), 1, 510, palette.highlight);
        if (captureAction_ >= 0) {
            fill(renderer_, {100, 150, 440, 170}, palette.background);
            outline(renderer_, {100, 150, 440, 170}, palette.highlight, 3);
            drawTextFit(renderer_, 120, 172, tr(Str::CPress), 2, 400, palette.text);
            drawTextFit(renderer_, 120, 204, actionName(static_cast<Action>(captureAction_)), 3, 400, palette.highlight);
            const Uint32 now = SDL_GetTicks();
            const int seconds = now >= captureDeadline_ ? 0 : static_cast<int>((captureDeadline_ - now + 999U) / 1000U);
            drawTextFit(renderer_, 120, 266, std::string(tr(Str::CWait)) + " " + std::to_string(seconds), 2, 400, palette.text);
            const int barWidth = 400 * seconds / (CaptureTimeoutMs / 1000);
            fill(renderer_, {120, 292, std::max(0, barWidth), 8}, palette.accent);
        }
    }

    void drawHelp(const Theme& palette) {
        fill(renderer_, {48, 70, 544, 330}, darken(palette.panel, 4));
        outline(renderer_, {48, 70, 544, 330}, palette.highlight, 3);
        drawTextFit(renderer_, 74, 88, tr(Str::SControls), 4, 480, palette.text);
        const std::array<std::pair<std::string, std::string>, 8> lines{{
            {directionLabel(), tr(Str::HelpCursor)},
            {input_.label(Action::Confirm), tr(Str::AConfirm)},
            {input_.label(Action::Back), tr(Str::ABack)},
            {input_.label(Action::ToggleView), tr(Str::AView)},
            {input_.label(Action::Settings), tr(Str::HintSettings)},
            {input_.label(Action::NewGame), tr(Str::HintNew)},
            {input_.label(Action::CamLeft) + " " + input_.label(Action::CamRight), tr(Str::HintCamera)},
            {input_.label(Action::Quit), tr(Str::AQuit)},
        }};
        for (std::size_t index = 0; index < lines.size(); ++index) {
            const int y = 134 + static_cast<int>(index) * 25;
            drawTextFit(renderer_, 74, y, lines[index].first, 2, 170, palette.highlight);
            drawTextFit(renderer_, 256, y, lines[index].second, 2, 320, palette.text);
        }
        drawTextFit(renderer_, 74, 350, input_.label(Action::Help) + ": " + tr(Str::HelpClose), 2, 480, palette.highlight);
    }
};

// ---------------------------------------------------------------------------
// Gamepad / joystick -> numbered buttons (see padid in input_map.hpp). What a button does is decided
// by the InputMap, so every button can be reassigned in Settings -> Controls.
// ---------------------------------------------------------------------------
class GamepadInput {
public:
    using Sink = std::function<void(int id, bool down, bool repeat)>;

    GamepadInput(const InputMap& map, Sink sink) : map_(map), sink_(std::move(sink)) {
        debug_ = std::filesystem::exists("saves/input_debug");
    }

    ~GamepadInput() {
        for (auto* c : controllers_) SDL_GameControllerClose(c);
        for (auto* j : joysticks_) SDL_JoystickClose(j);
    }

    void openAll() {
        for (int i = 0; i < SDL_NumJoysticks(); ++i) openDevice(i);
    }

    void handle(const SDL_Event& e) {
        switch (e.type) {
            case SDL_CONTROLLERDEVICEADDED: openDevice(e.cdevice.which); break;
            case SDL_JOYDEVICEADDED: openDevice(e.jdevice.which); break;
            case SDL_CONTROLLERBUTTONDOWN:
            case SDL_CONTROLLERBUTTONUP:
                press(chessforge::padid::controller(e.cbutton.button), e.type == SDL_CONTROLLERBUTTONDOWN);
                break;
            case SDL_CONTROLLERAXISMOTION: axis(e.caxis.axis, e.caxis.value); break;
            case SDL_JOYBUTTONDOWN:
            case SDL_JOYBUTTONUP:
                if (!isController(e.jbutton.which)) press(chessforge::padid::joystick(e.jbutton.button), e.type == SDL_JOYBUTTONDOWN);
                break;
            case SDL_JOYHATMOTION:
                if (!isController(e.jhat.which)) hat(e.jhat.value);
                break;
            case SDL_JOYAXISMOTION:
                if (!isController(e.jaxis.which)) axis(e.jaxis.axis, e.jaxis.value);
                break;
            default: break;
        }
    }

    // Call once per frame: key repeat for a held direction.
    void update() {
        if (repeatId_ < 0) return;
        const Uint32 now = SDL_GetTicks();
        if (now >= nextRepeat_) {
            sink_(repeatId_, true, true);
            nextRepeat_ = now + 90;
        }
    }

private:
    const InputMap& map_;
    Sink sink_;
    std::vector<SDL_GameController*> controllers_;
    std::vector<SDL_Joystick*> joysticks_;
    std::array<bool, 32> axisState_{};  // [axis * 2 + positive]
    Uint8 hatState_ = 0;
    int repeatId_ = -1;
    Uint32 nextRepeat_ = 0;
    bool debug_ = false;

    void log(const std::string& line) const {
        if (!debug_) return;
        std::filesystem::create_directories("saves");
        std::ofstream("saves/input.log", std::ios::app) << line << '\n';
    }

    bool isController(SDL_JoystickID id) const { return SDL_GameControllerFromInstanceID(id) != nullptr; }

    void openDevice(int index) {
        if (SDL_IsGameController(index)) {
            if (SDL_GameController* c = SDL_GameControllerOpen(index)) {
                controllers_.push_back(c);
                log(std::string("controller: ") + SDL_GameControllerName(c));
            }
        } else if (SDL_Joystick* j = SDL_JoystickOpen(index)) {
            joysticks_.push_back(j);
            log(std::string("joystick: ") + SDL_JoystickName(j));
        }
    }

    void press(int id, bool down) {
        log("button id " + std::to_string(id) + (down ? " down" : " up"));
        sink_(id, down, false);
        if (down) {
            const auto action = map_.lookup(id);
            if (action && InputMap::isDirection(*action)) {
                repeatId_ = id;
                nextRepeat_ = SDL_GetTicks() + 350;
            }
        } else if (id == repeatId_) {
            repeatId_ = -1;
        }
    }

    void axis(Uint8 index, Sint16 value) {
        constexpr Sint16 Dead = 16000;
        if (index >= 16) return;
        const std::size_t slot = static_cast<std::size_t>(index) * 2U;
        const bool negative = value < -Dead;
        const bool positive = value > Dead;
        if (negative != axisState_[slot]) {
            axisState_[slot] = negative;
            press(chessforge::padid::axis(index, false), negative);
        }
        if (positive != axisState_[slot + 1]) {
            axisState_[slot + 1] = positive;
            press(chessforge::padid::axis(index, true), positive);
        }
    }

    void hat(Uint8 value) {
        struct Direction { Uint8 mask; int offset; };
        constexpr std::array<Direction, 4> directions{{{SDL_HAT_UP, 0}, {SDL_HAT_DOWN, 1}, {SDL_HAT_LEFT, 2}, {SDL_HAT_RIGHT, 3}}};
        for (const Direction& direction : directions) {
            const bool now = (value & direction.mask) != 0;
            const bool before = (hatState_ & direction.mask) != 0;
            if (now != before) press(chessforge::padid::Hat + direction.offset, now);
        }
        hatState_ = value;
    }
};

}  // namespace

int main(int, char**) {
    SDL_SetHint(SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS, "1");
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS | SDL_INIT_JOYSTICK | SDL_INIT_GAMECONTROLLER) != 0) return 1;
    SDL_Window* window = SDL_CreateWindow("ChessForge 3D", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, ScreenWidth, ScreenHeight,
                                          SDL_WINDOW_SHOWN | SDL_WINDOW_FULLSCREEN_DESKTOP);
    if (window == nullptr) { SDL_Quit(); return 1; }
    SDL_ShowCursor(SDL_DISABLE);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (renderer == nullptr) renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
    if (renderer == nullptr) { SDL_DestroyWindow(window); SDL_Quit(); return 1; }
    SDL_RenderSetLogicalSize(renderer, ScreenWidth, ScreenHeight);

    {
        // Button layout of the device (saves/input.cfg) and the player's own assignments (saves/controls.cfg).
        chessforge::PadLayout layout;
        layout.loadFile("saves/input.cfg");
        InputMap input(layout);
        input.load("saves/controls.cfg");

        ChessApp app(renderer, input);
        GamepadInput pad(input, [&app](int id, bool down, bool repeat) { app.onPad(id, down, repeat); });
        pad.openAll();
        while (app.running()) {
            SDL_Event event;
            while (SDL_PollEvent(&event) != 0) {
                pad.handle(event);
                app.handleEvent(event);
            }
            pad.update();
            app.tick();
            app.draw();
            SDL_RenderPresent(renderer);
            SDL_Delay(16);
        }
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
