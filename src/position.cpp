#include "position.h"
#include "move.h"
#include <cassert>
#include <vector>

Bitboard Position::_black_occupancy() const {
    Bitboard occ = black_bishops.board() | black_king.board() | black_knights.board() | black_pawns.board() |
                   black_queens.board() | black_rooks.board();

    return occ;
}

Bitboard Position::_white_occupancy() const {
    Bitboard occ = white_bishops.board() | white_king.board() | white_knights.board() | white_pawns.board() |
                   white_queens.board() | white_rooks.board();

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

bool     Position::canCastle(int castle_side) const { return castle_perm[castle_side]; }

int      Position::getSide() const { return current_side; }

int      Position::piece_count() const { return occupancy().count(); }

Position Position::get_previous_position() const { return *prev; }

void     Position::switch_side() { current_side ^= 1; }

int      Position::king_square(int color) const {
    return color == WHITE ? white_king.square() : black_king.square();
}

bool Position::isPiece(int sq, pieceType piece) const {
    assert(sq >= 0 && sq <= 64);
    if (piece != pieces[sq])
        return false;
    return true;
}

int Position::material_score() const {
    int score = 0;
    for (int i = 0; i < 64; i++) {
        if (pieces[i] == NOPE || pieces[i] == KING)
            continue;

        switch (pieces[i]) {

        case PAWN:
            score += PAWN_VAL;
            break;

        case KNIGHT:
            score += KNIGHT_VAL;
            break;

        case BISHOP:
            score += BISHOP_VAL;
            break;

        case ROOK:
            score += ROOK_VAL;
            break;

        case QUEEN:
            score += QUEEN_VAL;
            break;

        default:
            break;
        }
    }

    return score;
}

unsigned int Position::getColor(int sq) const {
    pieceType piece = pieceOn(sq);

    if (piece == NOPE) {
        return BOTH;
    }

    Bitboard occ = _white_occupancy();

    return occ.is_bitset(sq) ? WHITE : BLACK;
}

int Position::numberOf(pieceType piece, int color) const {
    assert(color < BOTH && color >= WHITE);

    switch (piece) {

    case QUEEN:
        return color == WHITE ? white_queens.count() : black_queens.count();

    case ROOK:
        return color == WHITE ? white_rooks.count() : black_rooks.count();

    case BISHOP:
        return color == WHITE ? white_bishops.count() : black_bishops.count();

    case KNIGHT:
        return color == WHITE ? white_knights.count() : white_knights.count();

    case PAWN:
        return color == WHITE ? white_pawns.count() : white_pawns.count();

    default:
        return 0;
    }
}

void Position::make_null_move() {
    Position* previous_pos = this;
    switch_side();
    played_positions.emplace_back(position_key);
    ply_fromNull = 0;
    fifty_moves_counter++;
    stacked_his++;
}

void Position::undo_move(const Move move) {
    *this      = *prev;
    this->prev = &Position(played_positions[played_positions.size() - 1]);
}

int Position::isPinned(const int sq) const {
    if (pieces[sq] == NOPE)
        return false;

    Bitboard opponent_occupancy = current_side == WHITE ? _black_occupancy() : _white_occupancy();

    for (int op_sq = 0; op_sq < 64 && opponent_occupancy.is_bitset(op_sq); op_sq++) {
        switch (pieces[op_sq]) {
        case BISHOP: {
            std::vector<int8_t> moves = bishop_moves(sq, current_side == WHITE ? BLACK : WHITE, *this);

            if (std::find(moves.begin(), moves.end(), sq) != moves.end()) {
                return op_sq;
            }

            break;
        }

        case QUEEN: {
            std::vector<int8_t> moves = queen_moves(sq, current_side == WHITE ? BLACK : WHITE, *this);

            if (std::find(moves.begin(), moves.end(), sq) != moves.end()) {
                return op_sq;
            }

            break;
        }

        case ROOK: {
            std::vector<int8_t> moves = rook_moves(sq, current_side == WHITE ? BLACK : WHITE, *this);

            if (std::find(moves.begin(), moves.end(), sq) != moves.end()) {
                return op_sq;
            }

            break;
        }

        default:
            break;
        }
    }

    return NOPE;
}

uint64_t genPositionKey(const Position& pos) {
    uint64_t key;

    for (int sq = 0; sq < 64; sq++) {
        const pieceType piece = pos.pieceOn(sq);

        if (piece != NOPE) {
            assert(piece >= KING && piece <= PAWN);
            key ^= pieceKeys[piece][sq];
        }
    }

    if (pos.getSide() == WHITE) {
        return (key ^= sideKey[0]);
    } else {
        return (key ^= sideKey[1]);
    }
}
