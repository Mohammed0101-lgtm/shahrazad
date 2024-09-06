#include "move.h"
#include "position.h"
#include "nnue.h"
#include <unordered_set>
#include <cassert>

void do_move(Position& pos, const Move& move) {
    unsigned int from   = move.getFrom();
    unsigned int to     = move.getTo();
    unsigned int _flag  = move.getFlags();
    unsigned int _color = pos.getColor(from);
    pieceType piece     = pos.pieceOn(from);

    assert(from < 64 && from >= 0);
    assert(to < 64 && to >= 0);
    assert(_color == pos.current_side);

    Position previous_position = pos;
    pos.pieces[from] = NOPE;
    pos.pieces[to] = piece;

    if (piece == PAWN || move.isCapture()) 
        pos.fifty_moves_counter = 0;
    else 
        pos.fifty_moves_counter++;

    pos.switch_side();
    pos.played_positions.emplace_back(pos.position_key);
    pos.stacked_his++;

    if (piece == KING) {
        pos.castle_perm[0] = false;
        pos.castle_perm[1] = false;
    }

    switch (piece) {
        case KING:
            _color == WHITE ? pos.white_king.move_bit(from, to) : pos.black_king.move_bit(from, to);
            break;
        case PAWN:
            _color == WHITE ? pos.white_pawns.move_bit(from, to) : pos.black_pawns.move_bit(from, to);
            break;
        case KNIGHT:
            _color == WHITE ? pos.white_knights.move_bit(from, to) : pos.black_knights.move_bit(from, to);
            break;
        case BISHOP:
            _color == WHITE ? pos.white_bishops.move_bit(from, to) : pos.black_bishops.move_bit(from, to);
            break;
        case ROOK:
            _color == WHITE ? pos.white_rooks.move_bit(from, to) : pos.black_rooks.move_bit(from, to);
            break;
        case QUEEN:
            _color == WHITE ? pos.white_queens.move_bit(from, to) : pos.black_queens.move_bit(from, to);
            break;
        default : 
            break;
    }

    for (int perspective : { WHITE, BLACK }) {
        if (pos.needs_refresh[perspective]) {
            nnue.refresh_accumulator(
                                        nnue.l_0,
                                        get_active_features(pos, perspective),
                                        perspective
                                    );
        } else {
            nnue.update_accumulator(
                                        nnue.l_0,
                                        get_removed_features(pos, *pos.prev, perspective),
                                        get_added_features(pos, *pos.prev, perspective),
                                        perspective 
                                    );
        }
    }
}


std::vector<int8_t> pawn_moves(const int square, int color, const Position& pos) {
    assert(square < 64 && square >= 0);
    assert(pos.pieceOn(square) == PAWN);

    std::vector<int8_t> moves;

    Bitboard our_occupancy = color == WHITE ? pos._white_occupancy() : pos._black_occupancy();
    Bitboard opp_occupancy = color == BLACK ? pos._black_occupancy() : pos._white_occupancy();

    if (our_occupancy.is_bitset(square)) {
        int left  = square + 7;
        int up    = square + 9;
        int right = square + 9;

        if (left % 8 != 0 && opp_occupancy.is_bitset(left)) 
            moves.push_back(left);

        if (left % 8 != 7 && opp_occupancy.is_bitset(right)) 
            moves.push_back(right);

        if (up < 64 && !our_occupancy.is_bitset(up) && !opp_occupancy.is_bitset(up)) 
            moves.push_back(right);
    }

    return moves;
}


