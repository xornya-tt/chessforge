#include "engine.hpp"

#include <algorithm>
#include <array>
#include <chrono>
#include <functional>
#include <limits>
#include <numeric>
#include <sstream>
#include <utility>
#include <vector>

#ifdef __linux__
#include <cerrno>
#include <csignal>
#include <cstring>
#include <poll.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#endif

namespace chessforge {
namespace {

int evaluate(const Board& board) {
    int score = 0;
    for (int square = 0; square < 64; ++square) {
        const Piece piece = board.at(square);
        if (piece.empty()) continue;
        const int value = materialValue(piece.type);
        score += piece.color == Color::White ? value : -value;
    }
    const int mobility = static_cast<int>(board.legalMoves().size()) * (board.sideToMove() == Color::White ? 2 : -2);
    return score + mobility;
}

int minimax(const Board& board, int depth, int alpha, int beta) {
    const GameState state = board.gameState();
    if (state == GameState::WhiteWon) return 100000 + depth;
    if (state == GameState::BlackWon) return -100000 - depth;
    if (state != GameState::Playing || depth == 0) return evaluate(board);

    const auto moves = board.legalMoves();
    if (board.sideToMove() == Color::White) {
        int best = std::numeric_limits<int>::min();
        for (const Move& move : moves) {
            Board next = board;
            next.makeMove(move);
            best = std::max(best, minimax(next, depth - 1, alpha, beta));
            alpha = std::max(alpha, best);
            if (beta <= alpha) break;
        }
        return best;
    }

    int best = std::numeric_limits<int>::max();
    for (const Move& move : moves) {
        Board next = board;
        next.makeMove(move);
        best = std::min(best, minimax(next, depth - 1, alpha, beta));
        beta = std::min(beta, best);
        if (beta <= alpha) break;
    }
    return best;
}

PieceType promotionFromUci(char value) {
    switch (value) {
        case 'q': return PieceType::Queen;
        case 'r': return PieceType::Rook;
        case 'b': return PieceType::Bishop;
        case 'n': return PieceType::Knight;
        default: return PieceType::Queen;
    }
}

#ifdef __linux__
bool writeAll(int fd, const std::string& value) {
    const char* data = value.data();
    std::size_t left = value.size();
    while (left > 0) {
        const ssize_t written = write(fd, data, left);
        if (written <= 0) return false;
        data += written;
        left -= static_cast<std::size_t>(written);
    }
    return true;
}

std::optional<std::string> readLine(int fd, int timeoutMs) {
    std::string line;
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeoutMs);
    while (std::chrono::steady_clock::now() < deadline) {
        const auto remaining = std::chrono::duration_cast<std::chrono::milliseconds>(deadline - std::chrono::steady_clock::now()).count();
        pollfd descriptor{fd, POLLIN, 0};
        const int result = poll(&descriptor, 1, static_cast<int>(std::max<long long>(1, remaining)));
        if (result <= 0 || (descriptor.revents & (POLLERR | POLLHUP | POLLNVAL))) return std::nullopt;
        char value = '\0';
        const ssize_t received = read(fd, &value, 1);
        if (received != 1) return std::nullopt;
        if (value == '\n') return line;
        if (value != '\r') line += value;
    }
    return std::nullopt;
}
#endif

}  // namespace

ComputerPlayer::ComputerPlayer(std::filesystem::path stockfishPath) : stockfishPath_(std::move(stockfishPath)) {}

bool ComputerPlayer::hasStockfish() const {
    std::error_code error;
    return std::filesystem::is_regular_file(stockfishPath_, error);
}

std::optional<Move> ComputerPlayer::chooseMove(const Board& board, int elo, int milliseconds) const {
    if (hasStockfish()) {
        if (const auto engineMove = askStockfish(board, elo, milliseconds)) return engineMove;
    }
    return fallbackMove(board, elo);
}

