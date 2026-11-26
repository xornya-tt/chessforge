#include "chess.hpp"
#include "engine.hpp"

#include <SDL2/SDL.h>

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <exception>
#include <filesystem>
#include <fstream>
#include <functional>
#include <optional>
#include <sstream>
#include <utility>
#include <string>
#include <system_error>
#include <vector>

namespace {

constexpr int ScreenWidth = 640;
constexpr int ScreenHeight = 480;
constexpr int BoardLeft = 16;
constexpr int BoardTop = 36;
constexpr int SquareSize = 52;
constexpr int BoardPixels = SquareSize * 8;
constexpr float Pi = 3.14159265358979F;

using chessforge::Board;
using chessforge::Color;
using chessforge::ComputerPlayer;
using chessforge::GameState;
using chessforge::Move;
using chessforge::Piece;
using chessforge::PieceType;

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

struct Settings {
    int theme = 0;
    int texture = 1;
    bool threeDimensional = true;
    int elo = 1200;
    bool playerWhite = true;
    float yaw = 0.0F;
    float pitch = 0.85F;
    std::string pieceSet;  // empty = built-in vector pieces, otherwise folder name inside pieces/
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
                if (key == "piece_set") { pieceSet = value; while (!pieceSet.empty() && (pieceSet.back() == '\r' || pieceSet.back() == ' ')) pieceSet.pop_back(); }
                if (key == "theme") theme = std::clamp(std::stoi(value), 0, 4);
                if (key == "texture") texture = std::clamp(std::stoi(value), 0, 4);
                if (key == "three_dimensional") threeDimensional = value == "1";
                if (key == "elo") elo = std::clamp(std::stoi(value), 400, 3200);
                if (key == "player_white") playerWhite = value == "1";
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

std::array<unsigned char, 7> fontRows(char raw) {
    const char c = static_cast<char>(std::toupper(static_cast<unsigned char>(raw)));
    switch (c) {
        case 'A': return {14,17,17,31,17,17,17}; case 'B': return {30,17,17,30,17,17,30};
        case 'C': return {15,16,16,16,16,16,15}; case 'D': return {30,17,17,17,17,17,30};
        case 'E': return {31,16,16,30,16,16,31}; case 'F': return {31,16,16,30,16,16,16};
        case 'G': return {15,16,16,23,17,17,14}; case 'H': return {17,17,17,31,17,17,17};
        case 'I': return {31,4,4,4,4,4,31}; case 'J': return {7,2,2,2,18,18,12};
        case 'K': return {17,18,20,24,20,18,17}; case 'L': return {16,16,16,16,16,16,31};
        case 'M': return {17,27,21,17,17,17,17}; case 'N': return {17,25,21,19,17,17,17};
        case 'O': return {14,17,17,17,17,17,14}; case 'P': return {30,17,17,30,16,16,16};
        case 'Q': return {14,17,17,17,21,18,13}; case 'R': return {30,17,17,30,20,18,17};
        case 'S': return {15,16,16,14,1,1,30}; case 'T': return {31,4,4,4,4,4,4};
        case 'U': return {17,17,17,17,17,17,14}; case 'V': return {17,17,17,17,17,10,4};
        case 'W': return {17,17,17,17,21,27,17}; case 'X': return {17,17,10,4,10,17,17};
        case 'Y': return {17,17,10,4,4,4,4}; case 'Z': return {31,1,2,4,8,16,31};
        case '0': return {14,17,19,21,25,17,14}; case '1': return {4,12,4,4,4,4,14};
        case '2': return {14,17,1,2,4,8,31}; case '3': return {30,1,1,14,1,1,30};
        case '4': return {2,6,10,18,31,2,2}; case '5': return {31,16,16,30,1,1,30};
        case '6': return {14,16,16,30,17,17,14}; case '7': return {31,1,2,4,8,8,8};
        case '8': return {14,17,17,14,17,17,14}; case '9': return {14,17,17,15,1,1,14};
        case ':': return {0,4,4,0,4,4,0}; case '-': return {0,0,0,31,0,0,0};
        case '/': return {1,2,2,4,8,8,16}; case '.': return {0,0,0,0,0,6,6};
        case '+': return {0,4,4,31,4,4,0}; case ' ': return {0,0,0,0,0,0,0};
        default: return {31,17,2,4,8,0,8};
    }
}

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

void drawText(SDL_Renderer* renderer, int x, int y, const std::string& text, int scale, SDL_Color color) {
    setColor(renderer, color);
    int pen = x;
    for (const char character : text) {
        const auto rows = fontRows(character);
        for (int row = 0; row < 7; ++row) {
            for (int column = 0; column < 5; ++column) {
                if ((rows[static_cast<std::size_t>(row)] & (1 << (4 - column))) == 0) continue;
                SDL_Rect pixel{pen + column * scale, y + row * scale, scale, scale};
                SDL_RenderFillRect(renderer, &pixel);
            }
        }
        pen += 6 * scale;
    }
}

int textWidth(const std::string& text, int scale) {
    return static_cast<int>(text.size()) * 6 * scale;
}

SDL_Color darken(SDL_Color value, int amount) {
    return {static_cast<Uint8>(std::clamp(static_cast<int>(value.r) - amount, 0, 255)), static_cast<Uint8>(std::clamp(static_cast<int>(value.g) - amount, 0, 255)), static_cast<Uint8>(std::clamp(static_cast<int>(value.b) - amount, 0, 255)), value.a};
}

SDL_Color brighten(SDL_Color value, int amount) {
    return {static_cast<Uint8>(std::clamp(static_cast<int>(value.r) + amount, 0, 255)), static_cast<Uint8>(std::clamp(static_cast<int>(value.g) + amount, 0, 255)), static_cast<Uint8>(std::clamp(static_cast<int>(value.b) + amount, 0, 255)), value.a};
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
    ~ChessApp() { destroyPieceTextures(); }
    explicit ChessApp(SDL_Renderer* renderer)
        : renderer_(renderer), computer_(std::filesystem::path("bin") / "stockfish") {
        settings_.load();
        loadAutosave();
        cursor_ = settings_.playerWhite ? Board::square(4, 1) : Board::square(4, 6);
        if (board_.sideToMove() != playerColor()) makeComputerMove();
    }

    void handleEvent(const SDL_Event& event) {
        if (event.type == SDL_QUIT) running_ = false;
        if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT && !settingsOpen_) handlePointer(event.button.x, event.button.y);
        if (event.type != SDL_KEYDOWN || event.key.repeat != 0) return;
        const SDL_Keycode key = event.key.keysym.sym;
        if (key == SDLK_ESCAPE) { selected_ = -1; helpOpen_ = false; colorEdit_ = -1; return; }
        if (key == SDLK_TAB || key == SDLK_x) { settings_.threeDimensional = !settings_.threeDimensional; settings_.save(); return; }
        if (key == SDLK_s || key == SDLK_y) { settingsOpen_ = !settingsOpen_; helpOpen_ = false; colorEdit_ = -1; settings_.save(); return; }
        if (key == SDLK_h || key == SDLK_F1) { helpOpen_ = !helpOpen_; settingsOpen_ = false; colorEdit_ = -1; return; }
        if (key == SDLK_n || (key == SDLK_RETURN && gameOver())) { newGame(); return; }
        if (settingsOpen_) { handleSettingsKey(key); return; }
        if (helpOpen_) { if (key == SDLK_b || key == SDLK_BACKSPACE) helpOpen_ = false; return; }
        if (key == SDLK_q) { settings_.yaw -= 0.10F; settings_.save(); return; }
        if (key == SDLK_e) { settings_.yaw += 0.10F; settings_.save(); return; }
        if (key == SDLK_u || key == SDLK_BACKSPACE) { undoTurn(); return; }
        if (key == SDLK_b) { if (selected_ >= 0) selected_ = -1; else undoTurn(); return; }
        if (key == SDLK_LEFT) moveCursor(-1, 0);
        else if (key == SDLK_RIGHT) moveCursor(1, 0);
        else if (key == SDLK_UP) moveCursor(0, -1);
        else if (key == SDLK_DOWN) moveCursor(0, 1);
        else if (key == SDLK_SPACE || key == SDLK_a) activateCursor();
    }

