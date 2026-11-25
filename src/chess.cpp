#include "chess.hpp"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <sstream>

namespace chessforge {
namespace {

constexpr bool inside(int file, int rank) {
    return file >= 0 && file < 8 && rank >= 0 && rank < 8;
}

bool sameColor(Piece piece, Color color) {
    return !piece.empty() && piece.color == color;
}

}  // namespace

Color Board::opposite(Color color) {
    return color == Color::White ? Color::Black : Color::White;
}

char pieceToFen(Piece piece) {
    char result = ' ';
    switch (piece.type) {
        case PieceType::Pawn: result = 'p'; break;
        case PieceType::Knight: result = 'n'; break;
        case PieceType::Bishop: result = 'b'; break;
        case PieceType::Rook: result = 'r'; break;
        case PieceType::Queen: result = 'q'; break;
        case PieceType::King: result = 'k'; break;
        case PieceType::None: return ' ';
    }
    return piece.color == Color::White ? static_cast<char>(std::toupper(result)) : result;
}

Piece pieceFromFen(char value) {
    const Color color = std::isupper(static_cast<unsigned char>(value)) ? Color::White : Color::Black;
    switch (static_cast<char>(std::tolower(static_cast<unsigned char>(value)))) {
        case 'p': return {PieceType::Pawn, color};
        case 'n': return {PieceType::Knight, color};
        case 'b': return {PieceType::Bishop, color};
        case 'r': return {PieceType::Rook, color};
        case 'q': return {PieceType::Queen, color};
        case 'k': return {PieceType::King, color};
        default: return {};
    }
}

char pieceGlyph(Piece piece) {
    if (piece.empty()) return ' ';
    return pieceToFen(piece);
}

int materialValue(PieceType type) {
    switch (type) {
        case PieceType::Pawn: return 100;
        case PieceType::Knight: return 320;
        case PieceType::Bishop: return 330;
        case PieceType::Rook: return 500;
        case PieceType::Queen: return 900;
        case PieceType::King: return 20000;
        case PieceType::None: return 0;
    }
    return 0;
}

Board::Board() {
    reset();
}

void Board::reset() {
    squares_.fill({});
    constexpr PieceType backRank[] = {
        PieceType::Rook, PieceType::Knight, PieceType::Bishop, PieceType::Queen,
        PieceType::King, PieceType::Bishop, PieceType::Knight, PieceType::Rook
    };
    for (int file = 0; file < 8; ++file) {
        squares_[square(file, 0)] = {backRank[file], Color::White};
        squares_[square(file, 1)] = {PieceType::Pawn, Color::White};
        squares_[square(file, 6)] = {PieceType::Pawn, Color::Black};
        squares_[square(file, 7)] = {backRank[file], Color::Black};
    }
    side_ = Color::White;
    castling_ = WhiteKingSide | WhiteQueenSide | BlackKingSide | BlackQueenSide;
    enPassant_ = -1;
    halfmove_ = 0;
    fullmove_ = 1;
}

Piece Board::at(int squareIndex) const {
    if (squareIndex < 0 || squareIndex >= 64) return {};
    return squares_[static_cast<std::size_t>(squareIndex)];
}

std::string Board::algebraic(int squareIndex) {
    if (squareIndex < 0 || squareIndex >= 64) return "-";
    std::string result;
    result += static_cast<char>('a' + fileOf(squareIndex));
    result += static_cast<char>('1' + rankOf(squareIndex));
    return result;
}

int Board::fromAlgebraic(const std::string& value) {
    if (value.size() != 2 || value[0] < 'a' || value[0] > 'h' || value[1] < '1' || value[1] > '8') return -1;
    return square(value[0] - 'a', value[1] - '1');
}

bool Board::isSquareAttacked(int target, Color attacker) const {
    const int file = fileOf(target);
    const int rank = rankOf(target);

    const int pawnSourceRank = rank + (attacker == Color::White ? -1 : 1);
    for (const int sourceFile : {file - 1, file + 1}) {
        if (inside(sourceFile, pawnSourceRank)) {
            const Piece piece = at(square(sourceFile, pawnSourceRank));
            if (piece.type == PieceType::Pawn && piece.color == attacker) return true;
        }
    }

    constexpr int knightOffsets[][2] = {{1, 2}, {2, 1}, {2, -1}, {1, -2}, {-1, -2}, {-2, -1}, {-2, 1}, {-1, 2}};
    for (const auto& offset : knightOffsets) {
        const int sourceFile = file + offset[0];
        const int sourceRank = rank + offset[1];
        if (inside(sourceFile, sourceRank)) {
            const Piece piece = at(square(sourceFile, sourceRank));
            if (piece.type == PieceType::Knight && piece.color == attacker) return true;
        }
    }

    constexpr int diagonal[][2] = {{1, 1}, {1, -1}, {-1, -1}, {-1, 1}};
    for (const auto& direction : diagonal) {
        int sourceFile = file + direction[0];
        int sourceRank = rank + direction[1];
        while (inside(sourceFile, sourceRank)) {
            const Piece piece = at(square(sourceFile, sourceRank));
            if (!piece.empty()) {
                if (piece.color == attacker && (piece.type == PieceType::Bishop || piece.type == PieceType::Queen)) return true;
                break;
            }
            sourceFile += direction[0];
            sourceRank += direction[1];
        }
    }

    constexpr int straight[][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};
    for (const auto& direction : straight) {
        int sourceFile = file + direction[0];
        int sourceRank = rank + direction[1];
        while (inside(sourceFile, sourceRank)) {
            const Piece piece = at(square(sourceFile, sourceRank));
            if (!piece.empty()) {
                if (piece.color == attacker && (piece.type == PieceType::Rook || piece.type == PieceType::Queen)) return true;
                break;
            }
            sourceFile += direction[0];
            sourceRank += direction[1];
        }
    }

    for (int fileOffset = -1; fileOffset <= 1; ++fileOffset) {
        for (int rankOffset = -1; rankOffset <= 1; ++rankOffset) {
            if (fileOffset == 0 && rankOffset == 0) continue;
            const int sourceFile = file + fileOffset;
            const int sourceRank = rank + rankOffset;
            if (inside(sourceFile, sourceRank)) {
                const Piece piece = at(square(sourceFile, sourceRank));
                if (piece.type == PieceType::King && piece.color == attacker) return true;
            }
        }
    }
    return false;
}

bool Board::inCheck(Color side) const {
    for (int index = 0; index < 64; ++index) {
        const Piece piece = squares_[static_cast<std::size_t>(index)];
        if (piece.type == PieceType::King && piece.color == side) return isSquareAttacked(index, opposite(side));
    }
    return true;
}

std::vector<Move> Board::pseudoLegalMoves(Color side) const {
    std::vector<Move> moves;
    const auto addMove = [&](int from, int to, PieceType promotion = PieceType::None, bool ep = false, bool castle = false) {
        moves.push_back({from, to, promotion, ep, castle});
    };

    for (int from = 0; from < 64; ++from) {
        const Piece piece = squares_[static_cast<std::size_t>(from)];
        if (piece.color != side) continue;
        const int file = fileOf(from);
        const int rank = rankOf(from);

        if (piece.type == PieceType::Pawn) {
            const int direction = side == Color::White ? 1 : -1;
            const int startRank = side == Color::White ? 1 : 6;
            const int promotionRank = side == Color::White ? 7 : 0;
            const int oneRank = rank + direction;
            if (inside(file, oneRank) && at(square(file, oneRank)).empty()) {
                if (oneRank == promotionRank) {
                    for (const PieceType promotion : {PieceType::Queen, PieceType::Rook, PieceType::Bishop, PieceType::Knight}) addMove(from, square(file, oneRank), promotion);
                } else {
                    addMove(from, square(file, oneRank));
                    const int twoRank = rank + 2 * direction;
                    if (rank == startRank && at(square(file, twoRank)).empty()) addMove(from, square(file, twoRank));
                }
            }
            for (const int targetFile : {file - 1, file + 1}) {
                const int targetRank = rank + direction;
                if (!inside(targetFile, targetRank)) continue;
                const int to = square(targetFile, targetRank);
                const Piece target = at(to);
                if (!target.empty() && target.color != side) {
                    if (targetRank == promotionRank) {
                        for (const PieceType promotion : {PieceType::Queen, PieceType::Rook, PieceType::Bishop, PieceType::Knight}) addMove(from, to, promotion);
                    } else {
                        addMove(from, to);
                    }
                }
                if (to == enPassant_) addMove(from, to, PieceType::None, true);
            }
            continue;
        }

        if (piece.type == PieceType::Knight) {
            constexpr int offsets[][2] = {{1, 2}, {2, 1}, {2, -1}, {1, -2}, {-1, -2}, {-2, -1}, {-2, 1}, {-1, 2}};
            for (const auto& offset : offsets) {
                const int targetFile = file + offset[0];
                const int targetRank = rank + offset[1];
                if (!inside(targetFile, targetRank)) continue;
                const int to = square(targetFile, targetRank);
                if (!sameColor(at(to), side)) addMove(from, to);
            }
            continue;
        }

        if (piece.type == PieceType::Bishop || piece.type == PieceType::Rook || piece.type == PieceType::Queen) {
            constexpr int bishopDirections[][2] = {{1, 1}, {1, -1}, {-1, -1}, {-1, 1}};
            constexpr int rookDirections[][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};
            const bool diagonal = piece.type == PieceType::Bishop || piece.type == PieceType::Queen;
            const bool straight = piece.type == PieceType::Rook || piece.type == PieceType::Queen;
            const auto addRay = [&](const int (&directions)[4][2]) {
                for (const auto& direction : directions) {
                    int targetFile = file + direction[0];
                    int targetRank = rank + direction[1];
                    while (inside(targetFile, targetRank)) {
                        const int to = square(targetFile, targetRank);
                        const Piece target = at(to);
                        if (sameColor(target, side)) break;
                        addMove(from, to);
                        if (!target.empty()) break;
                        targetFile += direction[0];
                        targetRank += direction[1];
                    }
                }
            };
            if (diagonal) addRay(bishopDirections);
            if (straight) addRay(rookDirections);
            continue;
        }

        if (piece.type == PieceType::King) {
            for (int fileOffset = -1; fileOffset <= 1; ++fileOffset) {
                for (int rankOffset = -1; rankOffset <= 1; ++rankOffset) {
                    if (fileOffset == 0 && rankOffset == 0) continue;
                    const int targetFile = file + fileOffset;
                    const int targetRank = rank + rankOffset;
                    if (!inside(targetFile, targetRank)) continue;
                    const int to = square(targetFile, targetRank);
                    if (!sameColor(at(to), side)) addMove(from, to);
                }
            }
            const int homeRank = side == Color::White ? 0 : 7;
            const unsigned char kingSide = side == Color::White ? WhiteKingSide : BlackKingSide;
            const unsigned char queenSide = side == Color::White ? WhiteQueenSide : BlackQueenSide;
            if (rank == homeRank && file == 4 && !inCheck(side)) {
                if ((castling_ & kingSide) && at(square(5, homeRank)).empty() && at(square(6, homeRank)).empty() &&
                    at(square(7, homeRank)).type == PieceType::Rook && at(square(7, homeRank)).color == side &&
                    !isSquareAttacked(square(5, homeRank), opposite(side)) && !isSquareAttacked(square(6, homeRank), opposite(side))) {
                    addMove(from, square(6, homeRank), PieceType::None, false, true);
                }
                if ((castling_ & queenSide) && at(square(1, homeRank)).empty() && at(square(2, homeRank)).empty() && at(square(3, homeRank)).empty() &&
                    at(square(0, homeRank)).type == PieceType::Rook && at(square(0, homeRank)).color == side &&
                    !isSquareAttacked(square(3, homeRank), opposite(side)) && !isSquareAttacked(square(2, homeRank), opposite(side))) {
                    addMove(from, square(2, homeRank), PieceType::None, false, true);
                }
            }
        }
    }
    return moves;
}

Board Board::after(const Move& move) const {
    Board copy = *this;
    copy.applyUnchecked(move);
    return copy;
}

std::vector<Move> Board::legalMoves() const {
    std::vector<Move> legal;
    for (const Move& move : pseudoLegalMoves(side_)) {
        const Board next = after(move);
        if (!next.inCheck(side_)) legal.push_back(move);
    }
    return legal;
}

std::vector<Move> Board::legalMovesFrom(int squareIndex) const {
    std::vector<Move> result;
    for (const Move& move : legalMoves()) {
        if (move.from == squareIndex) result.push_back(move);
    }
    return result;
}

std::optional<Move> Board::findLegalMove(int from, int to, PieceType promotion) const {
    for (const Move& move : legalMoves()) {
        if (!move.matches(from, to)) continue;
        if (move.promotion == PieceType::None || move.promotion == promotion) return move;
    }
    return std::nullopt;
}

void Board::applyUnchecked(const Move& move) {
    Piece moving = squares_[static_cast<std::size_t>(move.from)];
    Piece captured = squares_[static_cast<std::size_t>(move.to)];
    const Color movingSide = moving.color;

    squares_[static_cast<std::size_t>(move.from)] = {};
    if (move.enPassant) {
        const int capturedSquare = move.to + (movingSide == Color::White ? -8 : 8);
        captured = squares_[static_cast<std::size_t>(capturedSquare)];
        squares_[static_cast<std::size_t>(capturedSquare)] = {};
    }
    if (move.promotion != PieceType::None) moving.type = move.promotion;
    squares_[static_cast<std::size_t>(move.to)] = moving;

    if (move.castle) {
        const int rank = movingSide == Color::White ? 0 : 7;
        if (fileOf(move.to) == 6) {
            squares_[static_cast<std::size_t>(square(5, rank))] = squares_[static_cast<std::size_t>(square(7, rank))];
            squares_[static_cast<std::size_t>(square(7, rank))] = {};
        } else {
            squares_[static_cast<std::size_t>(square(3, rank))] = squares_[static_cast<std::size_t>(square(0, rank))];
            squares_[static_cast<std::size_t>(square(0, rank))] = {};
        }
    }

    const auto clearRookRight = [&](int location) {
        if (location == square(0, 0)) castling_ &= static_cast<unsigned char>(~WhiteQueenSide);
        if (location == square(7, 0)) castling_ &= static_cast<unsigned char>(~WhiteKingSide);
        if (location == square(0, 7)) castling_ &= static_cast<unsigned char>(~BlackQueenSide);
        if (location == square(7, 7)) castling_ &= static_cast<unsigned char>(~BlackKingSide);
    };
    if (moving.type == PieceType::King) {
        if (movingSide == Color::White) castling_ &= static_cast<unsigned char>(~(WhiteKingSide | WhiteQueenSide));
        else castling_ &= static_cast<unsigned char>(~(BlackKingSide | BlackQueenSide));
    }
    if (moving.type == PieceType::Rook) clearRookRight(move.from);
    if (captured.type == PieceType::Rook) clearRookRight(move.to);

    enPassant_ = -1;
    if (moving.type == PieceType::Pawn && std::abs(move.to - move.from) == 16) enPassant_ = (move.to + move.from) / 2;
    halfmove_ = (moving.type == PieceType::Pawn || !captured.empty() || move.enPassant) ? 0 : halfmove_ + 1;
    if (movingSide == Color::Black) ++fullmove_;
    side_ = opposite(side_);
}

bool Board::makeMove(const Move& move) {
    const auto legal = legalMoves();
    const auto found = std::find_if(legal.begin(), legal.end(), [&](const Move& candidate) {
        return candidate.from == move.from && candidate.to == move.to && candidate.promotion == move.promotion;
    });
    if (found == legal.end()) return false;
    applyUnchecked(*found);
    return true;
}

GameState Board::gameState() const {
    if (halfmove_ >= 100) return GameState::DrawFiftyMove;
    const auto moves = legalMoves();
    if (!moves.empty()) return GameState::Playing;
    if (inCheck(side_)) return side_ == Color::White ? GameState::BlackWon : GameState::WhiteWon;
    return GameState::DrawStalemate;
}

std::string Board::fen() const {
    std::ostringstream stream;
    for (int rank = 7; rank >= 0; --rank) {
        int empty = 0;
        for (int file = 0; file < 8; ++file) {
            const Piece piece = at(square(file, rank));
            if (piece.empty()) {
                ++empty;
            } else {
                if (empty != 0) stream << empty;
                empty = 0;
                stream << pieceToFen(piece);
            }
        }
        if (empty != 0) stream << empty;
        if (rank != 0) stream << '/';
    }
    stream << (side_ == Color::White ? " w " : " b ");
    if (castling_ == 0) stream << '-';
    else {
        if (castling_ & WhiteKingSide) stream << 'K';
        if (castling_ & WhiteQueenSide) stream << 'Q';
        if (castling_ & BlackKingSide) stream << 'k';
        if (castling_ & BlackQueenSide) stream << 'q';
    }
    stream << ' ' << (enPassant_ < 0 ? "-" : algebraic(enPassant_));
    stream << ' ' << halfmove_ << ' ' << fullmove_;
    return stream.str();
}

bool Board::loadFen(const std::string& fenText) {
    std::istringstream stream(fenText);
    std::string placement;
    std::string active;
    std::string castle;
    std::string ep;
    int halfmove = 0;
    int fullmove = 1;
    if (!(stream >> placement >> active >> castle >> ep >> halfmove >> fullmove)) return false;

    std::array<Piece, 64> parsed{};
    int rank = 7;
    int file = 0;
    for (const char value : placement) {
        if (value == '/') {
            if (file != 8 || rank == 0) return false;
            --rank;
            file = 0;
        } else if (value >= '1' && value <= '8') {
            file += value - '0';
            if (file > 8) return false;
        } else {
            const Piece piece = pieceFromFen(value);
            if (piece.empty() || file >= 8) return false;
            parsed[static_cast<std::size_t>(square(file++, rank))] = piece;
        }
    }
    if (rank != 0 || file != 8 || (active != "w" && active != "b")) return false;

    unsigned char rights = 0;
    if (castle != "-") {
        for (const char value : castle) {
            if (value == 'K') rights |= WhiteKingSide;
            else if (value == 'Q') rights |= WhiteQueenSide;
            else if (value == 'k') rights |= BlackKingSide;
            else if (value == 'q') rights |= BlackQueenSide;
            else return false;
        }
    }
    const int parsedEp = ep == "-" ? -1 : fromAlgebraic(ep);
    if (ep != "-" && parsedEp < 0) return false;

    squares_ = parsed;
    side_ = active == "w" ? Color::White : Color::Black;
    castling_ = rights;
    enPassant_ = parsedEp;
    halfmove_ = std::max(0, halfmove);
    fullmove_ = std::max(1, fullmove);
    return true;
}

}  // namespace chessforge