std::vector<int8_t> rook_moves(const int square, int color, const Position& pos) {
    assert(square < 64 && square >= 0);

    std::vector<int8_t> moves;

    Bitboard our_occupancy = color == WHITE ? pos._white_occupancy() : pos._black_occupancy();
    Bitboard opp_occupancy = color == BLACK ? pos._black_occupancy() : pos._white_occupancy();

    for (int up = square + 8; up < 64; up += 8) {
        if (our_occupancy.is_bitset(up)) 
            break;

        if (opp_occupancy.is_bitset(up)) {
            moves.push_back(up);
            break;
        }

        moves.push_back(up);
    }

    for (int down = square - 8; down >= 0; down -= 8) {
        if (our_occupancy.is_bitset(down)) 
            break;

        if (opp_occupancy.is_bitset(down)) {
            moves.push_back(down);
            break;
        }

        moves.push_back(down);
    }

    for (int left = square - 1; left >= 0 && (left + 1) % 8 != 0; left--) {
        if (our_occupancy.is_bitset(left)) 
            break;

        if (opp_occupancy.is_bitset(left)) {
            moves.push_back(left);
            break;
        }

        moves.push_back(left);
    }

    for (int right = square + 1; right < 64 && right % 8 != 0; right++) {
        if (our_occupancy.is_bitset(right)) 
            break;

        if (opp_occupancy.is_bitset(right)) {
            moves.push_back(right);
            break;
        }

        moves.push_back(right);
    }

    return moves;
}


std::vector<int8_t> bishop_moves(const int square, int color, const Position& pos) {
    assert(square < 64 && square >= 0);

    std::vector<int8_t> moves;

    Bitboard our_occupancy = color == WHITE ? pos._white_occupancy() : pos._black_occupancy();
    Bitboard opp_occupancy = color == BLACK ? pos._black_occupancy() : pos._white_occupancy();

    for (int top_right = square + 9; top_right < 64
         &&
        (top_right % 8) != 0; top_right += 9) {
        if (our_occupancy.is_bitset(top_right)) 
            break;

        if (opp_occupancy.is_bitset(top_right)) {
            moves.push_back(top_right);
            break;
        }

        moves.push_back(top_right);
    }

    for (int bottom_left = square - 9; bottom_left >= 0
         &&
        ((bottom_left + 1) % 8) != 0; bottom_left -= 9) {

        if (our_occupancy.is_bitset(bottom_left)) 
            break;

        if (opp_occupancy.is_bitset(bottom_left)) {
            moves.push_back(bottom_left);
            break;
        }

        moves.push_back(bottom_left);
    }

    for (int top_left = square + 7; top_left < 64
        &&
        ((top_left + 1) % 8) != 0; top_left += 7) {

        if (our_occupancy.is_bitset(top_left)) 
            break;
    
        if (opp_occupancy.is_bitset(top_left)) {
            moves.push_back(top_left);
            break;
        }

        moves.push_back(top_left);
    }

    for (int bottom_right = square - 7; bottom_right >= 0
         &&
        (bottom_right % 8) != 0; bottom_right -= 7) {

        if (our_occupancy.is_bitset(bottom_right)) 
            break;

        if (our_occupancy.is_bitset(bottom_right)) {
            moves.push_back(bottom_right);
            break;
        }

        moves.push_back(bottom_right);
    }

    return moves;
}


std::vector<int8_t> queen_moves(const int square, const int color, const Position& pos) {
    assert(pos.pieceOn(square) == QUEEN);

    std::vector<int8_t> moves;

    std::vector<int8_t> b = bishop_moves(square, color, pos);
    std::vector<int8_t> r = bishop_moves(square, color, pos);

    for (int move : b)
        moves.push_back(move); 

    for (int move : r) 
        moves.push_back(move); 

    return moves;
}


std::vector<int8_t> knight_moves(const int square, int color, const Position& pos) {
    assert(square < 64 && square >= 0);
    assert(pos.pieceOn(square) == KNIGHT);

    std::vector<int8_t> moves;

    Bitboard our_occupancy = color == WHITE ? pos._white_occupancy() : pos._black_occupancy();
    Bitboard opp_occupancy = color == BLACK ? pos._black_occupancy() : pos._white_occupancy();

    int knight_moves[8][2] = {
        { -2 , 1 } , { -2 , -1 } , { 2 , 1 } , { 2 , -1 },
        { -1 , 2 } , { -1 , -2 } , { 1 , 2 } , { 1 , -2 }
    };

    int start_file = square % 8;
    int start_rank = square / 8;

    for (const auto& move : knight_moves) {
        int new_file = start_file + move[0];
        int new_rank = start_rank + move[1];

        if (new_file >= 0 && new_rank < 8 && new_rank >= 0 && new_rank < 8) {
            int new_pos = new_rank * 8 + new_file;

            if (our_occupancy.is_bitset(new_pos)) 
                break;

            moves.push_back(new_pos);
        }
    }

    return moves;
}


