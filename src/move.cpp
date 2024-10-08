#include "move.h"
#include "nnue.h"
#include "position.h"

#include <cassert>
#include <unordered_set>


// make a move in a given position
void do_move(Position& pos, const Move& move) {
    // data about the given move and position
    uint8_t   from   = move.getFrom();
    uint8_t   to     = move.getTo();
    uint8_t   _flag  = move.getFlags();
    uint8_t   _color = pos.getColor(from);
    pieceType piece  = pos.pieceOn(from);

    // asserts
    assert(from < 64 && from >= 0);
    assert(to < 64 && to >= 0);
    assert(_color == pos.current_side);

    // store the current position as the previous position of the next one after commiting a move
    Position previous_position = pos;
    pos.pieces[from]           = NOPE;
    pos.pieces[to]             = piece;

    // increment the fifty moves counter if the move is quiet
    if (piece == PAWN || move.isCapture())
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
    if (piece == KING)
    {
        pos.castle_perm[0] = false;
        pos.castle_perm[1] = false;
    }

    // make move on the bitboard of that piece type map
    switch (piece)
    {
    case KING :
        _color == WHITE ? pos.white_king.move_bit(from, to) : pos.black_king.move_bit(from, to);
        break;
    case PAWN :
        _color == WHITE ? pos.white_pawns.move_bit(from, to) : pos.black_pawns.move_bit(from, to);
        break;
    case KNIGHT :
        _color == WHITE ? pos.white_knights.move_bit(from, to)
                        : pos.black_knights.move_bit(from, to);
        break;
    case BISHOP :
        _color == WHITE ? pos.white_bishops.move_bit(from, to)
                        : pos.black_bishops.move_bit(from, to);
        break;
    case ROOK :
        _color == WHITE ? pos.white_rooks.move_bit(from, to) : pos.black_rooks.move_bit(from, to);
        break;
    case QUEEN :
        _color == WHITE ? pos.white_queens.move_bit(from, to) : pos.black_queens.move_bit(from, to);
        break;
    default :
        break;
    }

    uint8_t pers[2] = {WHITE, BLACK};

    // update or refresh the NNUE accumalator caches
    for (uint8_t perspective : pers)
    {
        if (pos.needs_refresh[perspective])
        {
            nnue.refresh_accumulator(nnue.l_0, get_active_features(pos, perspective), perspective);
        }
        else
        {
            nnue.update_accumulator(nnue.l_0, get_removed_features(pos, *pos.prev, perspective),
                                    get_added_features(pos, *pos.prev, perspective), perspective);
        }
    }
}

// generate pawn moves
std::vector<uint8_t> pawn_moves(const uint8_t square, uint8_t color, const Position& pos) {
    // asserts
    assert(square < 64 && square >= 0);
    assert(pos.pieceOn(square) == PAWN);

    std::vector<uint8_t> moves;

    // get the occupancy of the current side and the opponent's side
    Bitboard our_occupancy = color == WHITE ? pos._white_occupancy() : pos._black_occupancy();
    Bitboard opp_occupancy = color == BLACK ? pos._black_occupancy() : pos._white_occupancy();

    // if the requested square actualy has a pawn on it
    if (our_occupancy.is_bitset(square))
    {
        // diagonal left pos
        uint8_t left = square + 7;
        // forward pos
        uint8_t up = square + 9;
        // diagonal right pos
        uint8_t right = square + 9;

        // check occupancies then push back the move to 'moves'
        if (left % 8 != 0 && opp_occupancy.is_bitset(left))
        {
            moves.push_back(left);
        }

        if (left % 8 != 7 && opp_occupancy.is_bitset(right))
        {
            moves.push_back(right);
        }

        if (up < 64 && !our_occupancy.is_bitset(up) && !opp_occupancy.is_bitset(up))
        {
            moves.push_back(right);
        }
    }

    return moves;
}

