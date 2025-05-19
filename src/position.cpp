#include "position.h"
#include "move.h"
#include <cassert>
#include <vector>


namespace Shahrazad {
namespace position {

board::Bitboard Position::_black_occupancy() const {
    board::Bitboard occ = black_bishops.board() | black_king.board() | black_knights.board() | black_pawns.board()
                        | black_queens.board() | black_rooks.board();

    return occ;
}

board::Bitboard Position::_white_occupancy() const {
    board::Bitboard occ = white_bishops.board() | white_king.board() | white_knights.board() | white_pawns.board()
                        | white_queens.board() | white_rooks.board();

    return occ;
}

board::Bitboard Position::occupancy() const {
    board::Bitboard occ = _black_occupancy().board() | _white_occupancy().board();
    return occ;
}

types::PieceType Position::pieceOn(const int sq) const {
    assert(sq < 64 && sq >= 0);
    return pieces[sq];
}

types::PieceType Position::pieceOn(const types::Square sq) const {
    assert(sq < types::Square::h8 && sq >= types::Square::a1);
    return pieces[static_cast<int>(sq)];
}

bool Position::isAttacked(types::Square sq, types::Color color) const {
    assert(color == types::Color::WHITE || color == types::Color::BLACK);
    return color == types::Color::WHITE ? attacked_white[static_cast<int>(sq)] : attacked_black[static_cast<int>(sq)];
}

bool         Position::canCastle(int castle_side) const { return castle_perm[castle_side]; }
types::Color Position::getSide() const { return current_side; }
unsigned int Position::piece_count() const { return occupancy().count(); }
Position     Position::get_previous_position() const { return *prev; }
void         Position::switch_side() { current_side = types::Color(static_cast<int>(current_side) ^ 1); }

types::Square Position::king_square(types::Color color) const {
    return color == types::Color::WHITE ? types::Square(white_king.square()) : types::Square(black_king.square());
}

bool Position::isPiece(int sq, types::PieceType piece) const {
    assert(sq >= 0 && sq <= 64);

    if (piece != pieces[sq])
    {
        return false;
    }

    return true;
}

unsigned int Position::material_score() const {
    int score = 0;
    for (int i = 0; i < 64; i++)
    {
        if (pieces[i] == types::PieceType::NOPE || pieces[i] == types::PieceType::KING)
        {
            continue;
        }

        switch (pieces[i])
        {

        case types::PieceType::PAWN :
            score += types::PAWN_VAL;
            break;

        case types::PieceType::KNIGHT :
            score += types::KNIGHT_VAL;
            break;

        case types::PieceType::BISHOP :
            score += types::BISHOP_VAL;
            break;

        case types::PieceType::ROOK :
            score += types::ROOK_VAL;
            break;

        case types::PieceType::QUEEN :
            score += types::QUEEN_VAL;
            break;

        default :
            break;
        }
    }

    return score;
}

types::Color Position::getColor(types::Square sq) const {
    types::PieceType piece = pieceOn(sq);

    if (piece == types::PieceType::NOPE)
    {
        return types::Color::BOTH;
    }

    board::Bitboard occ = _white_occupancy();
    return occ.is_bitset(sq) ? types::Color::WHITE : types::Color::BLACK;
}

unsigned int Position::numberOf(types::PieceType piece, types::Color color) const {
    assert(color == types::Color::BLACK || color == types::Color::WHITE);

    switch (piece)
    {

    case types::PieceType::QUEEN :
        return color == types::Color::WHITE ? white_queens.count() : black_queens.count();
    case types::PieceType::ROOK :
        return color == types::Color::WHITE ? white_rooks.count() : black_rooks.count();
    case types::PieceType::BISHOP :
        return color == types::Color::WHITE ? white_bishops.count() : black_bishops.count();
    case types::PieceType::KNIGHT :
        return color == types::Color::WHITE ? white_knights.count() : white_knights.count();
    case types::PieceType::PAWN :
        return color == types::Color::WHITE ? white_pawns.count() : white_pawns.count();

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
    *this = *prev;
    // static position::Position previous = position::Position(played_positions[played_positions.size() - 1]);
    // this->prev = &previous;
}

types::Square Position::isPinned(const types::Square sq) const {
    if (pieces[static_cast<int>(sq)] == types::PieceType::NOPE)
      {  return types::Square::NONE;}

    board::Bitboard opponent_occupancy = current_side == types::Color::WHITE ? _black_occupancy() : _white_occupancy();

    for (int op_sq = 0; op_sq < 64 && opponent_occupancy.is_bitset(op_sq); op_sq++)
    {
        switch (pieces[op_sq])
        {
        case types::PieceType::BISHOP : {
            std::vector<types::Move> moves =
              movegen::bishop_moves(sq, current_side == types::Color::WHITE ? types::Color::BLACK : types::Color::WHITE,
                                    *this);

            if (std::find(moves.begin(), moves.end(), sq) != moves.end())
            {
                return types::Square(op_sq);
            }
            break;
        }

        case types::PieceType::QUEEN : {
            std::vector<types::Move> moves =
              movegen::queen_moves(sq, current_side == types::Color::WHITE ? types::Color::BLACK : types::Color::WHITE,
                                   *this);

            if (std::find(moves.begin(), moves.end(), sq) != moves.end())
            {
                return types::Square(op_sq);
            }
            break;
        }

        case types::PieceType::ROOK : {
            std::vector<types::Move> moves =
              movegen::rook_moves(sq, current_side == types::Color::WHITE ? types::Color::BLACK : types::Color::WHITE,
                                  *this);

            if (std::find(moves.begin(), moves.end(), sq) != moves.end())
            {
                return types::Square(op_sq);
            }
            break;
        }

        default :
            break;
        }
    }

    return types::Square::NONE;
}

uint64_t genPositionKey(const Position& pos) {
    uint64_t key = 0;

    for (int sq = 0; sq < 64; sq++)
    {
        const types::PieceType piece = pos.pieceOn(sq);

        if (piece != types::PieceType::NOPE)
        {
            assert(piece >= types::PieceType::KING && piece <= types::PieceType::PAWN);
            key ^= pieceKeys[static_cast<int>(piece)][sq];
        }
    }

    if (pos.getSide() == types::Color::WHITE)
    {
        return (key ^= sideKey[0]);
    }
    else
    {
        return (key ^= sideKey[1]);
    }
}

void Position::reset() {
    white_pawns   = board::Bitboard();
    white_king    = board::Bitboard();
    white_knights = board::Bitboard();
    white_bishops = board::Bitboard();
    white_queens  = board::Bitboard();
    white_rooks   = board::Bitboard();

    black_pawns   = board::Bitboard();
    black_king    = board::Bitboard();
    black_knights = board::Bitboard();
    black_bishops = board::Bitboard();
    black_queens  = board::Bitboard();
    black_rooks   = board::Bitboard();

    current_side        = types::Color::WHITE;
    enPassant_square    = types::Square::NONE;
    stacked_his         = uint8_t();
    half_moves          = uint8_t();
    fifty_moves_counter = uint8_t();
    ply_fromNull        = uint8_t();
    position_key        = uint64_t();
    played_positions    = std::vector<uint64_t>(0ULL);
    prev                = nullptr;

    for (int i = 0; i < 64; i++)
    {
        pieces[i]         = types::PieceType::NOPE;
        attacked_white[i] = false;
        attacked_black[i] = false;
    }

    for (int i = 0; i < 2; i++)
    {
        castle_perm[i] = false;
        castle_perm[i] = false;
    }
}

}  // namespace position
}  // namespace Shahrazad