std::vector<int8_t> king_moves(const int square, int color, const Position& pos) {
    assert(square < 64 && square >= 0 );
    assert(pos.pieceOn(square) == KING);

    std::vector<int8_t> moves;

    Bitboard our_occupancy = color == WHITE ? pos._white_occupancy() : pos._black_occupancy();
    Bitboard opp_occupancy = color == BLACK ? pos._black_occupancy() : pos._white_occupancy();

    int bottom_right = square - 7;
    int top_left     = square + 7;
    int bottom_left  = square - 9;
    int top_right    = square + 9;
    int up           = square + 8;
    int down         = square - 8;
    int left         = square - 1;
    int right        = square + 1;

    if (bottom_left >= 0 && ((bottom_left + 1) % 8) != 0) 
        moves.push_back(bottom_left);

    if (bottom_right >= 0 && (bottom_right % 8) != 0) 
        moves.push_back(bottom_right);
    
    if (top_left < 64 && (top_left  + 1) % 8 != 0) 
        moves.push_back(top_left);

    if (top_right < 64 && (top_right + 1) % 8 != 0)  
        moves.push_back(top_right);

    if (up < 64) 
        moves.push_back(up);

    if (down >= 0) 
        moves.push_back(down);

    if (left >= 0 && (left + 1) % 8 != 0) 
        moves.push_back(left);

    if (right < 64 && right % 8 != 0)  
        moves.push_back(right);

    return moves;
}


std::vector<Move> generate_all_moves(const Position& pos, int color) {
    assert(color == WHITE || color == BLACK);

    Bitboard occupancy = color == WHITE ? pos._white_occupancy() : pos._black_occupancy();

    std::vector<Move> total;

    for (int square = 0; square < 64; square++) {
        if (occupancy.is_bitset(square)) {
            pieceType piece_type = pos.pieceOn(square);

            if (piece_type == NOPE) {
                continue;
            }

            std::vector<int8_t> to_squares;

            switch (piece_type) {
                case PAWN: 
                    to_squares = pawn_moves(square, color, pos); 
                    break;
                case KNIGHT: 
                    to_squares = king_moves(square, color, pos); 
                    break;
                case BISHOP: 
                    to_squares = bishop_moves(square, color, pos); 
                    break;
                case ROOK: 
                    to_squares = rook_moves(square, color, pos); 
                    break;
                case QUEEN: 
                    to_squares = queen_moves(square, color, pos); 
                    break;
                case KING: 
                    to_squares = king_moves(square, color, pos); 
                    break;

                default :
                    break;
            }

            if (to_squares.empty()) 
                continue;

            std::vector<Move> moves;

            for (int i = 0, n = to_squares.size(); i < n; i++) {
                Move m = Move(square, to_squares[i], Quiet);
                moves.push_back(m);
            }

            for (Move m : moves) 
                total.push_back(m);
        }
    }

    return total;
}

std::vector<int> piece_squares(pieceType piece_type, const Position& pos) {
    if (piece_type == NOPE) 
        return {};

    assert(piece_type <= NOPE && piece_type >= KING);

    std::vector<int> ans;

    for (int sq = 0; sq < 64; sq++) {
        if (pos.pieces[sq] == piece_type) 
            ans.push_back(sq);
    }

    return ans;
}

