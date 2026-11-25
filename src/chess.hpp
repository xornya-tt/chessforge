#pragma once

#include <array>
#include <optional>
#include <string>
#include <vector>

namespace chessforge {

enum class Color : unsigned char { None, White, Black };
enum class PieceType : unsigned char { None, Pawn, Knight, Bishop, Rook, Queen, King };

struct Piece {
    PieceType type = PieceType::None;
    Color color = Color::None;

    [[nodiscard]] bool empty() const { return type == PieceType::None; }
};

struct Move {
    int from = -1;
    int to = -1;
    PieceType promotion = PieceType::None;
    bool enPassant = false;
    bool castle = false;

    [[nodiscard]] bool matches(int source, int destination) const {
        return from == source && to == destination;
    }
};

enum Castling : unsigned char {
    WhiteKingSide = 1 << 0,
    WhiteQueenSide = 1 << 1,
    BlackKingSide = 1 << 2,
    BlackQueenSide = 1 << 3,
};

enum class GameState { Playing, WhiteWon, BlackWon, DrawStalemate, DrawFiftyMove };

class Board {
public:
    Board();

    void reset();
    [[nodiscard]] Piece at(int square) const;
    [[nodiscard]] Color sideToMove() const { return side_; }
    [[nodiscard]] int enPassantSquare() const { return enPassant_; }
    [[nodiscard]] unsigned char castlingRights() const { return castling_; }
    [[nodiscard]] int halfmoveClock() const { return halfmove_; }
    [[nodiscard]] int fullmoveNumber() const { return fullmove_; }

    [[nodiscard]] std::vector<Move> legalMoves() const;
    [[nodiscard]] std::vector<Move> legalMovesFrom(int square) const;
    [[nodiscard]] bool inCheck(Color side) const;
    [[nodiscard]] bool isSquareAttacked(int square, Color attacker) const;
    [[nodiscard]] GameState gameState() const;
    [[nodiscard]] std::optional<Move> findLegalMove(int from, int to, PieceType promotion = PieceType::Queen) const;

    bool makeMove(const Move& move);
    bool loadFen(const std::string& fen);
    [[nodiscard]] std::string fen() const;
    [[nodiscard]] std::string uciPosition() const { return fen(); }

    [[nodiscard]] static int square(int file, int rank) { return rank * 8 + file; }
    [[nodiscard]] static int fileOf(int square) { return square & 7; }
    [[nodiscard]] static int rankOf(int square) { return square >> 3; }
    [[nodiscard]] static Color opposite(Color color);
    [[nodiscard]] static std::string algebraic(int square);
    [[nodiscard]] static int fromAlgebraic(const std::string& value);

private:
    std::array<Piece, 64> squares_{};
    Color side_ = Color::White;
    unsigned char castling_ = WhiteKingSide | WhiteQueenSide | BlackKingSide | BlackQueenSide;
    int enPassant_ = -1;
    int halfmove_ = 0;
    int fullmove_ = 1;

    [[nodiscard]] std::vector<Move> pseudoLegalMoves(Color side) const;
    [[nodiscard]] Board after(const Move& move) const;
    void applyUnchecked(const Move& move);
};

[[nodiscard]] char pieceToFen(Piece piece);
[[nodiscard]] Piece pieceFromFen(char value);
[[nodiscard]] char pieceGlyph(Piece piece);
[[nodiscard]] int materialValue(PieceType type);

}  // namespace chessforge
