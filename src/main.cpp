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
#include <vector>

namespace {

constexpr int ScreenWidth = 640;
constexpr int ScreenHeight = 480;
constexpr int BoardLeft = 16;
constexpr int BoardTop = 36;
constexpr int SquareSize = 52;
constexpr int BoardPixels = SquareSize * 8;

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
    float yaw = 0.62F;
    float pitch = 0.66F;
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

struct Vec3 { float x; float y; float z; };

class ChessApp {
public:
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
        const int fileDirection = flipped() ? -visualX : visualX;
        const int rankDirection = flipped() ? visualY : -visualY;
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
            if (settingIndex_ >= 5) {
                colorEdit_ = settingIndex_ - 5;
                colorChannel_ = 0;
                return;
            }
            settingsOpen_ = false;
            settings_.save();
            return;
        }
        if (key == SDLK_b) { settingsOpen_ = false; settings_.save(); return; }
        if (key == SDLK_UP) settingIndex_ = (settingIndex_ + 8) % 9;
        else if (key == SDLK_DOWN) settingIndex_ = (settingIndex_ + 1) % 9;
        else if (key == SDLK_LEFT || key == SDLK_RIGHT) {
            const int direction = key == SDLK_LEFT ? -1 : 1;
            if (settingIndex_ == 0) settings_.theme = (settings_.theme + direction + 5) % 5;
            if (settingIndex_ == 1) settings_.texture = (settings_.texture + direction + 5) % 5;
            if (settingIndex_ == 2) settings_.elo = std::clamp(settings_.elo + direction * 100, 400, 3200);
            if (settingIndex_ == 3) settings_.playerWhite = !settings_.playerWhite;
            if (settingIndex_ == 4) settings_.pitch = std::clamp(settings_.pitch + static_cast<float>(direction) * 0.05F, 0.30F, 1.10F);
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

    void drawPiece2D(int square, Piece piece, const Theme& palette) {
        const SDL_Rect cell = squareRect(square);
        const std::string label(1, chessforge::pieceGlyph(piece));
        const int scale = 5;
        const SDL_Color shadow = darken(piece.color == Color::White ? palette.whitePiece : palette.blackPiece, 80);
        drawText(renderer_, cell.x + (cell.w - textWidth(label, scale)) / 2 + 2, cell.y + 10 + 2, label, scale, shadow);
        drawText(renderer_, cell.x + (cell.w - textWidth(label, scale)) / 2, cell.y + 10, label, scale, piece.color == Color::White ? palette.whitePiece : palette.blackPiece);
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

    SDL_Point project(Vec3 point) const {
        const float cosineYaw = std::cos(settings_.yaw);
        const float sineYaw = std::sin(settings_.yaw);
        const float x1 = point.x * cosineYaw - point.z * sineYaw;
        const float z1 = point.x * sineYaw + point.z * cosineYaw;
        const float cosinePitch = std::cos(settings_.pitch);
        const float sinePitch = std::sin(settings_.pitch);
        const float y2 = point.y * cosinePitch - z1 * sinePitch;
        const float z2 = point.y * sinePitch + z1 * cosinePitch;
        const float perspective = 405.0F / std::max(3.0F, 11.0F + z2);
        return {static_cast<int>(240.0F + x1 * perspective), static_cast<int>(252.0F - y2 * perspective)};
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
        const Vec3 a{centerX - half, bottom, centerZ - half};
        const Vec3 b{centerX + half, bottom, centerZ - half};
        const Vec3 c{centerX + half, bottom, centerZ + half};
        const Vec3 d{centerX - half, bottom, centerZ + half};
        const Vec3 e{centerX - half, top, centerZ - half};
        const Vec3 f{centerX + half, top, centerZ - half};
        const Vec3 g{centerX + half, top, centerZ + half};
        const Vec3 h{centerX - half, top, centerZ + half};
        fillQuad(a, b, f, e, darken(color, 25));
        fillQuad(b, c, g, f, color);
        fillQuad(c, d, h, g, brighten(color, 15));
        fillQuad(d, a, e, h, darken(color, 45));
        fillQuad(e, f, g, h, brighten(color, 20));
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
        for (int square = 0; square < 64; ++square) {
            const Piece piece = board_.at(square);
            if (!piece.empty()) drawPiece3D(square, piece, palette);
        }
        if (selected_ >= 0) drawOutline3D(selected_, palette.accent);
        drawOutline3D(cursor_, palette.highlight);
        for (int square = 0; square < 64; ++square) if (legalDestination(square)) drawOutline3D(square, palette.accent);
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
        const std::array<std::string, 9> labels{{"THEME", "TEXTURE", "BOT ELO", "YOUR SIDE", "CAMERA", "LIGHT RGB", "DARK RGB", "ACCENT RGB", "BACK RGB"}};
        const std::array<std::string, 5> themeNames{{"OAK", "VIOLET", "FOREST", "MONO", "CUSTOM"}};
        const std::array<std::string, 5> textureNames{{"FLAT", "WOOD", "MARBLE", "CARBON", "FELT"}};
        const auto rgb = [](SDL_Color color) { return std::to_string(color.r) + " " + std::to_string(color.g) + " " + std::to_string(color.b); };
        const std::array<std::string, 9> values{{
            themeNames[static_cast<std::size_t>(settings_.theme)], textureNames[static_cast<std::size_t>(settings_.texture)],
            std::to_string(settings_.elo), settings_.playerWhite ? "WHITE" : "BLACK", std::to_string(static_cast<int>(settings_.pitch * 100.0F)),
            rgb(settings_.customLight), rgb(settings_.customDark), rgb(settings_.customAccent), rgb(settings_.customBackground)
        }};
        for (int index = 0; index < 9; ++index) {
            const int y = 110 + index * 29;
            if (index == settingIndex_) fill(renderer_, {68, y - 4, 504, 24}, palette.accent);
            drawText(renderer_, 82, y, labels[static_cast<std::size_t>(index)], 2, index == settingIndex_ ? palette.background : palette.text);
            const int valueX = 550 - textWidth(values[static_cast<std::size_t>(index)], 2);
            drawText(renderer_, valueX, y, values[static_cast<std::size_t>(index)], 2, index == settingIndex_ ? palette.background : palette.highlight);
        }
        drawText(renderer_, 74, 382, "LEFT RIGHT CHANGE A EDITS RGB", 1, palette.text);
        drawText(renderer_, 74, 402, "B CLOSES AND SAVES", 1, palette.highlight);
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