void setAttackedSquares(Position& pos) {
    int color = pos.getSide();
    int oppent_color = color == WHITE ? BLACK : WHITE;

    std::unordered_set<int8_t> total_access;
    auto add_moves = [&total_access](const std::vector<int8_t>& moves) {
        for (int8_t move : moves) 
            total_access.insert(move);
    };

    for (const pieceType piece : pos.pieces) {
        std::vector<int> piece_sq = piece_squares(piece, pos);

        if (piece_sq.empty()) 
            continue;

        for (const int sq : piece_sq) {
            std::vector<int8_t> access;

            switch (piece) {
                case PAWN: 
                    access = pawn_moves(sq , color , pos); 
                    break;
                case KNIGHT: 
                    access = knight_moves(sq , color , pos); 
                    break;
                case BISHOP: 
                    access = bishop_moves(sq , color , pos); 
                    break;
                case ROOK: 
                    access = bishop_moves(sq , color , pos); 
                    break;
                case QUEEN: 
                    access = queen_moves(sq , color , pos);
                    break;
                case KING:
                    access = king_moves(sq , color , pos); 
                    break;

                default: 
                    break;
            }

            add_moves(access);
        }
    }

    for (int sq = 0; sq < 64; sq++) {
        color == WHITE ?
             pos.attacked_black[sq] = (total_access.find(sq) != total_access.end()):
             pos.attacked_white[sq] = (total_access.find(sq) != total_access.end());
    }
}

bool is_tactical(const Move& _move) {
    unsigned int _flag = _move.getFlags();
    if (_flag == CAPTURE)  
        return true;

    return false;
}

bool isPseudoLegal(const Position& pos, const Move& move) {
    const    int color       = pos.getSide();
    unsigned int from        = move.getFrom();
    unsigned int to          = move.getTo();
    unsigned int _flag       = move.getFlags();
    unsigned int _move_color = pos.getColor(from);
    pieceType    moved_piece = pos.pieceOn(from);
    Bitboard     occupancy   = pos.occupancy();
    Bitboard     our_occ     = color == WHITE ? pos._white_occupancy() : pos._black_occupancy();

    assert( from < 64  &&  from >= 0 );
    assert( to   < 64  &&  to   >= 0 );

    if (our_occ.is_bitset(to) == true) 
        return false;
    
    if (color != _move_color)
        return false;

    if (pos.pieceOn(from) == NOPE)   
        return false;

    if (from == to)   
        return false;

    if (moved_piece == NOPE)  
        return false;
    

    if ((_flag == KSCastle || _flag == QSCastle) && moved_piece != KING) 
        return false;

    if (_flag == EN_PASSANT && moved_piece != PAWN) 
        return false;

    if (_flag == EN_PASSANT && pos.pieceOn(to) != NOPE) 
        return false;

    if (_flag == Capture && pos.pieceOn(to) == NOPE) 
        return false;

    if ((_flag == Promo || _flag == enPassant) && moved_piece != PAWN) 
        return false;

    const int NORTH = (color == WHITE) ? -8 : 8;

    switch (moved_piece) {
        case PAWN: {
            std::vector<int8_t> moves = pawn_moves(from, color, pos);
            if (std::find(moves.begin(), moves.end(), to) == moves.end()) 
                return false;

            if (_flag == EN_PASSANT) {
                if (color == WHITE && to != (from + NORTH)) 
                    return false;
            
                if (color == BLACK && to != (from - NORTH)) 
                    return false;
            } else if (_flag == Capture && pos.pieceOn(to) == NOPE) {
                return false;
            } else if (_flag == Promo) {
                if (color == WHITE ? ranks[from] != 6 : ranks[from] != 1) 
                    return false;

                if (color == WHITE ? ranks[to]   != 7 : ranks[to] != 1) 
                    return false;
            } else {
                if (color == WHITE && ranks[from] >= 1 && ranks[from] < 6 && ranks[from] >= ranks[to]) 
                    return false;
            
                if (color == BLACK && ranks[from] >= 6 && ranks[from] < 1 && ranks[from] <= ranks[to]) 
                    return false;
            }
        }
            break;
        
        case KNIGHT: {
            std::vector<int8_t> moves = knight_moves(from, color, pos);

            if (std::find(moves.begin(), moves.end(), to) == moves.end()) 
                return false;
        }
            break;

        case BISHOP: {
            std::vector<int8_t> moves = bishop_moves(from, color, pos);

            if (std::find(moves.begin(), moves.end(), to) == moves.end()) 
                return false;
        }
            break;

        case ROOK: {
            std::vector<int8_t> moves = rook_moves(from, color, pos);

            if (std::find(moves.begin(), moves.end(), to) == moves.end()) 
                return false;
        }
            break;

        case QUEEN: {
            std::vector<int8_t> moves = queen_moves(from, color, pos);

            if (std::find(moves.begin(), moves.end(), to) == moves.end()) 
                return false;
        }
            break;

        case KING: {
            if (pos.isAttacked(to, color)) 
                return false;

            if (_flag == KSCastle) {
                if (color == WHITE) {
                    if (occupancy.is_bitset(f1) || occupancy.is_bitset(g1) ||
                        pos.isAttacked(f1, WHITE) || pos.isAttacked(g1, WHITE)) 

                        return false;
                } else {
                    if (occupancy.is_bitset(f8) || occupancy.is_bitset(g8) ||
                        pos.isAttacked(f8, BLACK) || pos.isAttacked(g8, BLACK)) 

                        return false;
                }
            } else if (_flag == QSCastle) {
                if (color == WHITE) {
                    if (occupancy.is_bitset(d1) || occupancy.is_bitset(c1) || occupancy.is_bitset(b1)
                        ||
                        pos.isAttacked(d1, WHITE) || pos.isAttacked(c1, WHITE) || pos.isAttacked(b1, WHITE)) 

                        return false;
                } else {
                    if (occupancy.is_bitset(d8) || occupancy.is_bitset(c8) || occupancy.is_bitset(b8)
                        ||
                        pos.isAttacked(d8, BLACK) || pos.isAttacked(c8, BLACK) || pos.isAttacked(b8, BLACK)) 

                        return false; 
                }
            }

            std::vector<int8_t> moves = king_moves(from, color, pos);
        
            if (std::find(moves.begin(), moves.end(), to) == moves.end()) 
                return false;
        }
            break;
        
        default: 
            break;
    }

    return true;
}