std::optional<Move> ComputerPlayer::askStockfish(const Board& board, int elo, int milliseconds) const {
#ifdef __linux__
    int inputPipe[2]{};
    int outputPipe[2]{};
    if (pipe(inputPipe) != 0 || pipe(outputPipe) != 0) return std::nullopt;

    const pid_t child = fork();
    if (child == 0) {
        dup2(inputPipe[0], STDIN_FILENO);
        dup2(outputPipe[1], STDOUT_FILENO);
        dup2(outputPipe[1], STDERR_FILENO);
        close(inputPipe[0]);
        close(inputPipe[1]);
        close(outputPipe[0]);
        close(outputPipe[1]);
        const std::string binary = stockfishPath_.string();
        execl(binary.c_str(), "stockfish", static_cast<char*>(nullptr));
        _exit(127);
    }
    close(inputPipe[0]);
    close(outputPipe[1]);
    if (child < 0) {
        close(inputPipe[1]);
        close(outputPipe[0]);
        return std::nullopt;
    }

    const auto cleanup = [&]() {
        writeAll(inputPipe[1], "quit\n");
        close(inputPipe[1]);
        close(outputPipe[0]);
        int status = 0;
        if (waitpid(child, &status, WNOHANG) == 0) {
            kill(child, SIGTERM);
            waitpid(child, &status, 0);
        }
    };

    bool accepted = writeAll(inputPipe[1], "uci\n");
    std::string line;
    while (accepted) {
        const auto received = readLine(outputPipe[0], 1500);
        if (!received) { accepted = false; break; }
        if (*received == "uciok") break;
    }
    if (accepted) {
        const int stockfishElo = std::clamp(elo, 1320, 3190);
        accepted = writeAll(inputPipe[1], "setoption name UCI_LimitStrength value true\n") &&
                   writeAll(inputPipe[1], "setoption name UCI_Elo value " + std::to_string(stockfishElo) + "\n") &&
                   writeAll(inputPipe[1], "isready\n");
    }
    while (accepted) {
        const auto received = readLine(outputPipe[0], 1500);
        if (!received) { accepted = false; break; }
        if (*received == "readyok") break;
    }
    if (accepted) {
        accepted = writeAll(inputPipe[1], "position fen " + board.uciPosition() + "\n") &&
                   writeAll(inputPipe[1], "go movetime " + std::to_string(std::clamp(milliseconds, 80, 5000)) + "\n");
    }
    std::optional<Move> result;
    while (accepted) {
        const auto received = readLine(outputPipe[0], std::clamp(milliseconds + 1200, 1500, 7000));
        if (!received) break;
        if (received->rfind("bestmove ", 0) != 0) continue;
        std::istringstream parser(*received);
        std::string tag;
        std::string uci;
        parser >> tag >> uci;
        if (uci.size() >= 4 && uci != "(none)") {
            const int from = Board::fromAlgebraic(uci.substr(0, 2));
            const int to = Board::fromAlgebraic(uci.substr(2, 2));
            const PieceType promotion = uci.size() >= 5 ? promotionFromUci(uci[4]) : PieceType::Queen;
            result = board.findLegalMove(from, to, promotion);
        }
        break;
    }
    cleanup();
    return result;
#else
    (void)board;
    (void)elo;
    (void)milliseconds;
    return std::nullopt;
#endif
}

std::optional<Move> ComputerPlayer::fallbackMove(const Board& board, int elo) const {
    const auto moves = board.legalMoves();
    if (moves.empty()) return std::nullopt;
    const int depth = elo < 900 ? 1 : (elo < 1650 ? 2 : 3);
    struct Candidate { Move move; int score; };
    std::vector<Candidate> ranked;
    ranked.reserve(moves.size());
    for (const Move& move : moves) {
        Board next = board;
        next.makeMove(move);
        ranked.push_back({move, minimax(next, depth - 1, std::numeric_limits<int>::min() + 1, std::numeric_limits<int>::max() - 1)});
    }
    const bool white = board.sideToMove() == Color::White;
    std::sort(ranked.begin(), ranked.end(), [white](const Candidate& left, const Candidate& right) {
        return white ? left.score > right.score : left.score < right.score;
    });

    const std::size_t choices = elo < 550 ? std::min<std::size_t>(ranked.size(), 8) :
                                elo < 900 ? std::min<std::size_t>(ranked.size(), 4) :
                                elo < 1300 ? std::min<std::size_t>(ranked.size(), 2) : 1;
    const std::size_t pick = choices == 1 ? 0 : std::hash<std::string>{}(board.fen()) % choices;
    return ranked[pick].move;
}

}  // namespace chessforge