// generate rook moves
std::vector<uint8_t> rook_moves(const uint8_t square, uint8_t color, const Position& pos) {
    assert(square < 64 && square >= 0);

    std::vector<uint8_t> moves;

    // get occupancies
    Bitboard our_occupancy = color == WHITE ? pos._white_occupancy() : pos._black_occupancy();
    Bitboard opp_occupancy = color == BLACK ? pos._black_occupancy() : pos._white_occupancy();

    // generate forward moves
    for (uint8_t up = square + 8; up < 64; up += 8)
    {
        if (our_occupancy.is_bitset(up))
        {
            break;
        }

        if (opp_occupancy.is_bitset(up))
        {
            moves.push_back(up);
            break;
        }

        moves.push_back(up);
    }

    // generate backward moves
    for (uint8_t down = square - 8; down >= 0; down -= 8)
    {
        if (our_occupancy.is_bitset(down))
        {
            break;
        }

        if (opp_occupancy.is_bitset(down))
        {
            moves.push_back(down);
            break;
        }

        moves.push_back(down);
    }

    // generate moves to the left
    for (uint8_t left = square - 1; left >= 0 && (left + 1) % 8 != 0; left--)
    {
        if (our_occupancy.is_bitset(left))
        {
            break;
        }

        if (opp_occupancy.is_bitset(left))
        {
            moves.push_back(left);
            break;
        }

        moves.push_back(left);
    }

    // generate moves to the right
    for (uint8_t right = square + 1; right < 64 && right % 8 != 0; right++)
    {
        if (our_occupancy.is_bitset(right))
        {
            break;
        }

        if (opp_occupancy.is_bitset(right))
        {
            moves.push_back(right);
            break;
        }

        moves.push_back(right);
    }

    return moves;
}

// generate bishop moves
std::vector<uint8_t> bishop_moves(const uint8_t square, uint8_t color, const Position& pos) {
    assert(square < 64 && square >= 0);

    std::vector<uint8_t> moves;

    // get occupancies
    Bitboard our_occupancy = color == WHITE ? pos._white_occupancy() : pos._black_occupancy();
    Bitboard opp_occupancy = color == BLACK ? pos._black_occupancy() : pos._white_occupancy();

    // generate moves in the top right side
    for (uint8_t top_right = square + 9; top_right < 64 && (top_right % 8) != 0; top_right += 9)
    {
        if (our_occupancy.is_bitset(top_right))
        {
            break;
        }

        if (opp_occupancy.is_bitset(top_right))
        {
            moves.push_back(top_right);
            break;
        }

        moves.push_back(top_right);
    }

    // generate moves in the bottom left side
    for (uint8_t bottom_left = square - 9; bottom_left >= 0 && ((bottom_left + 1) % 8) != 0;
         bottom_left -= 9)
    {
        if (our_occupancy.is_bitset(bottom_left))
        {
            break;
        }

        if (opp_occupancy.is_bitset(bottom_left))
        {
            moves.push_back(bottom_left);
            break;
        }

        moves.push_back(bottom_left);
    }

    // generate moves in the top left side
    for (uint8_t top_left = square + 7; top_left < 64 && ((top_left + 1) % 8) != 0; top_left += 7)
    {

        if (our_occupancy.is_bitset(top_left))
        {
            break;
        }

        if (opp_occupancy.is_bitset(top_left))
        {
            moves.push_back(top_left);
            break;
        }

        moves.push_back(top_left);
    }

    // generate moves in the bottom right side
    for (uint8_t bottom_right = square - 7; bottom_right >= 0 && (bottom_right % 8) != 0;
         bottom_right -= 7)
    {
        if (our_occupancy.is_bitset(bottom_right))
        {
            break;
        }

        if (our_occupancy.is_bitset(bottom_right))
        {
            moves.push_back(bottom_right);
            break;
        }

        moves.push_back(bottom_right);
    }

    return moves;
}

// generate queen moves queen moves are actualy a combination of rook
// moves and bishop moves, so all that we're doing here is
// merging both from the previously implemented methods
std::vector<uint8_t> queen_moves(const uint8_t square, const uint8_t color, const Position& pos) {
    assert(pos.pieceOn(square) == QUEEN);

    std::vector<uint8_t> moves;
    std::vector<uint8_t> b = bishop_moves(square, color, pos);
    std::vector<uint8_t> r = bishop_moves(square, color, pos);

    moves.reserve(b.size() + r.size());

    for (uint8_t i = 0, n = b.size(); i < n; i++)
    {
        moves[i] = b[i];
    }

    for (uint8_t i = b.size(), n = b.size() + r.size(), k = 0; i < n, k < r.size(); i++)
    {
        moves[i] = r[k++];
    }

    return moves;
}

// generate knight moves knight moves are tricky to explain to
// even humans, but we can just use a constanqwdt map relative to the
//  given piece position and ignore the pieces which we jumped over
std::vector<uint8_t> knight_moves(const uint8_t square, uint8_t color, const Position& pos) {
    assert(square < 64 && square >= 0);
    assert(pos.pieceOn(square) == KNIGHT);

    std::vector<uint8_t> moves;

    Bitboard our_occupancy = color == WHITE ? pos._white_occupancy() : pos._black_occupancy();
    Bitboard opp_occupancy = color == BLACK ? pos._black_occupancy() : pos._white_occupancy();

    uint8_t knight_moves[8][2] = {{-2, 1}, {-2, -1}, {2, 1}, {2, -1},
                                  {-1, 2}, {-1, -2}, {1, 2}, {1, -2}};

    uint8_t start_file = square % 8;
    uint8_t start_rank = square / 8;

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

            moves.push_back(new_pos);
        }
    }

    return moves;
}