    bool running() const { return running_; }

    void draw() {
        const Theme palette = currentTheme();
        ensurePieceTextures(palette);
        fill(renderer_, {0, 0, ScreenWidth, ScreenHeight}, palette.background);
        if (settings_.threeDimensional) drawBoard3D(palette);
        else drawBoard2D(palette);
        drawSidebar(palette);
        if (settingsOpen_) drawSettings(palette);
        if (helpOpen_) drawHelp(palette);
    }

private:
    SDL_Renderer* renderer_;
    Settings settings_;
    Board board_;
    ComputerPlayer computer_;
    std::vector<Board> history_;
    int cursor_ = Board::square(4, 1);
    int selected_ = -1;
    bool settingsOpen_ = false;
    bool helpOpen_ = false;
    int settingIndex_ = 0;
    int colorEdit_ = -1;
    int colorChannel_ = 0;
    bool running_ = true;
    std::string notice_;
    std::array<SDL_Texture*, 12> pieceTextures_{};
    bool piecesBuilt_ = false;
    std::string builtSet_;
    SDL_Color builtWhite_{};
    SDL_Color builtBlack_{};
    float camScale_ = 100.0F;
    float camCx_ = 226.0F;
    float camCy_ = 251.0F;
    static constexpr float CameraDistance = 13.0F;

