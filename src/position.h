#pragma once

#include "bitboard.h"
#include "types.h"

#include <vector>


namespace Shahrazad {
namespace position {

inline uint64_t pieceKeys[12][64];
inline uint64_t sideKey[2];


class Position {
   public:
    board::Bitboard white_pawns;
    board::Bitboard white_king;
    board::Bitboard white_knights;
    board::Bitboard white_bishops;
    board::Bitboard white_queens;
    board::Bitboard white_rooks;

    board::Bitboard black_pawns;
    board::Bitboard black_king;
    board::Bitboard black_knights;
    board::Bitboard black_bishops;
    board::Bitboard black_queens;
    board::Bitboard black_rooks;

    types::Color   current_side;
    types::Square   enPassant_square    = types::Square::NONE;
    uint8_t   stacked_his         = 0;
    uint8_t   half_moves          = 0;
    uint8_t   fifty_moves_counter = 0;
    uint8_t   ply_fromNull        = 0;
    types::PieceType pieces[64];
    bool      attacked_white[64];
    bool      attacked_black[64];
    bool      castle_perm[2];
    bool      inCheck;
    bool      needs_refresh[2];

    uint64_t              position_key;
    std::vector<uint64_t> played_positions;
    Position*             prev;


    Position() :
        current_side(types::Color::WHITE),
        attacked_white{false},
        attacked_black{false} {}

    Position(types::Color color) :
        current_side(color),
        attacked_black{false},
        attacked_white{false} {}
    
    Position(const std::vector<uint64_t>& played_positions) {}
    
    Position get_previous_position() const;
    board::Bitboard _black_occupancy() const;
    board::Bitboard _white_occupancy() const;
    board::Bitboard occupancy() const;
    void reset();
    void switch_side();
    void undo_move();
    void make_null_move();
    void take_null_move();
    bool canCastle(int castle_side) const;
    bool isAttacked(types::Square sq, types::Color color) const;
    bool isPiece(int sq, types::PieceType piece) const;
    types::PieceType pieceOn(const int sq) const;
    types::PieceType pieceOn(const types::Square sq) const;
    types::Color getSide() const;
    types::Square isPinned(const types::Square sq) const;
    unsigned int material_score() const;
    unsigned int piece_count() const;
    unsigned int numberOf(types::PieceType piece, types::Color color) const;
    types::Square king_square(types::Color color) const;
    types::Color getColor(types::Square sq) const;
};


}  // namespace position
}  // namespace Shahrazad