// generate king moves
std::vector<uint8_t> king_moves(const uint8_t square, uint8_t color, const Position& pos) {
    // asserts
    assert(square < 64 && square >= 0);
    assert(pos.pieceOn(square) == KING);

    std::vector<uint8_t> moves;

    // get occupancies
    Bitboard our_occupancy = color == WHITE ? pos._white_occupancy() : pos._black_occupancy();
    Bitboard opp_occupancy = color == BLACK ? pos._black_occupancy() : pos._white_occupancy();

    // moves of 1 square in every direction
    uint8_t bottom_right = square - 7;
    uint8_t top_left     = square + 7;
    uint8_t bottom_left  = square - 9;
    uint8_t top_right    = square + 9;
    uint8_t up           = square + 8;
    uint8_t down         = square - 8;
    uint8_t left         = square - 1;
    uint8_t right        = square + 1;

    // checking boundaries and occupancies for validation

    if (bottom_left >= 0 && ((bottom_left + 1) % 8) != 0)
    {
        moves.push_back(bottom_left);
    }

    if (bottom_right >= 0 && (bottom_right % 8) != 0)
    {
        moves.push_back(bottom_right);
    }

    if (top_left < 64 && (top_left + 1) % 8 != 0)
    {
        moves.push_back(top_left);
    }

    if (top_right < 64 && (top_right + 1) % 8 != 0)
    {
        moves.push_back(top_right);
    }

    if (up < 64)
    {
        moves.push_back(up);
    }

    if (down >= 0)
    {
        moves.push_back(down);
    }

    if (left >= 0 && (left + 1) % 8 != 0)
    {
        moves.push_back(left);
    }

    if (right < 64 && right % 8 != 0)
    {
        moves.push_back(right);
    }

    return moves;
}

// generate all the possible pseudo moves of a side in a board
std::vector<Move> generate_all_moves(const Position& pos, uint8_t color) {
    assert(color == WHITE || color == BLACK);

    // get occupancies
    Bitboard occupancy = color == WHITE ? pos._white_occupancy() : pos._black_occupancy();

    std::vector<Move> total;  // ret container

    // loop for each square on the board
    for (uint8_t square = 0; square < 64; square++)
    {
        // if there is a piece on that square
        if (occupancy.is_bitset(square))
        {
            pieceType piece_type = pos.pieceOn(square);

            // just in case
            if (piece_type == NOPE)
            {
                continue;
            }

            // possible destinations container
            std::vector<uint8_t> to_squares;

            // we don't know what's the actual piece type and we don't wanna know
            switch (piece_type)
            {
            case PAWN :
                to_squares = pawn_moves(square, color, pos);
                break;
            case KNIGHT :
                to_squares = king_moves(square, color, pos);
                break;
            case BISHOP :
                to_squares = bishop_moves(square, color, pos);
                break;
            case ROOK :
                to_squares = rook_moves(square, color, pos);
                break;
            case QUEEN :
                to_squares = queen_moves(square, color, pos);
                break;
            case KING :
                to_squares = king_moves(square, color, pos);
                break;

            default :
                break;
            }

            // if no moves have been found for the current piece
            if (to_squares.empty())
            {
                continue;
            }

            std::vector<Move> moves;  // temp

            // add the moves to the list
            for (uint8_t i = 0, n = to_squares.size(); i < n; i++)
            {
                Move m = Move(square, to_squares[i], Quiet);
                moves.push_back(m);
            }

            // add the list to the return vector
            for (Move m : moves)
            {
                total.push_back(m);
            }
        }
    }

    return total;
}

// gets the squares on which the given piece type resides
// useful when we don't know which squares we're dealing with
std::vector<uint8_t> piece_squares(pieceType piece_type, const Position& pos) {
    if (piece_type == NOPE)
    {
        return {};
    }

    // assert a valid piece type
    assert(piece_type <= NOPE && piece_type >= KING);

    std::vector<uint8_t> ans;

    // look for the piece and get it's square if found
    for (uint8_t sq = 0; sq < 64; sq++)
    {
        if (pos.pieces[sq] == piece_type)
        {
            ans.push_back(sq);
        }
    }

    return ans;
}