    [[nodiscard]] Color playerColor() const { return settings_.playerWhite ? Color::White : Color::Black; }
    [[nodiscard]] bool gameOver() const { return board_.gameState() != GameState::Playing; }
    [[nodiscard]] bool flipped() const { return !settings_.playerWhite; }
    [[nodiscard]] Theme currentTheme() const {
        if (settings_.theme < 4) return Themes[static_cast<std::size_t>(settings_.theme)];
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
        if (std::getline(input, stored) && board_.loadFen(stored)) notice_ = "GAME RESTORED";
    }

    void newGame() {
        board_.reset();
        history_.clear();
        selected_ = -1;
        notice_ = "NEW GAME";
        cursor_ = settings_.playerWhite ? Board::square(4, 1) : Board::square(4, 6);
        saveAutosave();
        if (board_.sideToMove() != playerColor()) makeComputerMove();
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
        if (gameOver() || board_.sideToMove() != playerColor()) return;
        const Piece piece = board_.at(cursor_);
        if (selected_ < 0) {
            if (!piece.empty() && piece.color == playerColor()) {
                selected_ = cursor_;
                notice_ = "SELECT DESTINATION";
            }
            return;
        }
        if (cursor_ == selected_) { selected_ = -1; return; }
        if (const auto move = board_.findLegalMove(selected_, cursor_)) {
            history_.push_back(board_);
            board_.makeMove(*move);
            selected_ = -1;
            saveAutosave();
            if (!gameOver()) makeComputerMove();
            return;
        }
        if (!piece.empty() && piece.color == playerColor()) {
            selected_ = cursor_;
            notice_ = "PIECE CHANGED";
        } else {
            notice_ = "ILLEGAL MOVE";
        }
    }

    void makeComputerMove() {
        if (gameOver() || board_.sideToMove() == playerColor()) return;
        notice_ = computer_.hasStockfish() ? "STOCKFISH THINKING" : "BUILTIN BOT THINKING";
        SDL_RenderPresent(renderer_);
        const int thinkingTime = std::clamp(140 + settings_.elo / 3, 200, 1100);
        if (const auto move = computer_.chooseMove(board_, settings_.elo, thinkingTime)) {
            history_.push_back(board_);
            board_.makeMove(*move);
            notice_ = computer_.hasStockfish() ? "STOCKFISH MOVED" : "BUILTIN BOT MOVED";
            saveAutosave();
        }
    }

