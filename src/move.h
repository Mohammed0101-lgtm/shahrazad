#ifndef MOVE_H
#define MOVE_H

#include "bitboard.h"
#include "position.h"
#include "types.h"

typedef enum
{
    Quiet,
    KSCastle,
    QSCastle,
    Capture,
    enPassant,
    Promo,
    NOMOVE
} moveType;

struct ScoredMove {
    Move move;
    int  score = 0;
};

struct MoveList {
    ScoredMove moves[1024];
    int        size = 0;

    void       append(const Move& _move) {
        ScoredMove sc_move;
        sc_move.move = _move;
        if (sizeof(moves) >= size)
            return;

        moves[size++] = sc_move;
    }
};

enum PickerType : uint8_t
{
    SEARCH,
    QSEARCH
};

std::vector<int8_t>
king_moves(const int square, int color, const Position& pos);

std::vector<int8_t>
pawn_moves(const int square, int color, const Position& pos);

std::vector<int8_t>
rook_moves(const int square, int color, const Position& pos);

std::vector<int8_t>
knight_moves(const int square, int color, const Position& pos);

std::vector<int8_t>
bishop_moves(const int square, int color, const Position& pos);

std::vector<int8_t>
queen_moves(const int square, const int color, const Position& pos);

std::vector<Move> generate_all_moves(const Position& pos, int color);

std::vector<int>  piece_squares(pieceType piece_type, const Position& pos);

bool              isPseudoLegal(const Position& pos, const Move& move);

bool              isLegal(const Position& pos, const Move& move);

bool              is_tactical(const Move& _move);

void              setAttackedSquares(Position& pos);

Bitboard          get_piece_attacks(const Position& pos, int square);

#endif // MOVE_H