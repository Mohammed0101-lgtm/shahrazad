#include "position.h"
#include "move.h"
#include <cassert>
#include <vector>


Bitboard Position::_black_occupancy() const {
    Bitboard occ = black_bishops.board() | black_king.board() | black_knights.board()
                 | black_pawns.board() | black_queens.board() | black_rooks.board();

    return occ;
}

Bitboard Position::_white_occupancy() const {
    Bitboard occ = white_bishops.board() | white_king.board() | white_knights.board()
                 | white_pawns.board() | white_queens.board() | white_rooks.board();

    return occ;
}

Bitboard Position::occupancy() const {
    Bitboard occ = _black_occupancy().board() | _white_occupancy().board();

    return occ;
}

pieceType Position::pieceOn(const int sq) const {
    assert(sq < 64 && sq >= 0);
    return pieces[sq];
}

bool Position::isAttacked(int sq, int color) const {
    assert(color == WHITE || color == BLACK);
    return color == WHITE ? attacked_white[sq] : attacked_black[sq];
}

bool Position::canCastle(int castle_side) const { return castle_perm[castle_side]; }

uint8_t Position::getSide() const { return current_side; }

uint8_t Position::piece_count() const { return occupancy().count(); }

Position Position::get_previous_position() const { return *prev; }

void Position::switch_side() { current_side ^= 1; }

uint8_t Position::king_square(int color) const {
    return color == WHITE ? white_king.square() : black_king.square();
}

bool Position::isPiece(int sq, pieceType piece) const {
    assert(sq >= 0 && sq <= 64);
    if (piece != pieces[sq])
        return false;
    return true;
}

uint8_t Position::material_score() const {
    int score = 0;
    for (int i = 0; i < 64; i++)
    {
        if (pieces[i] == NOPE || pieces[i] == KING)
            continue;

        switch (pieces[i])
        {

        case PAWN :
            score += PAWN_VAL;
            break;

        case KNIGHT :
            score += KNIGHT_VAL;
            break;

        case BISHOP :
            score += BISHOP_VAL;
            break;

        case ROOK :
            score += ROOK_VAL;
            break;

        case QUEEN :
            score += QUEEN_VAL;
            break;

        default :
            break;
        }
    }

    return score;
}

uint8_t Position::getColor(int sq) const {
    pieceType piece = pieceOn(sq);

    if (piece == NOPE)
        return BOTH;

    Bitboard occ = _white_occupancy();

    return occ.is_bitset(sq) ? WHITE : BLACK;
}

uint8_t Position::numberOf(pieceType piece, int color) const {
    assert(color < BOTH && color >= WHITE);

    switch (piece)
    {

    case QUEEN :
        return color == WHITE ? white_queens.count() : black_queens.count();
    case ROOK :
        return color == WHITE ? white_rooks.count() : black_rooks.count();
    case BISHOP :
        return color == WHITE ? white_bishops.count() : black_bishops.count();
    case KNIGHT :
        return color == WHITE ? white_knights.count() : white_knights.count();
    case PAWN :
        return color == WHITE ? white_pawns.count() : white_pawns.count();

    default :
        return 0;
    }
}

void Position::make_null_move() {
    switch_side();
    played_positions.push_back(position_key);
    ply_fromNull = 0;
    fifty_moves_counter++;
    stacked_his++;
}

void Position::take_null_move() {
    // I am not really sure wheather or not that is the right thing to do
    *this = *prev;
}

void Position::undo_move() {
    *this      = *prev;
    this->prev = &(Position(played_positions[played_positions.size() - 1]));
}

uint8_t Position::isPinned(const int sq) const {
    if (pieces[sq] == NOPE)
        return false;

    Bitboard opponent_occupancy = current_side == WHITE ? _black_occupancy() : _white_occupancy();

    for (int op_sq = 0; op_sq < 64 && opponent_occupancy.is_bitset(op_sq); op_sq++)
    {
        switch (pieces[op_sq])
        {
        case BISHOP : {
            std::vector<uint8_t> moves =
              bishop_moves(sq, current_side == WHITE ? BLACK : WHITE, *this);

            if (std::find(moves.begin(), moves.end(), sq) != moves.end())
                return op_sq;

            break;
        }

        case QUEEN : {
            std::vector<uint8_t> moves =
              queen_moves(sq, current_side == WHITE ? BLACK : WHITE, *this);

            if (std::find(moves.begin(), moves.end(), sq) != moves.end())
                return op_sq;

            break;
        }

        case ROOK : {
            std::vector<uint8_t> moves =
              rook_moves(sq, current_side == WHITE ? BLACK : WHITE, *this);

            if (std::find(moves.begin(), moves.end(), sq) != moves.end())
                return op_sq;

            break;
        }

        default :
            break;
        }
    }

    return NOPE;
}

uint64_t genPositionKey(const Position& pos) {
    uint64_t key;

    for (int sq = 0; sq < 64; sq++)
    {
        const pieceType piece = pos.pieceOn(sq);

        if (piece != NOPE)
        {
            assert(piece >= KING && piece <= PAWN);
            key ^= pieceKeys[piece][sq];
        }
    }

    if (pos.getSide() == WHITE)
        return (key ^= sideKey[0]);
    else
        return (key ^= sideKey[1]);
}

void Position::reset() {
    white_pawns   = Bitboard();
    white_king    = Bitboard();
    white_knights = Bitboard();
    white_bishops = Bitboard();
    white_queens  = Bitboard();
    white_rooks   = Bitboard();

    black_pawns   = Bitboard();
    black_king    = Bitboard();
    black_knights = Bitboard();
    black_bishops = Bitboard();
    black_queens  = Bitboard();
    black_rooks   = Bitboard();

    current_side        = WHITE;
    enPassant_square    = NOPE;
    stacked_his         = uint8_t();
    half_moves          = uint8_t();
    fifty_moves_counter = uint8_t();
    ply_fromNull        = uint8_t();
    position_key        = uint64_t();
    played_positions    = std::vector<uint64_t>(0ULL);
    prev                = nullptr;

    for (int i = 0; i < 64; i++)
    {
        pieces[i]         = NOPE;
        attacked_white[i] = false;
        attacked_black[i] = false;
    }

    for (int i = 0; i < 2; i++)
    {
        castle_perm[i] = false;
        castle_perm[i] = false;
    }
}