// sets a bitboard of the attacked squares in a
// given position by the opposite color
void setAttackedSquares(Position& pos) {
    // position data
    uint8_t                    color        = pos.getSide();
    uint8_t                    oppent_color = color == WHITE ? BLACK : WHITE;
    std::unordered_set<int8_t> total_access;  // ret container

    // lambda kinda function that adds a move to the list of accessed squares
    auto add_moves = [&total_access](const std::vector<uint8_t>& moves) {
        for (uint8_t move : moves)
        {
            total_access.insert(move);
        }
    };

    // also for each square in the board
    for (const pieceType piece : pos.pieces)
    {
        std::vector<uint8_t> piece_sq = piece_squares(piece, pos);

        if (piece_sq.empty())
            continue;

        for (const uint8_t sq : piece_sq)
        {
            std::vector<uint8_t> access;

            switch (piece)
            {
            case PAWN :
                access = pawn_moves(sq, color, pos);
                break;
            case KNIGHT :
                access = knight_moves(sq, color, pos);
                break;
            case BISHOP :
                access = bishop_moves(sq, color, pos);
                break;
            case ROOK :
                access = bishop_moves(sq, color, pos);
                break;
            case QUEEN :
                access = queen_moves(sq, color, pos);
                break;
            case KING :
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
        color == WHITE ? pos.attacked_black[sq] = (total_access.find(sq) != total_access.end())
                       : pos.attacked_white[sq] = (total_access.find(sq) != total_access.end());
    }
}

// is the given move a capture
bool is_tactical(const Move& _move) {
    uint8_t _flag = _move.getFlags();

    if (_flag == CAPTURE)
    {
        return true;
    }

    return false;
}

// pseudo legality checks inspired by stockfish
bool isPseudoLegal(const Position& pos, const Move& move) {
    // data about the position and the given move
    uint8_t       from        = move.getFrom();
    uint8_t       to          = move.getTo();
    uint8_t       _flag       = move.getFlags();
    uint8_t       _move_color = pos.getColor(from);
    const uint8_t color       = pos.getSide();
    pieceType     moved_piece = pos.pieceOn(from);
    Bitboard      occupancy   = pos.occupancy();
    Bitboard      our_occ     = color == WHITE ? pos._white_occupancy() : pos._black_occupancy();

    // asserts
    assert(from < 64 && from >= 0);
    assert(to < 64 && to >= 0);

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
    if (color != _move_color)
    {
        return false;
    }

    // no void moves
    if (pos.pieceOn(from) == NOPE)
    {
        return false;
    }

    // can't move to the same square you're in
    if (from == to)
    {
        return false;
    }

    // can't move a void piece
    if (moved_piece == NOPE)
    {
        return false;
    }

    // can't casle with no king
    if ((_flag == KSCastle || _flag == QSCastle) && moved_piece != KING)
    {
        return false;
    }

    // can't en passant with no pawn
    if (_flag == EN_PASSANT && moved_piece != PAWN)
    {
        return false;
    }

    // the destination sqaure in en passant should always be empty
    if (_flag == EN_PASSANT && pos.pieceOn(to) != NOPE)
    {
        return false;
    }

    // can't capture on an empty square
    if (_flag == Capture && pos.pieceOn(to) == NOPE)
    {
        return false;
    }

    // can't promote with no pawn
    if ((_flag == Promo || _flag == enPassant) && moved_piece != PAWN)
    {
        return false;
    }

    const uint8_t NORTH = (color == WHITE) ? -8 : 8;  // set north fot simplicity

    switch (moved_piece)
    {
    case PAWN : {
        // generate all the possible moves
        std::vector<uint8_t> moves = pawn_moves(from, color, pos);
        // the given moves should always exist in this list
        if (std::find(moves.begin(), moves.end(), to) == moves.end())
        {
            return false;
        }

        // additional checks for robustness
        if (_flag == EN_PASSANT)
        {
            if (color == WHITE && to != (from + NORTH))
            {
                return false;
            }

            if (color == BLACK && to != (from - NORTH))
            {
                return false;
            }
        }
        else if (_flag == Capture && pos.pieceOn(to) == NOPE)
        {
            return false;
        }
        else if (_flag == Promo)
        {
            if (color == WHITE ? ranks[from] != 6 : ranks[from] != 1)
            {
                return false;
            }

            if (color == WHITE ? ranks[to] != 7 : ranks[to] != 1)
            {
                return false;
            }
        }
        else
        {
            if (color == WHITE && ranks[from] >= 1 && ranks[from] < 6 && ranks[from] >= ranks[to])
            {
                return false;
            }

            if (color == BLACK && ranks[from] >= 6 && ranks[from] < 1 && ranks[from] <= ranks[to])
            {
                return false;
            }
        }
    }
    break;

    // almost the same with the rest of the pieces but with some slight
    // differences based on the move type
    case KNIGHT : {
        std::vector<uint8_t> moves = knight_moves(from, color, pos);

        if (std::find(moves.begin(), moves.end(), to) == moves.end())
        {
            return false;
        }
    }
    break;

    case BISHOP : {
        std::vector<uint8_t> moves = bishop_moves(from, color, pos);

        if (std::find(moves.begin(), moves.end(), to) == moves.end())
        {
            return false;
        }
    }
    break;

    case ROOK : {
        std::vector<uint8_t> moves = rook_moves(from, color, pos);

        if (std::find(moves.begin(), moves.end(), to) == moves.end())
        {
            return false;
        }
    }
    break;

    case QUEEN : {
        std::vector<uint8_t> moves = queen_moves(from, color, pos);

        if (std::find(moves.begin(), moves.end(), to) == moves.end())
        {
            return false;
        }
    }
    break;

    case KING : {
        if (pos.isAttacked(to, color))
        {
            return false;
        }

        if (_flag == KSCastle)
        {
            if (color == WHITE)
            {
                if (occupancy.is_bitset(f1) || occupancy.is_bitset(g1) || pos.isAttacked(f1, WHITE)
                    || pos.isAttacked(g1, WHITE))
                {

                    return false;
                }
            }
            else
            {
                if (occupancy.is_bitset(f8) || occupancy.is_bitset(g8) || pos.isAttacked(f8, BLACK)
                    || pos.isAttacked(g8, BLACK))
                {

                    return false;
                }
            }
        }
        else if (_flag == QSCastle)
        {
            if (color == WHITE)
            {
                if (occupancy.is_bitset(d1) || occupancy.is_bitset(c1) || occupancy.is_bitset(b1)
                    || pos.isAttacked(d1, WHITE) || pos.isAttacked(c1, WHITE)
                    || pos.isAttacked(b1, WHITE))
                {

                    return false;
                }
            }
            else
            {
                if (occupancy.is_bitset(d8) || occupancy.is_bitset(c8) || occupancy.is_bitset(b8)
                    || pos.isAttacked(d8, BLACK) || pos.isAttacked(c8, BLACK)
                    || pos.isAttacked(b8, BLACK))
                {

                    return false;
                }
            }
        }

        std::vector<uint8_t> moves = king_moves(from, color, pos);

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
Bitboard get_piece_attacks(const Position& pos, uint8_t square) {
    pieceType piece = pos.pieceOn(square);

    if (piece == NOPE)
    {
        return Bitboard(0ULL);
    }

    // typical data
    uint8_t              side   = pos.getColor(square);
    Bitboard             occ    = side == WHITE ? pos._black_occupancy() : pos._white_occupancy();
    Bitboard             result = Bitboard(0ULL);
    std::vector<uint8_t> moves  = rook_moves(square, side, pos);

    // if no moves for the given piece then return a 0 bitboard
    if (moves.empty())
    {
        return Bitboard(0ULL);
    }

    // fetch the result in brute force
    for (const int8_t m : moves)
    {
        if (occ.is_bitset(m))
        {
            result.set_bit(m);
        }
    }

    return result;
}

// is a given move strictly legal
bool isLegal(const Position& pos, const Move& move) {
    // metadata
    uint8_t   color = pos.getSide();
    uint8_t   _flag = move.getFlags();
    uint8_t   from  = move.getFrom();
    uint8_t   to    = move.getTo();
    pieceType piece = pos.pieceOn(from);

    // asserts
    assert(color == pos.getColor(from));
    assert(from < 64 && from >= 0);
    assert(to < 64 && to >= 0);

    // all seen before
    if (_flag != Capture && pos.pieceOn(to) != NOPE)
    {
        return false;
    }

    if (piece == KING)
    {
        if (_flag == KSCastle)
        {
            return !pos.isAttacked(color == WHITE ? f1 : f8, color)
                && !pos.isAttacked(color == WHITE ? g1 : g8, color);
        }

        if (_flag == QSCastle)
        {
            return !pos.isAttacked(color == WHITE ? b1 : b8, color)
                && !pos.isAttacked(color == WHITE ? c1 : c8, color)
                && !pos.isAttacked(color == WHITE ? d1 : d8, color);
        }

        if (pos.isAttacked(to, color))
        {
            return false;
        }
    }

    if (pos.isPinned(from))
    {
        return false;
    }

    return true;
}
