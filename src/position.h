#pragma once

#include "bitboard.h"
#include "types.h"

#include <vector>


inline uint64_t pieceKeys[12][64];
inline uint64_t sideKey[2];

class Position {
   public:
    Bitboard white_pawns;
    Bitboard white_king;
    Bitboard white_knights;
    Bitboard white_bishops;
    Bitboard white_queens;
    Bitboard white_rooks;

    Bitboard black_pawns;
    Bitboard black_king;
    Bitboard black_knights;
    Bitboard black_bishops;
    Bitboard black_queens;
    Bitboard black_rooks;

    uint8_t   current_side;
    uint8_t   enPassant_square    = NOPE;
    uint8_t   stacked_his         = 0;
    uint8_t   half_moves          = 0;
    uint8_t   fifty_moves_counter = 0;
    uint8_t   ply_fromNull        = 0;
    pieceType pieces[64];
    bool      attacked_white[64];
    bool      attacked_black[64];
    bool      castle_perm[2];
    bool      inCheck;
    bool      needs_refresh[2];

    uint64_t              position_key;
    std::vector<uint64_t> played_positions;
    Position*             prev;


    Position() :
        current_side(WHITE) {
        for (int i = 0; i < 64; i++)
            attacked_black[i] = false;

        for (int i = 0; i < 64; i++)
            attacked_white[i] = false;
    }

    Position(int color) :
        current_side(color) {
        for (int i = 0; i < 64; i++)
            attacked_black[i] = false;

        for (int i = 0; i < 64; i++)
            attacked_white[i] = false;
    }

    Position get_previous_position() const;

    Bitboard _black_occupancy() const;

    Bitboard _white_occupancy() const;

    Bitboard occupancy() const;

    void reset();

    void switch_side();

    void undo_move();

    void make_null_move();

    void take_null_move();

    bool canCastle(int castle_side) const;

    bool isAttacked(int sq, int color) const;

    bool isPiece(int sq, pieceType piece) const;

    pieceType pieceOn(const int sq) const;

    uint8_t getSide() const;

    uint8_t isPinned(const int sq) const;

    uint8_t material_score() const;

    uint8_t piece_count() const;

    uint8_t numberOf(pieceType piece, int color) const;

    uint8_t king_square(int color) const;

    uint8_t getColor(int sq) const;
};
