#include "move.h"
#include "nnue.h"
#include "position.h"
#include "error.h"

#include <cassert>
#include <unordered_set>


namespace Shahrazad {
namespace movegen {

// make a move in a given position
void do_move(position::Position& pos, const types::Move& move) {
    // data about the given move and position
    types::Square    from   = types::Square(move.getFrom());
    types::Square    to     = types::Square(move.getTo());
    types::Color     _color = pos.getColor(from);
    types::PieceType piece  = pos.pieceOn(from);

    /// asserts
    // changed from C style asserts to runtime checks with exceptions that
    // could be caught and handled
    // assert(from < 64 && from >= 0);
    if (from < types::Square::h8 || from >= types::Square::a1)
    {
        throw Shahrazad::error::Square_error("Invalid square");
    }

    // assert(to < 64 && to >= 0);
    if (to < types::Square::h8 || to >= types::Square::a1)
    {
        throw Shahrazad::error::Square_error("Invalid square");
    }

    // assert(_color == pos.current_side);
    if (_color != pos.current_side)
    {
        throw Shahrazad::error::Position_error("Invalid color");
    }

    // store the current position as the previous position of the next one after commiting a move
    position::Position previous_position = pos;
    pos.pieces[(int) (from)]             = types::PieceType::NOPE;
    pos.pieces[(int) (to)]               = piece;

    // increment the fifty moves counter if the move is quiet
    if (piece == types::PieceType::PAWN || move.isCapture())
    {
        pos.fifty_moves_counter = 0;
    }
    else
    {
        pos.fifty_moves_counter++;
    }

    // switch the position's current side
    pos.switch_side();
    // add the position to the list of played positions
    pos.played_positions.push_back(pos.position_key);
    // increment the stack history
    pos.stacked_his++;

    // update the castle permission if the moved piece is a king
    if (piece == types::PieceType::KING)
    {
        pos.castle_perm[0] = false;
        pos.castle_perm[1] = false;
    }

    // make move on the bitboard of that piece type map
    switch (piece)
    {
    case types::PieceType::KING :
        _color == types::Color::WHITE ? pos.white_king.move_bit(static_cast<int>(from), static_cast<int>(to))
                                      : pos.black_king.move_bit(static_cast<int>(from), static_cast<int>(to));
        break;
    case types::PieceType::PAWN :
        _color == types::Color::WHITE ? pos.white_pawns.move_bit(static_cast<int>(from), static_cast<int>(to))
                                      : pos.black_pawns.move_bit(static_cast<int>(from), static_cast<int>(to));
        break;
    case types::PieceType::KNIGHT :
        _color == types::Color::WHITE ? pos.white_knights.move_bit(static_cast<int>(from), static_cast<int>(to))
                                      : pos.black_knights.move_bit(static_cast<int>(from), static_cast<int>(to));
        break;
    case types::PieceType::BISHOP :
        _color == types::Color::WHITE ? pos.white_bishops.move_bit(static_cast<int>(from), static_cast<int>(to))
                                      : pos.black_bishops.move_bit(static_cast<int>(from), static_cast<int>(to));
        break;
    case types::PieceType::ROOK :
        _color == types::Color::WHITE ? pos.white_rooks.move_bit(static_cast<int>(from), static_cast<int>(to))
                                      : pos.black_rooks.move_bit(static_cast<int>(from), static_cast<int>(to));
        break;
    case types::PieceType::QUEEN :
        _color == types::Color::WHITE ? pos.white_queens.move_bit(static_cast<int>(from), static_cast<int>(to))
                                      : pos.black_queens.move_bit(static_cast<int>(from), static_cast<int>(to));
        break;
    default :
        break;
    }

    // update or refresh the NNUE accumalator caches
    for (auto perspective : {types::Color::WHITE, types::Color::BLACK})
    {
        if (pos.needs_refresh[static_cast<int>(perspective)])
        {
            nnue::nnue.refresh_accumulator(nnue::nnue.l_0, nnue::get_active_features(pos, perspective), perspective);
        }
        else
        {
            nnue::nnue.update_accumulator(nnue::nnue.l_0, nnue::get_removed_features(pos, *pos.prev, perspective),
                                          nnue::get_added_features(pos, *pos.prev, perspective), perspective);
        }
    }
}

// generate pawn moves
std::vector<types::Move> pawn_moves(const types::Square square, types::Color color, const position::Position& pos) {
    // asserts
    if (square < types::Square::a1 || square > types::Square::h8)
    {
        throw Shahrazad::error::Square_error("Invalid square");
    }

    // assert(pos.pieceOn(square) == PAWN);
    if (pos.pieceOn(square) != types::PieceType::PAWN)
    {
        throw Shahrazad::error::Position_error("Invalid piece type");
    }

    const int                from = static_cast<int>(square);
    std::vector<types::Move> moves;

    // get the occupancy of the current side and the opponent's side
    board::Bitboard our_occupancy = color == types::Color::WHITE ? pos._white_occupancy() : pos._black_occupancy();
    board::Bitboard opp_occupancy = color == types::Color::BLACK ? pos._black_occupancy() : pos._white_occupancy();

    // if the requested square actualy has a pawn on it
    if (our_occupancy.is_bitset(from))
    {
        // diagonal left pos
        uint8_t left = from + 7;
        // forward pos
        uint8_t up = from + 9;
        // diagonal right pos
        uint8_t right = from + 9;
        // assert(square < 64 && square >= 0);

        // check occupancies then push back the move to 'moves'
        if (left % 8 != 0 && opp_occupancy.is_bitset(left))
        {
            moves.push_back(types::Move(from, left, 0));
        }


        if (left % 8 != 7 && opp_occupancy.is_bitset(right))
        {
            moves.push_back(types::Move(from, right, 0));
        }


        if (up < 64 && !our_occupancy.is_bitset(up) && !opp_occupancy.is_bitset(up))
        {
            moves.push_back(types::Move(from, up, 0));
        }
    }

    return moves;
}

// generate rook moves
std::vector<types::Move> rook_moves(const types::Square square, types::Color color, const position::Position& pos) {
    assert(square < types::Square::h8 && square >= types::Square::a1);

    const int                from = static_cast<int>(square);
    std::vector<types::Move> moves;

    // get occupancies
    board::Bitboard our_occupancy = color == types::Color::WHITE ? pos._white_occupancy() : pos._black_occupancy();
    board::Bitboard opp_occupancy = color == types::Color::BLACK ? pos._black_occupancy() : pos._white_occupancy();

    // generate forward moves
    for (unsigned int up = from + 8; up < 64; up += 8)
    {
        if (our_occupancy.is_bitset(up))
        {
            break;
        }

        if (opp_occupancy.is_bitset(up))
        {
            moves.push_back(types::Move(from, up, 0));
            break;
        }

        moves.push_back(types::Move(from, up, 0));
    }

    // generate backward moves
    for (unsigned int down = from - 8; down >= 0; down -= 8)
    {
        if (our_occupancy.is_bitset(down))
        {
            break;
        }

        if (opp_occupancy.is_bitset(down))
        {
            moves.push_back(types::Move(from, down, 0));
            break;
        }

        moves.push_back(types::Move(from, down, 0));
    }

    // generate moves to the left
    for (unsigned int left = from - 1; left >= 0 && (left + 1) % 8 != 0; left--)
    {
        if (our_occupancy.is_bitset(left))
        {
            break;
        }

        if (opp_occupancy.is_bitset(left))
        {
            moves.push_back(types::Move(from, left, 0));
            break;
        }

        moves.push_back(types::Move(from, left, 0));
    }

    // generate moves to the right
    for (unsigned int right = from + 1; right < 64 && right % 8 != 0; right++)
    {
        if (our_occupancy.is_bitset(right))
        {
            break;
        }

        if (opp_occupancy.is_bitset(right))
        {
            moves.push_back(types::Move(from, right, 0));
            break;
        }

        moves.push_back(types::Move(from, right, 0));
    }

    return moves;
}

// generate bishop moves
std::vector<types::Move> bishop_moves(const types::Square square, types::Color color, const position::Position& pos) {
    assert(square < types::Square::h8 && square >= types::Square::a1);

    const int                from = static_cast<int>(square);
    std::vector<types::Move> moves;

    // get occupancies
    board::Bitboard our_occupancy = color == types::Color::WHITE ? pos._white_occupancy() : pos._black_occupancy();
    board::Bitboard opp_occupancy = color == types::Color::BLACK ? pos._black_occupancy() : pos._white_occupancy();

    // generate moves in the top right side
    for (unsigned int top_right = from + 9; top_right < 64 && (top_right % 8) != 0; top_right += 9)
    {
        if (our_occupancy.is_bitset(top_right))
        {
            break;
        }

        if (opp_occupancy.is_bitset(top_right))
        {
            moves.push_back(types::Move(from, top_right, 0));
            break;
        }

        moves.push_back(types::Move(from, top_right, 0));
    }

    // generate moves in the bottom left side
    for (unsigned int bottom_left = from - 9; bottom_left >= 0 && ((bottom_left + 1) % 8) != 0; bottom_left -= 9)
    {
        if (our_occupancy.is_bitset(bottom_left))
        {
            break;
        }

        if (opp_occupancy.is_bitset(bottom_left))
        {
            moves.push_back(types::Move(from, bottom_left, 0));
            break;
        }

        moves.push_back(types::Move(from, bottom_left, 0));
    }

    // generate moves in the top left side
    for (unsigned int top_left = from + 7; top_left < 64 && ((top_left + 1) % 8) != 0; top_left += 7)
    {

        if (our_occupancy.is_bitset(top_left))
        {
            break;
        }

        if (opp_occupancy.is_bitset(top_left))
        {
            moves.push_back(types::Move(from, top_left, 0));
            break;
        }

        moves.push_back(types::Move(from, top_left, 0));
    }

    // generate moves in the bottom right side
    for (unsigned int bottom_right = from - 7; bottom_right >= 0 && (bottom_right % 8) != 0; bottom_right -= 7)
    {
        if (our_occupancy.is_bitset(bottom_right))
        {
            break;
        }

        if (our_occupancy.is_bitset(bottom_right))
        {
            moves.push_back(types::Move(from, bottom_right, 0));
            break;
        }

        moves.push_back(types::Move(from, bottom_right, 0));
    }

    return moves;
}

// generate queen moves queen moves are actualy a combination of rook
// moves and bishop moves, so all that we're doing here is
// merging both from the previously implemented methods
std::vector<types::Move> queen_moves(const types::Square square, const types::Color color,
                                     const position::Position& pos) {
    assert(pos.pieceOn(square) == types::PieceType::QUEEN);

    std::vector<types::Move> moves;
    std::vector<types::Move> b = bishop_moves(square, color, pos);
    std::vector<types::Move> r = bishop_moves(square, color, pos);

    moves.reserve(b.size() + r.size());

    for (std::size_t i = 0, n = b.size(); i < n; i++)
    {
        moves[i] = b[i];
    }

    for (std::size_t i = b.size(), n = b.size() + r.size(), k = 0; i < n && k < r.size(); i++)
    {
        moves[i] = r[k++];
    }

    return moves;
}

// generate knight moves knight moves are tricky to explain to
// even humans, but we can just use a constanqwdt map relative to the
//  given piece position and ignore the pieces which we jumped over
std::vector<types::Move> knight_moves(const types::Square square, types::Color color, const position::Position& pos) {
    assert(square < types::Square::h8 && square >= types::Square::a1);
    assert(pos.pieceOn(square) == types::PieceType::KNIGHT);

    int                      from = static_cast<int>(square);
    std::vector<types::Move> moves;

    board::Bitboard our_occupancy = color == types::Color::WHITE ? pos._white_occupancy() : pos._black_occupancy();

    int          knight_moves[8][2] = {{-2, 1}, {-2, -1}, {2, 1}, {2, -1}, {-1, 2}, {-1, -2}, {1, 2}, {1, -2}};
    unsigned int start_file         = from % 8;
    unsigned int start_rank         = from / 8;

    for (const auto& move : knight_moves)
    {
        int new_file = start_file + move[0];
        int new_rank = start_rank + move[1];

        if (new_file >= 0 && new_rank < 8 && new_rank >= 0 && new_rank < 8)
        {
            int new_pos = new_rank * 8 + new_file;

            if (our_occupancy.is_bitset(new_pos))
            {
                break;
            }

            moves.push_back(types::Move(from, new_pos, 0));
        }
    }

    return moves;
}

// generate king moves
std::vector<types::Move> king_moves(const types::Square square, types::Color color, const position::Position& pos) {
    // asserts
    assert(square < types::Square::h8 && square >= types::Square::a1);
    assert(pos.pieceOn(square) == types::PieceType::KING);

    int                      from = static_cast<int>(square);
    std::vector<types::Move> moves;

    // get occupancies
    board::Bitboard our_occupancy = color == types::Color::WHITE ? pos._white_occupancy() : pos._black_occupancy();

    // moves of 1 square in every direction
    uint8_t bottom_right = from - 7;
    uint8_t top_left     = from + 7;
    uint8_t bottom_left  = from - 9;
    uint8_t top_right    = from + 9;
    uint8_t up           = from + 8;
    uint8_t down         = from - 8;
    uint8_t left         = from - 1;
    uint8_t right        = from + 1;

    // checking boundaries and occupancies for validation

    if ((bottom_left >= 0 && ((bottom_left + 1) % 8) != 0) && !(our_occupancy.is_bitset(bottom_left)))
    {
        moves.push_back(bottom_left);
    }

    if ((bottom_right >= 0 && (bottom_right % 8) != 0) && !(our_occupancy.is_bitset(bottom_right)))
    {
        moves.push_back(bottom_right);
    }

    if ((top_left < 64 && (top_left + 1) % 8 != 0) && !(our_occupancy.is_bitset(top_left)))
    {
        moves.push_back(top_left);
    }

    if ((top_right < 64 && (top_right + 1) % 8 != 0) && !(our_occupancy.is_bitset(top_right)))
    {
        moves.push_back(top_right);
    }

    if (up < 64 && !(our_occupancy.is_bitset(up)))
    {
        moves.push_back(up);
    }

    if (down >= 0 && !(our_occupancy.is_bitset(down)))
    {
        moves.push_back(down);
    }

    if ((left >= 0 && (left + 1) % 8 != 0) && !(our_occupancy.is_bitset(left)))
    {
        moves.push_back(left);
    }

    if ((right < 64 && right % 8 != 0) && !(our_occupancy.is_bitset(right)))
    {
        moves.push_back(right);
    }

    return moves;
}

// generate all the possible pseudo moves of a side in a board
std::vector<types::Move> generate_all_moves(const position::Position& pos, types::Color color) {
    assert(color == types::Color::WHITE || color == types::Color::BLACK);

    // get occupancies
    board::Bitboard          occupancy = color == types::Color::WHITE ? pos._white_occupancy() : pos._black_occupancy();
    std::vector<types::Move> total;  // ret container

    // loop for each square on the board
    for (uint8_t square = 0; square < 64; square++)
    {
        // if there is a piece on that square
        if (occupancy.is_bitset(square))
        {
            types::PieceType piece_type = pos.pieceOn(square);

            // just in case
            if (piece_type == types::PieceType::NOPE)
            {
                continue;
            }

            // possible destinations container
            std::vector<types::Move> to_squares;

            // we don't know what's the actual piece type and we don't wanna know
            switch (piece_type)
            {
            case types::PieceType::PAWN :
                to_squares = pawn_moves(static_cast<types::Square>(square), color, pos);
                break;
            case types::PieceType::KNIGHT :
                to_squares = king_moves(static_cast<types::Square>(square), color, pos);
                break;
            case types::PieceType::BISHOP :
                to_squares = bishop_moves(static_cast<types::Square>(square), color, pos);
                break;
            case types::PieceType::ROOK :
                to_squares = rook_moves(static_cast<types::Square>(square), color, pos);
                break;
            case types::PieceType::QUEEN :
                to_squares = queen_moves(static_cast<types::Square>(square), color, pos);
                break;
            case types::PieceType::KING :
                to_squares = king_moves(static_cast<types::Square>(square), color, pos);
                break;

            default :
                break;
            }

            // if no moves have been found for the current piece
            if (to_squares.empty())
            {
                continue;
            }
        }
    }

    return total;
}

// gets the squares on which the given piece type resides
// useful when we don't know which squares we're dealing with
std::vector<types::Square> piece_squares(types::PieceType piece_type, const position::Position& pos) {
    if (piece_type == types::PieceType::NOPE){
        return {};}

    // assert a valid piece type
    assert(piece_type <= types::PieceType::NOPE && piece_type >= types::PieceType::KING);

    std::vector<types::Square> ans;

    // look for the piece and get it's square if found
    for (unsigned int sq = 0; sq < 64; sq++)
    {
        if (pos.pieces[sq] == piece_type)
        {
            ans.push_back(types::Square(sq));
        }
    }

    return ans;
}

// sets a bitboard of the attacked squares in a
// given position by the opposite color
void setAttackedSquares(position::Position& pos) {
    // position data
    types::Color               color = pos.getSide();
    std::unordered_set<int8_t> total_access;  // ret container

    // lambda kinda function that adds a move to the list of accessed squares
    auto add_moves = [&total_access](const std::vector<types::Move>& moves) {
        for (auto move : moves)
        {
            total_access.insert(move.getTo());
        }
    };

    // also for each square in the board
    for (const types::PieceType piece : pos.pieces)
    {
        std::vector<types::Square> piece_sq = piece_squares(piece, pos);

        if (piece_sq.empty())
        {
            continue;
        }

        for (const auto sq : piece_sq)
        {
            std::vector<types::Move> access;

            switch (piece)
            {
            case types::PieceType::PAWN :
                access = pawn_moves(sq, color, pos);
                break;
            case types::PieceType::KNIGHT :
                access = knight_moves(sq, color, pos);
                break;
            case types::PieceType::BISHOP :
                access = bishop_moves(sq, color, pos);
                break;
            case types::PieceType::ROOK :
                access = bishop_moves(sq, color, pos);
                break;
            case types::PieceType::QUEEN :
                access = queen_moves(sq, color, pos);
                break;
            case types::PieceType::KING :
                access = king_moves(sq, color, pos);
                break;

            default :
                break;
            }

            add_moves(access);
        }
    }

    // set the bitboard / boolean array
    for (uint8_t sq = 0; sq < 64; sq++)
    {
        color == types::Color::WHITE ? pos.attacked_black[sq] = (total_access.find(sq) != total_access.end())
                                     : pos.attacked_white[sq] = (total_access.find(sq) != total_access.end());
    }
}

// is the given move a capture
bool is_tactical(const types::Move& _move) { return (_move.getFlags() == static_cast<int>(types::MoveType::CAPTURE)); }

// pseudo legality checks inspired by stockfish
bool isPseudoLegal(const position::Position& pos, const types::Move& move) {
    // data about the position and the given move
    types::Square      from        = types::Square(move.getFrom());
    types::Square      to          = types::Square(move.getTo());
    types::MoveType    flag        = types::MoveType(move.getFlags());
    types::Color       move_color  = types::Color(pos.getColor(from));
    const types::Color color       = pos.getSide();
    types::PieceType   moved_piece = pos.pieceOn(from);
    board::Bitboard    occupancy   = pos.occupancy();
    board::Bitboard    our_occ     = color == types::Color::WHITE ? pos._white_occupancy() : pos._black_occupancy();

    // asserts
    assert(from < types::Square::h8 && from >= types::Square::a1);
    assert(to < types::Square::h8 && to >= types::Square::a1);

    // if the move has the valid format
    if (!move.is_ok())
    {
        return false;
    }

    // if the destination is occupied by us
    if (our_occ.is_bitset(to) == true)
    {
        return false;
    }

    // no one moves twice
    if (color != move_color)
    {
        return false;
    }

    // no void moves
    if (pos.pieceOn(from) == types::PieceType::NOPE)
    {
        return false;
    }

    // can't move to the same square you're in
    if (from == to)
    {
        return false;
    }

    // can't move a void piece
    if (moved_piece == types::PieceType::NOPE)
    {
        return false;
    }

    // can't casle with no king
    if ((flag == types::MoveType::KSCASTLE || flag == types::MoveType::QSCASTLE)
        && moved_piece != types::PieceType::KING)
    {
        return false;
    }

    // can't en passant with no pawn
    if (flag == types::MoveType::EN_PASSANT && moved_piece != types::PieceType::PAWN)
    {
        return false;
    }

    // the destination sqaure in en passant should always be empty
    if (flag == types::MoveType::EN_PASSANT && pos.pieceOn(to) != types::PieceType::NOPE)
    {
        return false;
    }

    // can't capture on an empty square
    if (flag == types::MoveType::QUIET && pos.pieceOn(to) == types::PieceType::NOPE)
    {
        return false;
    }

    // can't promote with no pawn
    if ((flag == types::MoveType::PROMOTION || flag == types::MoveType::EN_PASSANT)
        && moved_piece != types::PieceType::PAWN)
    {
        return false;
    }

    const uint8_t NORTH = (color == types::Color::WHITE) ? -8 : 8;  // set north for simplicity

    switch (moved_piece)
    {
    case types::PieceType::PAWN : {
        // generate all the possible moves
        std::vector<types::Move> moves = pawn_moves(from, color, pos);
        // the given moves should always exist in this list
        if (std::find(moves.begin(), moves.end(), to) == moves.end())
        {
            return false;
        }

        // additional checks for robustness
        if (flag == types::MoveType::EN_PASSANT)
        {
            if (color == types::Color::WHITE && to != types::Square(static_cast<int>(from) + NORTH))
            {
                return false;
            }

            if (color == types::Color::BLACK && to != types::Square(static_cast<int>(from) - NORTH))
            {
                return false;
            }
        }
        else if (flag == types::MoveType::CAPTURE && pos.pieceOn(to) == types::PieceType::NOPE)
        {
            return false;
        }
        else if (flag == types::MoveType::PROMOTION)
        {
            if (color == types::Color::WHITE ? types::ranks[(int) (from)] != 6 : types::ranks[(int) (from)] != 1)
            {
                return false;
            }

            if (color == types::Color::WHITE ? types::ranks[(int) (to)] != 7 : types::ranks[(int) (to)] != 1)
            {
                return false;
            }
        }
        else
        {
            if (color == types::Color::WHITE && types::ranks[(int) (from)] >= 1 && types::ranks[(int) (from)] < 6
                && types::ranks[(int) (from)] >= types::ranks[(int) (to)])
            {
                return false;
            }

            if (color == types::Color::BLACK && types::ranks[(int) (from)] >= 6 && types::ranks[(int) (from)] < 1
                && types::ranks[(int) (from)] <= types::ranks[(int) (to)])
            {
                return false;
            }
        }
    }
    break;

    // almost the same with the rest of the pieces but with some slight
    // differences based on the move type
    case types::PieceType::KNIGHT : {
        std::vector<types::Move> moves = knight_moves(from, color, pos);

        if (std::find(moves.begin(), moves.end(), to) == moves.end())
        {
            return false;
        }
    }
    break;

    case types::PieceType::BISHOP : {
        std::vector<types::Move> moves = bishop_moves(from, color, pos);

        if (std::find(moves.begin(), moves.end(), to) == moves.end())
        {
            return false;
        }
    }
    break;

    case types::PieceType::ROOK : {
        std::vector<types::Move> moves = rook_moves(from, color, pos);

        if (std::find(moves.begin(), moves.end(), to) == moves.end())
        {
            return false;
        }
    }
    break;

    case types::PieceType::QUEEN : {
        std::vector<types::Move> moves = queen_moves(from, color, pos);

        if (std::find(moves.begin(), moves.end(), to) == moves.end())
        {
            return false;
        }
    }
    break;

    case types::PieceType::KING : {
        if (pos.isAttacked(to, color))
        {
            return false;
        }

        if (flag == types::MoveType::KSCASTLE)
        {
            if (color == types::Color::WHITE)
            {
                if (occupancy.is_bitset(types::Square::f1) || occupancy.is_bitset(types::Square::g1)
                    || pos.isAttacked(types::Square::f1, types::Color::WHITE)
                    || pos.isAttacked(types::Square::g1, types::Color::WHITE))
                {
                    return false;
                }
            }
            else
            {
                if (occupancy.is_bitset(types::Square::f8) || occupancy.is_bitset(types::Square::g8)
                    || pos.isAttacked(types::Square::f8, types::Color::BLACK)
                    || pos.isAttacked(types::Square::g8, types::Color::BLACK))
                {
                    return false;
                }
            }
        }
        else if (flag == types::MoveType::QSCASTLE)
        {
            if (color == types::Color::WHITE)
            {
                if (occupancy.is_bitset(types::Square::d1) || occupancy.is_bitset(types::Square::c1)
                    || occupancy.is_bitset(types::Square::b1) || pos.isAttacked(types::Square::d1, types::Color::WHITE)
                    || pos.isAttacked(types::Square::c1, types::Color::WHITE)
                    || pos.isAttacked(types::Square::b1, types::Color::WHITE))
                {
                    return false;
                }
            }
            else
            {
                if (occupancy.is_bitset(types::Square::d8) || occupancy.is_bitset(types::Square::c8)
                    || occupancy.is_bitset(types::Square::b8) || pos.isAttacked(types::Square::d8, types::Color::BLACK)
                    || pos.isAttacked(types::Square::c8, types::Color::BLACK)
                    || pos.isAttacked(types::Square::b8, types::Color::BLACK))
                {
                    return false;
                }
            }
        }

        std::vector<types::Move> moves = king_moves(from, color, pos);

        if (std::find(moves.begin(), moves.end(), to) == moves.end())
        {
            return false;
        }
    }
    break;

    default :
        break;
    }

    return true;  // all right
}

// gets the bitboard of the opponent pieces attacked by the given piece
board::Bitboard get_piece_attacks(const position::Position& pos, types::Square square) {
    types::PieceType piece = pos.pieceOn(square);

    if (piece == types::PieceType::NOPE)
    {
        return board::Bitboard(0ULL);
    }

    // typical data
    types::Color             side   = pos.getColor(square);
    board::Bitboard          occ    = side == types::Color::WHITE ? pos._black_occupancy() : pos._white_occupancy();
    board::Bitboard          result = board::Bitboard(0ULL);
    std::vector<types::Move> moves  = rook_moves(square, side, pos);

    // if no moves for the given piece then return a 0 bitboard
    if (moves.empty())
    {
        return board::Bitboard(0ULL);
    }

    // fetch the result in brute force
    for (const auto m : moves)
    {
        if (occ.is_bitset(m.getTo()))
        {
            result.set_bit(m.getTo());
        }
    }

    return result;
}

// is a given move strictly legal
bool isLegal(const position::Position& pos, const types::Move& move) {
    // metadata
    types::Color     color = pos.getSide();
    types::MoveType  flag  = types::MoveType(move.getFlags());
    types::Square    from  = types::Square(move.getFrom());
    types::Square    to    = types::Square(move.getTo());
    types::PieceType piece = pos.pieceOn(from);

    // asserts
    assert(color == pos.getColor(from));
    assert(from < types::Square::h8 && from >= types::Square::a1);
    assert(to < types::Square::h8 && to >= types::Square::a1);

    // all seen before
    if (flag != types::MoveType::CAPTURE && pos.pieceOn(to) != types::PieceType::NOPE)
    {
        return false;
    }

    if (piece == types::PieceType::KING)
    {
        if (flag == types::MoveType::KSCASTLE)
        {
            return !pos.isAttacked(color == types::Color::WHITE ? types::Square::f1 : types::Square::f8, color)
                && !pos.isAttacked(color == types::Color::WHITE ? types::Square::g1 : types::Square::g8, color);
        }

        if (flag == types::MoveType::QSCASTLE)
        {
            return !pos.isAttacked(color == types::Color::WHITE ? types::Square::b1 : types::Square::b8, color)
                && !pos.isAttacked(color == types::Color::WHITE ? types::Square::c1 : types::Square::c8, color)
                && !pos.isAttacked(color == types::Color::WHITE ? types::Square::d1 : types::Square::d8, color);
        }

        if (pos.isAttacked(to, color))
        {
            return false;
        }
    }

    if (pos.isPinned(from) != types::Square::NONE)
    {
        return false;
    }

    return true;
}


}  // namespace movegen
}  // namespace Shahrazad