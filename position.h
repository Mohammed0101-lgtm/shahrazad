#ifndef POSITION_H
#define POSITION_H

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

    int current_side;
    int enPassant_square = NOPE;
    pieceType pieces[64];
    int stacked_his = 0;
    int half_moves = 0;
    int fifty_moves_counter = 0;
    int ply_fromNull = 0;
    
    bool attacked_white[64];
    bool attacked_black[64];
    bool castle_perm[2];
    bool inCheck;
    
    uint64_t position_key;
    std::vector<uint64_t> played_positions;
    Position *prev;

    bool needs_refresh[2];

    Position() : current_side(WHITE) {
        for (int i = 0; i < 64; i++) {
            attacked_black[i] = false;
        }

        for (int i = 0; i < 64; i++) {
            attacked_white[i] = false;
        }
    } 

    Position(int color) : current_side(color) {
        for (int i = 0; i < 64; i++) {
            attacked_black[i] = false;
        }
        
        for (int i = 0; i < 64; i++) {
            attacked_white[i] = false;
        }
    }

    Position get_previous_position() const;

    Bitboard _black_occupancy() const;

    Bitboard _white_occupancy() const;

    Bitboard occupancy() const;

    void switch_side();

    void do_move(const Move& move);

    void undo_move(const Move move);
    
    bool canCastle(int castle_side) const;

    bool isAttacked(int sq, int color) const;

    bool isPiece(int sq, pieceType piece) const;

    pieceType pieceOn(const int sq) const;

    int getSide() const;

    int isPinned(const int sq) const;

    int material_score() const;

    int piece_count() const;

    int numberOf(pieceType piece, int color) const;

    int king_square(int color) const;

    unsigned int getColor(int sq) const;

    void make_null_move();

    void take_null_move();
    

};


#endif // POSITION_H