    void undoTurn() {
        if (history_.empty()) return;
        const bool afterComputer = board_.sideToMove() == playerColor();
        const std::size_t steps = afterComputer && history_.size() >= 2 ? 2 : 1;
        board_ = history_[history_.size() - steps];
        history_.resize(history_.size() - steps);
        selected_ = -1;
        notice_ = "TURN UNDONE";
        saveAutosave();
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

    void handleSettingsKey(SDL_Keycode key) {
        if (colorEdit_ >= 0) {
            SDL_Color* target = colorEdit_ == 0 ? &settings_.customLight : colorEdit_ == 1 ? &settings_.customDark : colorEdit_ == 2 ? &settings_.customAccent : &settings_.customBackground;
            if (key == SDLK_SPACE || key == SDLK_a || key == SDLK_b || key == SDLK_RETURN) {
                colorEdit_ = -1;
                settings_.save();
                return;
            }
            if (key == SDLK_UP) colorChannel_ = (colorChannel_ + 2) % 3;
            else if (key == SDLK_DOWN) colorChannel_ = (colorChannel_ + 1) % 3;
            else if (key == SDLK_LEFT || key == SDLK_RIGHT) {
                Uint8* channel = colorChannel_ == 0 ? &target->r : colorChannel_ == 1 ? &target->g : &target->b;
                const int delta = key == SDLK_LEFT ? -5 : 5;
                *channel = static_cast<Uint8>(std::clamp(static_cast<int>(*channel) + delta, 0, 255));
                settings_.theme = 4;
                settings_.save();
            }
            return;
        }
        if (key == SDLK_SPACE || key == SDLK_a || key == SDLK_RETURN) {
            if (settingIndex_ >= 6) {
                colorEdit_ = settingIndex_ - 6;
                colorChannel_ = 0;
                return;
            }
            settingsOpen_ = false;
            settings_.save();
            return;
        }
        if (key == SDLK_b) { settingsOpen_ = false; settings_.save(); return; }
        if (key == SDLK_UP) settingIndex_ = (settingIndex_ + 9) % 10;
        else if (key == SDLK_DOWN) settingIndex_ = (settingIndex_ + 1) % 10;
        else if (key == SDLK_LEFT || key == SDLK_RIGHT) {
            const int direction = key == SDLK_LEFT ? -1 : 1;
            if (settingIndex_ == 0) settings_.theme = (settings_.theme + direction + 5) % 5;
            if (settingIndex_ == 1) settings_.texture = (settings_.texture + direction + 5) % 5;
            if (settingIndex_ == 2) settings_.elo = std::clamp(settings_.elo + direction * 100, 400, 3200);
            if (settingIndex_ == 3) settings_.playerWhite = !settings_.playerWhite;
            if (settingIndex_ == 4) settings_.pitch = std::clamp(settings_.pitch + static_cast<float>(direction) * 0.05F, 0.30F, 1.10F);
            if (settingIndex_ == 5) {
                const std::vector<std::string> sets = pieceSets();
                std::size_t current = 0;
                for (std::size_t index = 0; index < sets.size(); ++index) if (sets[index] == settings_.pieceSet) current = index;
                const int count = static_cast<int>(sets.size());
                settings_.pieceSet = sets[static_cast<std::size_t>((static_cast<int>(current) + direction + count) % count)];
            }
            settings_.save();
        }
    }

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

    void drawBoard2D(const Theme& palette) {
        for (int square = 0; square < 64; ++square) {
            const int file = Board::fileOf(square);
            const int rank = Board::rankOf(square);
            const SDL_Color base = ((file + rank) & 1) == 0 ? palette.darkSquare : palette.lightSquare;
            const SDL_Color color = texturedSquareColor(base, file, rank);
            const SDL_Rect cell = squareRect(square);
            fill(renderer_, cell, color);
            drawTexture2D(cell, file, rank, color);
            if (square == selected_) outline(renderer_, cell, palette.accent, 3);
            if (square == cursor_) outline(renderer_, cell, palette.highlight, 2);
            if (legalDestination(square)) {
                fill(renderer_, {cell.x + 21, cell.y + 21, 10, 10}, palette.accent);
            }
        }
        for (int square = 0; square < 64; ++square) {
            const Piece piece = board_.at(square);
            if (!piece.empty()) drawPiece2D(square, piece, palette);
        }
        drawText(renderer_, BoardLeft, 14, "A B C D E F G H", 2, palette.text);
        drawText(renderer_, BoardLeft + BoardPixels + 6, BoardTop + BoardPixels - 9, "1 2 3 4 5 6 7 8", 1, palette.text);
    }

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
        for (const float x : {-4.15F, 4.15F}) {
            for (const float z : {-4.15F, 4.15F}) {
                for (const float y : {0.0F, 1.25F}) {
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

    SDL_Point project(Vec3 point) const {
        const Vec3 c = toCamera(point);
        const float perspective = camScale_ / std::max(3.0F, CameraDistance + c.z);
        return {static_cast<int>(std::lround(camCx_ + c.x * perspective)), static_cast<int>(std::lround(camCy_ - c.y * perspective))};
    }

    [[nodiscard]] bool faceVisible(Vec3 center, Vec3 normal) const {
        const Vec3 origin = toCamera({0.0F, 0.0F, 0.0F});
        const Vec3 rotated = toCamera(normal);
        const Vec3 n{rotated.x - origin.x, rotated.y - origin.y, rotated.z - origin.z};
        const Vec3 c = toCamera(center);
        return n.x * c.x + n.y * c.y + n.z * (c.z + CameraDistance) < 0.0F;
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

    void drawOutline3D(int square, SDL_Color color) {
        const float file = static_cast<float>(Board::fileOf(square)) - 4.0F;
        const float rank = static_cast<float>(Board::rankOf(square)) - 4.0F;
        const std::array<SDL_Point, 4> points{{project({file, 0.05F, rank}), project({file + 1.0F, 0.05F, rank}), project({file + 1.0F, 0.05F, rank + 1.0F}), project({file, 0.05F, rank + 1.0F})}};
        setColor(renderer_, color);
        for (int index = 0; index < 4; ++index) SDL_RenderDrawLine(renderer_, points[index].x, points[index].y, points[(index + 1) % 4].x, points[(index + 1) % 4].y);
    }

    void drawBox(float centerX, float centerZ, float bottom, float height, float width, SDL_Color color) {
        const float half = width / 2.0F;
        const float top = bottom + height;
        const float middle = bottom + height / 2.0F;
        const Vec3 a{centerX - half, bottom, centerZ - half};
        const Vec3 b{centerX + half, bottom, centerZ - half};
        const Vec3 c{centerX + half, bottom, centerZ + half};
        const Vec3 d{centerX - half, bottom, centerZ + half};
        const Vec3 e{centerX - half, top, centerZ - half};
        const Vec3 f{centerX + half, top, centerZ - half};
        const Vec3 g{centerX + half, top, centerZ + half};
        const Vec3 h{centerX - half, top, centerZ + half};
        if (faceVisible({centerX, middle, centerZ - half}, {0.0F, 0.0F, -1.0F})) fillQuad(a, b, f, e, darken(color, 25));
        if (faceVisible({centerX + half, middle, centerZ}, {1.0F, 0.0F, 0.0F})) fillQuad(b, c, g, f, color);
        if (faceVisible({centerX, middle, centerZ + half}, {0.0F, 0.0F, 1.0F})) fillQuad(c, d, h, g, brighten(color, 15));
        if (faceVisible({centerX - half, middle, centerZ}, {-1.0F, 0.0F, 0.0F})) fillQuad(d, a, e, h, darken(color, 45));
        if (faceVisible({centerX, top, centerZ}, {0.0F, 1.0F, 0.0F})) fillQuad(e, f, g, h, brighten(color, 20));
    }

    void drawPiece3D(int square, Piece piece, const Theme& palette) {
        const float x = static_cast<float>(Board::fileOf(square)) - 3.5F;
        const float z = static_cast<float>(Board::rankOf(square)) - 3.5F;
        const SDL_Color color = piece.color == Color::White ? palette.whitePiece : palette.blackPiece;
        drawBox(x, z, 0.03F, 0.10F, 0.72F, darken(color, 15));
        switch (piece.type) {
            case PieceType::Pawn:
                drawBox(x, z, 0.13F, 0.40F, 0.38F, color);
                drawBox(x, z, 0.53F, 0.22F, 0.46F, brighten(color, 8));
                break;
            case PieceType::Knight:
                drawBox(x, z, 0.13F, 0.50F, 0.52F, color);
                drawBox(x + 0.10F, z - 0.05F, 0.63F, 0.34F, 0.38F, brighten(color, 6));
                break;
            case PieceType::Bishop:
                drawBox(x, z, 0.13F, 0.55F, 0.48F, color);
                drawBox(x, z, 0.68F, 0.27F, 0.30F, brighten(color, 10));
                break;
            case PieceType::Rook:
                drawBox(x, z, 0.13F, 0.62F, 0.55F, color);
                drawBox(x, z, 0.75F, 0.14F, 0.70F, brighten(color, 3));
                break;
            case PieceType::Queen:
                drawBox(x, z, 0.13F, 0.58F, 0.52F, color);
                drawBox(x, z, 0.71F, 0.20F, 0.38F, brighten(color, 12));
                drawBox(x, z, 0.91F, 0.08F, 0.24F, color);
                break;
            case PieceType::King:
                drawBox(x, z, 0.13F, 0.68F, 0.52F, color);
                drawBox(x, z, 0.81F, 0.18F, 0.34F, brighten(color, 12));
                drawBox(x, z, 0.99F, 0.12F, 0.12F, brighten(color, 25));
                drawBox(x, z, 1.04F, 0.03F, 0.34F, brighten(color, 25));
                break;
            case PieceType::None: break;
        }
    }

    void drawBoard3D(const Theme& palette) {
        updateCamera();
        for (int rank = 0; rank < 8; ++rank) {
            for (int file = 0; file < 8; ++file) {
                const SDL_Color base = ((file + rank) & 1) == 0 ? palette.darkSquare : palette.lightSquare;
                const SDL_Color color = texturedSquareColor(base, file, rank);
                const float x = static_cast<float>(file) - 4.0F;
                const float z = static_cast<float>(rank) - 4.0F;
                fillQuad({x, 0.0F, z}, {x + 1.0F, 0.0F, z}, {x + 1.0F, 0.0F, z + 1.0F}, {x, 0.0F, z + 1.0F}, color);
                drawTexture3D(file, rank, color);
            }
        }
        if (selected_ >= 0) drawOutline3D(selected_, palette.accent);
        drawOutline3D(cursor_, palette.highlight);
        for (int square = 0; square < 64; ++square) if (legalDestination(square)) drawOutline3D(square, palette.accent);

        // Painter's algorithm: farthest pieces first.
        std::vector<std::pair<float, int>> order;
        for (int square = 0; square < 64; ++square) {
            if (board_.at(square).empty()) continue;
            const Vec3 centre{static_cast<float>(Board::fileOf(square)) - 3.5F, 0.3F, static_cast<float>(Board::rankOf(square)) - 3.5F};
            order.emplace_back(toCamera(centre).z, square);
        }
        std::sort(order.begin(), order.end(), [](const auto& left, const auto& right) { return left.first > right.first; });
        for (const auto& entry : order) drawPiece3D(entry.second, board_.at(entry.second), palette);
        drawText(renderer_, 16, 16, "3D BOARD", 2, palette.text);
    }

    std::string stateLabel() const {
        switch (board_.gameState()) {
            case GameState::Playing: return board_.sideToMove() == Color::White ? "WHITE TO MOVE" : "BLACK TO MOVE";
            case GameState::WhiteWon: return "CHECKMATE WHITE WINS";
            case GameState::BlackWon: return "CHECKMATE BLACK WINS";
            case GameState::DrawStalemate: return "DRAW STALEMATE";
            case GameState::DrawFiftyMove: return "DRAW FIFTY MOVE";
        }
        return "";
    }

    void drawSidebar(const Theme& palette) {
        const int x = 450;
        fill(renderer_, {x, 16, 178, 448}, palette.panel);
        outline(renderer_, {x, 16, 178, 448}, palette.accent, 1);
        drawText(renderer_, x + 12, 32, "CHESSFORGE", 3, palette.text);
        drawText(renderer_, x + 54, 58, "3D", 3, palette.accent);
        drawText(renderer_, x + 14, 100, stateLabel(), 2, palette.highlight);
        drawText(renderer_, x + 14, 136, "ELO " + std::to_string(settings_.elo), 3, palette.text);
        drawText(renderer_, x + 14, 166, computer_.hasStockfish() ? "ENGINE STOCKFISH" : "ENGINE BUILTIN", 2, palette.text);
        drawText(renderer_, x + 14, 200, settings_.playerWhite ? "YOU WHITE" : "YOU BLACK", 2, palette.text);
        drawText(renderer_, x + 14, 230, settings_.threeDimensional ? "VIEW 3D" : "VIEW 2D", 2, palette.text);
        drawText(renderer_, x + 14, 280, "A MOVE", 2, palette.text);
        drawText(renderer_, x + 14, 304, "B UNDO", 2, palette.text);
        drawText(renderer_, x + 14, 328, "X 2D 3D", 2, palette.text);
        drawText(renderer_, x + 14, 352, "Y SETTINGS", 2, palette.text);
        drawText(renderer_, x + 14, 376, "START NEW", 2, palette.text);
        drawText(renderer_, x + 14, 400, "Q E CAMERA", 2, palette.text);
        if (!notice_.empty()) drawText(renderer_, x + 14, 434, notice_, 1, palette.highlight);
    }

    void drawSettings(const Theme& palette) {
        fill(renderer_, {50, 42, 540, 402}, darken(palette.panel, 4));
        outline(renderer_, {50, 42, 540, 402}, palette.accent, 3);
        drawText(renderer_, 76, 60, "SETTINGS", 3, palette.text);
        if (colorEdit_ >= 0) {
            const SDL_Color edited = colorEdit_ == 0 ? settings_.customLight : colorEdit_ == 1 ? settings_.customDark : colorEdit_ == 2 ? settings_.customAccent : settings_.customBackground;
            const std::array<std::string, 4> names{{"LIGHT SQUARE", "DARK SQUARE", "ACCENT", "BACKGROUND"}};
            fill(renderer_, {110, 134, 420, 220}, palette.background);
            outline(renderer_, {110, 134, 420, 220}, palette.highlight, 2);
            drawText(renderer_, 138, 158, "EDIT " + names[static_cast<std::size_t>(colorEdit_)], 2, palette.text);
            const std::array<std::string, 3> channels{{"RED", "GREEN", "BLUE"}};
            const std::array<int, 3> values{{edited.r, edited.g, edited.b}};
            for (int index = 0; index < 3; ++index) {
                const int y = 204 + index * 34;
                if (index == colorChannel_) fill(renderer_, {132, y - 4, 240, 25}, palette.accent);
                drawText(renderer_, 146, y, channels[static_cast<std::size_t>(index)], 2, index == colorChannel_ ? palette.background : palette.text);
                drawText(renderer_, 310, y, std::to_string(values[static_cast<std::size_t>(index)]), 2, index == colorChannel_ ? palette.background : palette.highlight);
            }
            fill(renderer_, {405, 202, 88, 88}, edited);
            outline(renderer_, {405, 202, 88, 88}, palette.text, 2);
            drawText(renderer_, 140, 318, "UP DOWN CHANNEL LEFT RIGHT VALUE", 1, palette.text);
            drawText(renderer_, 140, 336, "A SAVES", 1, palette.highlight);
            return;
        }
        const std::array<std::string, 10> labels{{"THEME", "TEXTURE", "BOT ELO", "YOUR SIDE", "CAMERA", "PIECES", "LIGHT RGB", "DARK RGB", "ACCENT RGB", "BACK RGB"}};
        const std::array<std::string, 5> themeNames{{"OAK", "VIOLET", "FOREST", "MONO", "CUSTOM"}};
        const std::array<std::string, 5> textureNames{{"FLAT", "WOOD", "MARBLE", "CARBON", "FELT"}};
        const auto rgb = [](SDL_Color color) { return std::to_string(color.r) + " " + std::to_string(color.g) + " " + std::to_string(color.b); };
        const std::array<std::string, 10> values{{
            themeNames[static_cast<std::size_t>(settings_.theme)], textureNames[static_cast<std::size_t>(settings_.texture)],
            std::to_string(settings_.elo), settings_.playerWhite ? "WHITE" : "BLACK", std::to_string(static_cast<int>(settings_.pitch * 100.0F)),
            settings_.pieceSet.empty() ? std::string("BUILT-IN") : settings_.pieceSet,
            rgb(settings_.customLight), rgb(settings_.customDark), rgb(settings_.customAccent), rgb(settings_.customBackground)
        }};
        for (int index = 0; index < 10; ++index) {
            const int y = 104 + index * 28;
            if (index == settingIndex_) fill(renderer_, {68, y - 4, 504, 24}, palette.accent);
            drawText(renderer_, 82, y, labels[static_cast<std::size_t>(index)], 2, index == settingIndex_ ? palette.background : palette.text);
            const int valueX = 550 - textWidth(values[static_cast<std::size_t>(index)], 2);
            drawText(renderer_, valueX, y, values[static_cast<std::size_t>(index)], 2, index == settingIndex_ ? palette.background : palette.highlight);
        }
        drawText(renderer_, 74, 392, "LEFT RIGHT CHANGE A EDITS RGB", 1, palette.text);
        drawText(renderer_, 74, 410, "PIECE SETS: FOLDER PIECES/NAME", 1, palette.text);
        drawText(renderer_, 74, 428, "B CLOSES AND SAVES", 1, palette.highlight);
    }

    void drawHelp(const Theme& palette) {
        fill(renderer_, {48, 70, 544, 330}, darken(palette.panel, 4));
        outline(renderer_, {48, 70, 544, 330}, palette.highlight, 3);
        drawText(renderer_, 74, 94, "CONTROLS", 4, palette.text);
        drawText(renderer_, 74, 148, "DPAD MOVE CURSOR", 2, palette.text);
        drawText(renderer_, 74, 178, "A SELECT OR MOVE", 2, palette.text);
        drawText(renderer_, 74, 208, "B UNDO OR CANCEL", 2, palette.text);
        drawText(renderer_, 74, 238, "X TOGGLE 2D 3D", 2, palette.text);
        drawText(renderer_, 74, 268, "Y SETTINGS  START NEW GAME", 2, palette.text);
        drawText(renderer_, 74, 298, "Q E ROTATE 3D CAMERA", 2, palette.text);
        drawText(renderer_, 74, 348, "H CLOSE HELP", 2, palette.highlight);
    }
};

// ---------------------------------------------------------------------------
// Gamepad -> keyboard bridge. The game logic only understands keys, so every
// controller/joystick event is translated into the same key presses.
// ---------------------------------------------------------------------------
class GamepadInput {
public:
    using KeySink = std::function<void(SDL_Keycode)>;

    explicit GamepadInput(KeySink sink) : sink_(std::move(sink)) {
        loadConfig();
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
            case SDL_CONTROLLERBUTTONUP: controllerButton(e.cbutton.button, e.type == SDL_CONTROLLERBUTTONDOWN); break;
            case SDL_CONTROLLERAXISMOTION: controllerAxis(e.caxis.axis, e.caxis.value); break;
            case SDL_JOYBUTTONDOWN:
            case SDL_JOYBUTTONUP:
                if (!isController(e.jbutton.which)) joyButton(e.jbutton.button, e.type == SDL_JOYBUTTONDOWN);
                break;
            case SDL_JOYHATMOTION:
                if (!isController(e.jhat.which)) hat(e.jhat.value);
                break;
            case SDL_JOYAXISMOTION:
                if (!isController(e.jaxis.which)) joyAxis(e.jaxis.axis, e.jaxis.value);
                break;
            default: break;
        }
    }

    // Call once per frame: key repeat for a held D-pad direction.
    void update() {
        const Uint32 now = SDL_GetTicks();
        for (int d = 0; d < 4; ++d) {
            if (!dir_[d]) continue;
            if (now >= nextRepeat_) {
                sink_(DirKeys[d]);
                nextRepeat_ = now + 90;
            }
            return;
        }
    }

private:
    static constexpr SDL_Keycode DirKeys[4] = {SDLK_UP, SDLK_DOWN, SDLK_LEFT, SDLK_RIGHT};

    KeySink sink_;
    std::vector<SDL_GameController*> controllers_;
    std::vector<SDL_Joystick*> joysticks_;
    bool dir_[4] = {false, false, false, false};
    Uint32 nextRepeat_ = 0;
    bool debug_ = false;
    bool nintendo_ = true;  // physical A/B and X/Y are swapped relative to SDL names
    // Raw joystick button indices (used only when SDL has no controller mapping).
    int btnA_ = 0, btnB_ = 1, btnX_ = 2, btnY_ = 3, btnL_ = 4, btnR_ = 5, btnSelect_ = 8, btnStart_ = 9, btnMenu_ = 10;

    void log(const std::string& line) const {
        if (!debug_) return;
        std::filesystem::create_directories("saves");
        std::ofstream("saves/input.log", std::ios::app) << line << '\n';
    }

    void loadConfig() {
        std::ifstream input("saves/input.cfg");
        std::string key;
        std::string value;
        while (std::getline(input, key, '=') && std::getline(input, value)) {
            try {
                const int v = std::stoi(value);
                if (key == "nintendo_layout") nintendo_ = v != 0;
                else if (key == "btn_a") btnA_ = v;
                else if (key == "btn_b") btnB_ = v;
                else if (key == "btn_x") btnX_ = v;
                else if (key == "btn_y") btnY_ = v;
                else if (key == "btn_l") btnL_ = v;
                else if (key == "btn_r") btnR_ = v;
                else if (key == "btn_select") btnSelect_ = v;
                else if (key == "btn_start") btnStart_ = v;
                else if (key == "btn_menu") btnMenu_ = v;
            } catch (const std::exception&) {
            }
        }
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

    void setDir(int d, bool down) {
        if (down && !dir_[d]) {
            sink_(DirKeys[d]);
            nextRepeat_ = SDL_GetTicks() + 350;
        }
        dir_[d] = down;
    }

    void action(SDL_Keycode key, bool down) {
        if (down) sink_(key);
    }

    void quit() const {
        SDL_Event q{};
        q.type = SDL_QUIT;
        SDL_PushEvent(&q);
    }

    void hat(Uint8 v) {
        setDir(0, (v & SDL_HAT_UP) != 0);
        setDir(1, (v & SDL_HAT_DOWN) != 0);
        setDir(2, (v & SDL_HAT_LEFT) != 0);
        setDir(3, (v & SDL_HAT_RIGHT) != 0);
        log("hat " + std::to_string(v));
    }

    void joyAxis(Uint8 axis, Sint16 value) {
        log("axis " + std::to_string(axis) + " " + std::to_string(value));
        constexpr Sint16 Dead = 16000;
        if (axis == 0) { setDir(2, value < -Dead); setDir(3, value > Dead); }
        if (axis == 1) { setDir(0, value < -Dead); setDir(1, value > Dead); }
    }

    void controllerAxis(Uint8 axis, Sint16 value) {
        if (axis == SDL_CONTROLLER_AXIS_LEFTX) joyAxis(0, value);
        if (axis == SDL_CONTROLLER_AXIS_LEFTY) joyAxis(1, value);
    }

    void controllerButton(Uint8 b, bool down) {
        log("ctrl button " + std::to_string(b) + (down ? " down" : " up"));
        const bool n = nintendo_;
        switch (b) {
            case SDL_CONTROLLER_BUTTON_DPAD_UP: setDir(0, down); break;
            case SDL_CONTROLLER_BUTTON_DPAD_DOWN: setDir(1, down); break;
            case SDL_CONTROLLER_BUTTON_DPAD_LEFT: setDir(2, down); break;
            case SDL_CONTROLLER_BUTTON_DPAD_RIGHT: setDir(3, down); break;
            case SDL_CONTROLLER_BUTTON_A: action(n ? SDLK_b : SDLK_a, down); break;
            case SDL_CONTROLLER_BUTTON_B: action(n ? SDLK_a : SDLK_b, down); break;
            case SDL_CONTROLLER_BUTTON_X: action(n ? SDLK_y : SDLK_x, down); break;
            case SDL_CONTROLLER_BUTTON_Y: action(n ? SDLK_x : SDLK_y, down); break;
            case SDL_CONTROLLER_BUTTON_LEFTSHOULDER: action(SDLK_q, down); break;
            case SDL_CONTROLLER_BUTTON_RIGHTSHOULDER: action(SDLK_e, down); break;
            case SDL_CONTROLLER_BUTTON_BACK: action(SDLK_h, down); break;
            case SDL_CONTROLLER_BUTTON_START: action(SDLK_n, down); break;
            case SDL_CONTROLLER_BUTTON_GUIDE: if (down) quit(); break;
            default: break;
        }
    }

    void joyButton(Uint8 b, bool down) {
        log("joy button " + std::to_string(b) + (down ? " down" : " up"));
        const int i = b;
        if (i == btnA_) action(SDLK_a, down);
        else if (i == btnB_) action(SDLK_b, down);
        else if (i == btnX_) action(SDLK_x, down);
        else if (i == btnY_) action(SDLK_y, down);
        else if (i == btnL_) action(SDLK_q, down);
        else if (i == btnR_) action(SDLK_e, down);
        else if (i == btnSelect_) action(SDLK_h, down);
        else if (i == btnStart_) action(SDLK_n, down);
        else if (i == btnMenu_ && down) quit();
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
        ChessApp app(renderer);
        GamepadInput pad([&app](SDL_Keycode key) {
            SDL_Event e{};
            e.type = SDL_KEYDOWN;
            e.key.state = SDL_PRESSED;
            e.key.repeat = 0;
            e.key.keysym.sym = key;
            app.handleEvent(e);
        });
        pad.openAll();
        while (app.running()) {
            SDL_Event event;
            while (SDL_PollEvent(&event) != 0) {
                pad.handle(event);
                app.handleEvent(event);
            }
            pad.update();
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