Bitboard get_piece_attacks(const Position& pos, int square) {
    pieceType piece = pos.pieceOn(square);
    
    if (piece == NOPE) 
        return Bitboard(0ULL);

    int side                  = pos.getColor(square);
    Bitboard occ              = side == WHITE ? pos._black_occupancy() : pos._white_occupancy();
    std::vector<int8_t> moves = rook_moves(square, side, pos);
    Bitboard result           = Bitboard(0ULL);
    
    if (moves.empty()) 
        return result;

    for (const int8_t m : moves) 
        if (occ.is_bitset(m)) 
            result.set_bit(m);

    return result;
}


bool isLegal(const Position& pos, const Move& move) {
    unsigned int color = pos.getSide();
    unsigned int _flag = move.getFlags();
    unsigned int from  = move.getFrom();
    unsigned int to    = move.getTo();
    pieceType    piece = pos.pieceOn(from);

    assert(color == pos.getColor(from));
    assert(from < 64  &&  from >= 0);
    assert(to   < 64  &&  to   >= 0);

    if (_flag != Capture && pos.pieceOn(to) != NOPE) 
        return false;

    if (piece == KING) {
        if (_flag == KSCastle) 
            return !pos.isAttacked( color == WHITE ? f1 : f8, color )
                && !pos.isAttacked( color == WHITE ? g1 : g8, color );

        if (_flag == QSCastle)
            return !pos.isAttacked( color == WHITE ? b1 : b8, color )
                && !pos.isAttacked( color == WHITE ? c1 : c8, color )
                && !pos.isAttacked( color == WHITE ? d1 : d8, color );

        if (pos.isAttacked(to, color)) 
            return false;
    }

    if (pos.isPinned(from)) 
        return false;
    
    return true;
}
