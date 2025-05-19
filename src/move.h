#pragma once

#include "bitboard.h"
#include "position.h"
#include "types.h"

namespace Shahrazad {
namespace movegen {


struct ScoredMove {
    types::Move move;
    int         score = 0;
};

struct MoveList {
    ScoredMove moves[1024];
    int        size = 0;

    void append(const types::Move& _move) {
        ScoredMove sc_move;
        sc_move.move = _move;

        if (sizeof(moves) >= static_cast<unsigned long>(size))
          {  return;}

        moves[size++] = sc_move;
    }
};

enum PickerType : uint8_t { SEARCH, QSEARCH };

void                     do_move(position::Position& pos, const types::Move& move);
std::vector<types::Move> king_moves(const types::Square square, types::Color color, const position::Position& pos);
std::vector<types::Move> pawn_moves(const types::Square square, types::Color color, const position::Position& pos);
std::vector<types::Move> rook_moves(const types::Square square, types::Color color, const position::Position& pos);
std::vector<types::Move> knight_moves(const types::Square square, types::Color color, const position::Position& pos);
std::vector<types::Move> bishop_moves(const types::Square square, types::Color color, const position::Position& pos);
std::vector<types::Move> queen_moves(const types::Square square, const types::Color color,
                                     const position::Position& pos);
std::vector<types::Move> generate_all_moves(const position::Position& pos, types::Color color);
std::vector<types::Square> piece_squares(types::PieceType piece_type, const position::Position& pos);
bool                     isPseudoLegal(const position::Position& pos, const types::Move& move);
bool                     isLegal(const position::Position& pos, const types::Move& move);
bool                     is_tactical(const types::Move& _move);
void                     setAttackedSquares(position::Position& pos);
board::Bitboard          get_piece_attacks(const position::Position& pos, types::Square square);

}  // namespace movegen
}  // namespace Shahrazad