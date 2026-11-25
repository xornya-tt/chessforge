#include "chess.hpp"

#include <cassert>
#include <iostream>

using chessforge::Board;
using chessforge::GameState;
using chessforge::PieceType;

int main() {
    Board board;
    assert(board.legalMoves().size() == 20);

    assert(board.loadFen("r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 0 1"));
    assert(board.findLegalMove(Board::fromAlgebraic("e1"), Board::fromAlgebraic("g1")));
    assert(board.findLegalMove(Board::fromAlgebraic("e1"), Board::fromAlgebraic("c1")));

    assert(board.loadFen("4k3/8/8/3pP3/8/8/8/4K3 w - d6 0 1"));
    const auto enPassant = board.findLegalMove(Board::fromAlgebraic("e5"), Board::fromAlgebraic("d6"));
    assert(enPassant && enPassant->enPassant);

    assert(board.loadFen("8/P3k3/8/8/8/8/8/4K3 w - - 0 1"));
    const auto promotion = board.findLegalMove(Board::fromAlgebraic("a7"), Board::fromAlgebraic("a8"), PieceType::Knight);
    assert(promotion && promotion->promotion == PieceType::Knight);

    assert(board.loadFen("7k/6Q1/6K1/8/8/8/8/8 b - - 0 1"));
    assert(board.gameState() == GameState::WhiteWon);

    std::cout << "Chess rules tests passed\n";
}
