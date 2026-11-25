#pragma once

#include "chess.hpp"

#include <filesystem>
#include <optional>
#include <string>

namespace chessforge {

class ComputerPlayer {
public:
    explicit ComputerPlayer(std::filesystem::path stockfishPath);

    [[nodiscard]] std::optional<Move> chooseMove(const Board& board, int elo, int milliseconds) const;
    [[nodiscard]] bool hasStockfish() const;

private:
    std::filesystem::path stockfishPath_;

    [[nodiscard]] std::optional<Move> askStockfish(const Board& board, int elo, int milliseconds) const;
    [[nodiscard]] std::optional<Move> fallbackMove(const Board& board, int elo) const;
};

}  // namespace chessforge
