#define __SEARCH_CPP__

#include "search.h"
#include "eval.h"
#include "move.h"
#include "movepick.h"
#include "ttable.h"

#include <cassert>
#include <vector>


template <bool pvNode> int Quiescence(int alpha, int beta, ThreadData* thread_data, SearchStack* ss);

TranspositionTable tt;
const int          ageMask = 0b11111000;

uint8_t ttAge(uint8_t ageBoundPV) { 
    return (ageBoundPV & ageMask) >> 3; 
}

// just another hashing function
uint64_t hash(const uint64_t key) {
    uint64_t hash = key;

    hash ^= (hash >> 33);
    hash *= 0xff51afd7ed558ccd;
    hash ^= (hash >> 33);
    hash *= 0xc4ceb9fe1a85ec53;
    hash ^= (hash >> 33);

    return hash;
}

static bool isRepetition(const Position& pos) {
    assert(pos.half_moves >= pos.fifty_moves_counter);

    int counter = 0;
    int dis     = std::min(pos.fifty_moves_counter, pos.ply_fromNull);
    int start   = pos.played_positions.size();

    for (int i = 0; i <= dis; i += 2) {
        if (pos.played_positions[start - i] == pos.position_key) {
            if (i <= pos.stacked_his && i <= pos.stacked_his) {
                return true;
            }

            counter++;

            if (counter >= 2) {
                return true;
            }
        }
    }

    return false;
}

bool isMaterialDraw(const Position& pos) {
    std::vector<pieceType> w_pieces;
    std::vector<pieceType> b_pieces;
    Bitboard               black_occ = pos._black_occupancy();
    Bitboard               white_occ = pos._white_occupancy();

    for (int square = 0; square < 64; ++square) {
        pieceType piece = pos.pieceOn(square);

        if (piece != NOPE) {
            if (white_occ.is_bitset(square)) {
                w_pieces.push_back(piece);
            } else {
                b_pieces.push_back(piece);
            }
        }
    }

    if (w_pieces.empty() && b_pieces.empty()) {
        return true;
    }

    const int sum = w_pieces.size() + b_pieces.size();

    if (sum == 1) {
        pieceType single_piece = w_pieces.empty() ? b_pieces[0] : w_pieces[0];
        return (single_piece != ROOK && single_piece != QUEEN);
    }

    if (sum == 2) {
        if (w_pieces.empty() || b_pieces.empty()) {
            const auto& pieces = w_pieces.empty() ? b_pieces : w_pieces;

            for (pieceType piece : pieces) {
                if (piece != KNIGHT) {
                    return false;
                }
            }

            return true;
        } else {
            pieceType w_piece = w_pieces[0];
            pieceType b_piece = b_pieces[0];

            return (w_piece == KNIGHT && b_piece == KNIGHT) || (w_piece == KNIGHT && b_piece == BISHOP) ||
                   (w_piece == BISHOP && b_piece == KNIGHT);
        }
    }

    return false;
}

bool isFiftyMovesDraw(const Position& pos) {
    return true;
}

bool isDraw(const Position& pos) { 
    return isRepetition(pos) || isFiftyMovesDraw(pos) || isMaterialDraw(pos); 
}

uint8_t pack(uint8_t bound, bool wasPv, uint8_t age) {
    return static_cast<uint8_t>(bound + (wasPv << 2) + (age << 3));
}

int get_history_score(const Position& pos, SearchData* search_data, const Move& move, SearchStack* ss) {
    return 1;
}

void update_corrHistScore(const Position* pos, SearchData* search_data, int depth, int score) {
    return;
}

void update_histories(
    const Position* pos, SearchData* search_data, SearchStack* ss, int depth, const Move& best_move,
    MoveList* quietMoves, MoveList* noisyMoves) {
    
    return;
}

int get_single_score(SearchStack* ss, Move& move, int offset) {
    return 1;
}

void update_score(SearchStack* ss, Move& move, int bonus, const int offset, const Position& pos) {
    if ((ss - offset)->move.asShort()) {
        const int scaledBonus = bonus - get_single_score(ss, move, offset) * std::abs(bonus) / CH_MAX;
        (*((ss - offset)->contHistEntry))[pos.pieceOn(move.getFrom())] += scaledBonus;
    }
}

int history_bonus(int depth) { 
    return std::min(16 * depth * depth + 32 * depth + 16, 1200); 
}

Bitboard get_piece_by_type(const Position* pos, pieceType piece) {
    switch (piece) {
    case KING:
        return pos->white_king.board() | pos->black_king.board();
    case QUEEN:
        return pos->white_queens.board() | pos->black_queens.board();
    case ROOK:
        return pos->white_rooks.board() | pos->black_rooks.board();
    case BISHOP:
        return pos->white_bishops.board() | pos->black_bishops.board();
    case KNIGHT:
        return pos->white_knights.board() | pos->black_knights.board();
    case PAWN:
        return pos->white_pawns.board() | pos->black_pawns.board();

    default:
        return Bitboard(0ULL);
    }
}

bool SEE(const Position pos, Move& move, int thresh_hold) {
    unsigned int _flag  = move.getFlags();
    unsigned int to     = move.getTo();
    unsigned int from   = move.getFrom();
    pieceType    target = _flag == enPassant ? PAWN : pos.pieceOn(to);
    int          promo; //= promoted_piece_type(move);
    int          val  = SEEval[target] - thresh_hold;
    int          side = pos.current_side;

    if (_flag == KSCastle || _flag == QSCastle) {
        return thresh_hold <= 0;
    }

    if (val < 0) {
        return false;
    }

    pieceType attacker = pos.pieceOn(from);
    val -= _flag == Promo ? SEEval[promo] : SEEval[attacker];

    if (val >= 0) {
        return true;
    }

    Bitboard occ = pos.occupancy().board() ^ (1ULL << from);

    if (_flag == enPassant) {
        occ; // ^= get_enpassant_sq(pos);
    }

    uint64_t attackers; //  = pos.attacks_to(to, occ);

    Bitboard bishops = Bitboard(
        side == WHITE ? pos.white_bishops.board() | pos.white_queens.board()
                      : pos.black_bishops.board() | pos.black_queens.board());

    Bitboard rooks = Bitboard(
        side == WHITE ? pos.white_rooks.board() | pos.white_queens.board()
                      : pos.black_rooks.board() | pos.black_queens.board());

    Bitboard our_occ = side == WHITE ? pos._white_occupancy() : pos._black_occupancy();

    while (true) {
        attackers &= occ.board();

        Bitboard OurAttackers = attackers & our_occ.board();

        if (!OurAttackers.board()) {
            break;
        }

        pieceType piece_type;
        for (piece_type = PAWN; piece_type < KING; piece_type = static_cast<pieceType>(piece_type + 1)) {
            if (OurAttackers.board() & get_piece_by_type(&pos, piece_type).board()) {
                break;
            }
        }

        side ^= 1;
        val = -val - 1 - SEEval[piece_type];

        if (val >= 0) {
            if (piece_type == KING && (attackers & our_occ.board())) {
                side ^= 1;
            }

            break;
        }

        // occ ^= 1ULL << ();

        if (piece_type == PAWN || piece_type == BISHOP || piece_type == QUEEN) {
            attackers |= get_piece_attacks(pos, to).board() & bishops.board();
        }

        if (piece_type == ROOK || piece_type == QUEEN) {
            attackers |= get_piece_attacks(pos, to).board() & rooks.board();
        }
    }

    return side != pos.getColor(from);
}

Move get_best_move(const PvTable* pvTable) { 
    return pvTable->pvArray[0][0]; 
}

template <bool pvNode>
int search(int alpha, int beta, int depth, const bool cutNode, ThreadData* thread_data, SearchStack* ss) {
    Position*   pos              = &thread_data->pos;
    SearchData* search_data      = &thread_data->search_data;
    SearchInfo* info             = &thread_data->info;
    PvTable*    pv_table         = &thread_data->pvTable;
    const bool  inCheck          = pos->inCheck;
    const bool  isRootNode       = (ss->ply == 0);
    const Move  excludedMove     = ss->excludedMove;
    const short excludedMove_val = excludedMove.asShort();
    int         score            = -MAXSCORE;
    int         eval;
    int         rawEval;
    bool        improve;
    MoveList    list;

    if (!excludedMove_val) {
        pv_table->pvLength[ss->ply] = ss->ply;
    }

    if (ss->ply > info->seldepth) {
        info->seldepth = ss->ply;
    }

    if (depth <= 0) {
        return 0;
    }

    if (thread_data->id == 0 && TimeOver(&thread_data->info)) {
        thread_interrupt();
        thread_data->info.stopped = true;
    }

    if (!isRootNode) {
        if (isDraw(*pos)) {
            return 0;
        }

        if (ss->ply >= MAX_DEPTH - 1) {
            return inCheck ? 0 : network_eval(*pos, nnue, caches);
        }

        alpha = std::max(alpha, -MATE_SCORE + ss->ply);
        beta  = std::min(beta, MATE_SCORE - ss->ply - 1);

        if (alpha >= beta) {
            return alpha;
        }

        // check for upcoming repetition
    }

    ttEntry*       tt_entry = tt.probe(pos->position_key);
    TT_data        tt_data  = tt_entry ? tt_entry->read() : TT_data();
    const bool     tt_exist = !excludedMove_val && tt_entry;
    const uint16_t move_val = tt_exist ? tt_data.move : NOMOVE;
    const Move     tt_move  = tt_exist ? Move(move_val) : Move();

    if (!pvNode && tt_data.value != SCORE_NONE && tt_data.depth >= depth &&
        ((tt_data.bound == UPPER && tt_data.value <= alpha) ||
         (tt_data.bound == LOWER && tt_data.value >= alpha) || tt_data.bound == EXACT)) {
        return tt_data.eval;
    }

    if (depth >= 4 && tt_data.bound == NO_BOUND) {
        depth--;
    }

    (ss + 1)->excludedMove = NOMOVE;
    (ss + 1)->searchKiller = NOMOVE;

    if (inCheck) {
        eval = rawEval = ss->staticEval = SCORE_NONE;
    } else if (excludedMove_val) {
        eval = rawEval = ss->staticEval;
    } else if (tt_exist) {
        rawEval = (tt_data.value != SCORE_NONE) ? tt_data.eval : network_eval(*pos, nnue, caches);
        // need to adjust the eval for the history context

        eval = ss->staticEval = rawEval;

        if (tt_data.eval != SCORE_NONE &&
            ((tt_data.bound == UPPER && tt_data.eval < eval) ||
             (tt_data.bound == LOWER && tt_data.eval > eval) || tt_data.bound == EXACT)) {
            eval = tt_data.eval;
        }
    } else {
        rawEval = network_eval(*pos, nnue, caches);
        // again you need to adjust the eval
        eval = rawEval;

        TT_data new_data;

        new_data.bound   = NO_BOUND;
        new_data.pos_key = pos->position_key;
        new_data.move    = NOMOVE;
        new_data.eval    = rawEval;
        new_data.depth   = 0;
        new_data.value   = SCORE_NONE;

        tt_entry->save(new_data);
    }

    if (inCheck) {
        improve = false;
    } else if ((ss - 2)->staticEval != SCORE_NONE) {
        improve = ss->staticEval > (ss - 2)->staticEval;
    } else if ((ss - 4)->staticEval != SCORE_NONE) {
        improve = ss->staticEval > (ss - 4)->staticEval;
    } else {
        improve = true;
    }

    if (!pvNode && !excludedMove_val && !inCheck) {
        if (depth < 10 && abs(eval) < MATE_FOUND && eval - 91 * (depth - improve) >= beta)
            return eval;

        Bitboard pawns    = pos->current_side == WHITE ? pos->white_pawns : pos->black_pawns;
        bool     no_pawns = (pawns.board() == 0ULL);

        if (eval >= ss->staticEval && eval >= beta && (ss - 1)->move != NOMOVE &&
            ss->ply >= thread_data->nmpPlies && no_pawns) {
            ss->move          = NOMOVE;
            const int R       = 3 + depth / 3 + std::min((eval - beta) / 200, 3);
            ss->contHistEntry = &search_data->contHist[NOPE];

            pos->make_null_move();

            int nmpScore = -search<false>(-beta, -beta + 1, depth - R, !cutNode, thread_data, ss + 1);

            pos->take_null_move();

            if (nmpScore >= beta) {
                if (nmpScore > MATE_FOUND) {
                    nmpScore = beta;
                }

                if (thread_data->nmpPlies || depth < 15) {
                    return nmpScore;
                }

                thread_data->nmpPlies = ss->ply + (depth - R) * 3;
                int verificationScore = search<false>(beta - 1, beta, depth - R, false, thread_data, ss);
                thread_data->nmpPlies = 0;

                if (verificationScore >= beta) {
                    return nmpScore;
                }
            }
        }

        if (depth <= 5 && eval + 256 * depth < alpha) {
            const int razorScore = Quiescence<false>(alpha, beta, thread_data, ss);

            if (razorScore <= alpha) {
                return razorScore;
            }
        }
    }

    const int  origAlpha  = alpha;
    int        bestScore  = -MAXSCORE;
    int        totalMoves = 0;
    bool       skipQuiets = false;
    Move       best_move  = NOMOVE;
    Move       move;

    Movepicker move_picker = Movepicker();
    MoveList   quietMoves, noisyMoves;

    while ((move = move_picker.next(skipQuiets)) != NOMOVE) {
        if (move == excludedMove || !isLegal(*pos, move)) {
            continue;
        }

        totalMoves++;

        const bool isQuiet     = !is_tactical(move);
        const int  moveHistory = get_history_score(*pos, search_data, move, ss);
        Bitboard   pawns       = pos->current_side == WHITE ? pos->white_pawns : pos->black_pawns;
        bool       no_pawns    = (pawns.board() == 0ULL);

        if (!isRootNode && no_pawns && bestScore > -MATE_FOUND) {
            const int lmrDepth = std::max(
                0, depth - reductions[isQuiet][std::min(depth, 63)][std::min(totalMoves, 63)] +
                       moveHistory / 8192);

            if (!skipQuiets) {
                if (!pvNode && !inCheck && totalMoves > lmp_margin[std::min(depth, 63)][improve]) {
                    skipQuiets = true;
                }

                if (!inCheck && lmrDepth < 11 && ss->staticEval + 250 + 150 * lmrDepth <= alpha) {
                    skipQuiets = true;
                }
            }

            if (depth <= 8 && !SEE(*pos, move, see_margin[std::min(lmrDepth, 63)][isQuiet])) {
                continue;
            }
        }

        int extension = 0;

        if (ss->ply < thread_data->rootDepth * 2) {
            if (!isRootNode && depth >= 7 && move == tt_data.move && !excludedMove_val &&
                (tt_data.bound & LOWER) && abs(tt_data.value) < MATE_SCORE && tt_data.depth >= depth - 3) {
                const int singular_beta = tt_data.value - depth;
                const int singularDepth = (depth - 1) / 2;

                ss->excludedMove        = tt_data.move;
                int singular_score =
                    search<false>(singular_beta - 1, singular_beta, singularDepth, cutNode, thread_data, ss);
                ss->excludedMove = NOMOVE;

                if (singular_score < singular_beta) {
                    extension = 1;

                    if (!pvNode && singular_score < singular_beta - 17 && ss->doubleExtensions <= 11) {
                        extension = 2 + (!is_tactical(tt_data.move) && singular_score < singular_beta - 100);
                        ss->doubleExtensions = (ss - 1)->doubleExtensions + 1;
                        depth += depth < 10;
                    }
                } else if (singular_score >= beta) {
                    return singular_score;
                } else if (tt_data.value >= beta) {
                    extension = -2;
                } else if (cutNode) {
                    extension = -1;
                }
            }
        }

        int new_depth = depth - 1 + extension;

        pos->do_move(move);
        ss->contHistEntry = &search_data->contHist[pos->pieceOn(move.getFrom())];
        list.append(move);
        info->nodes++;
        const uint64_t nodes_before_search = info->nodes;

        if (totalMoves > 1 + pvNode && depth >= 3 && (isQuiet)) {
            int depth_reduction = reductions[isQuiet][std::min(depth, 63)][std::min(totalMoves, 63)];

            if (isQuiet) {
                if (cutNode) {
                    depth_reduction += 2;
                }

                if (!improve) {
                    depth_reduction++;
                }

                if (move == move_picker.killer || move == move_picker.count) {
                    depth_reduction--;
                }

                if (pos->inCheck) {
                    depth_reduction--;
                }

                depth_reduction -= moveHistory / 8192;
            } else {
                if (cutNode) {
                    depth_reduction += 2;
                }

                depth_reduction -= moveHistory / 6144;
            }

            depth_reduction   = std::clamp(depth_reduction, 0, new_depth - 1);
            int reduced_depth = new_depth - depth_reduction;
            score             = -search<false>(-alpha - 1, -alpha, reduced_depth, true, thread_data, ss + 1);

            if (score > alpha && new_depth > reduced_depth) {
                const bool goDeepe   = score > (bestScore + 53 + 2 * new_depth);
                const bool goShallow = score < (bestScore + new_depth);
                new_depth += goDeepe - goShallow;

                if (new_depth > reduced_depth) {
                    score -= search<false>(-alpha - 1, -alpha, new_depth, !cutNode, thread_data, ss + 1);
                }

                int bonus = score > alpha ? history_bonus(depth) : -history_bonus(depth);

                update_score(ss, move, bonus);
            }
        } else if (!pvNode || totalMoves > 1) {
            score = -search<false>(-alpha - 1, -alpha, new_depth, !cutNode, thread_data, ss + 1);
        }

        if (pvNode && (totalMoves == 1 || score > alpha)) {
            score = -search<true>(-beta, -alpha, new_depth, false, thread_data, ss + 1);
        }

        pos->undo_move(move);

        if (thread_data->id == 0 && isRootNode) {
            thread_data->nodeSpentTable[move.getFrom()] += info->nodes - nodes_before_search;
        }

        if (info->stopped) {
            return 0;
        }

        if (score > bestScore) {
            bestScore = score;

            if (score > alpha) {
                best_move = move;

                if (pvNode) {
                    pv_table->pvArray[ss->ply][ss->ply] = move;

                    for (int next = ss->ply + 1; next < pv_table->pvLength[ss->ply + 1]; next++) {
                        pv_table->pvArray[ss->ply][next] = pv_table->pvArray[ss->ply + 1][next];
                    }

                    pv_table->pvLength[ss->ply] = pv_table->pvLength[ss->ply + 1];
                }

                if (score >= beta) {
                    if (isQuiet) {
                        ss->searchKiller = best_move;

                        if (ss->ply >= 1) {
                            search_data->counterMoves[(ss - 1)->move.getFrom()] = move.asShort();
                        }
                    }

                    update_histories(
                        pos, search_data, ss, depth + (eval <= alpha), best_move, &quietMoves, &noisyMoves);

                    break;
                }

                alpha = score;
            }
        }
    }

    if (totalMoves == 0) {
        return excludedMove_val ? -MAXSCORE : inCheck ? -MATE_SCORE + ss->ply : 0;
    }

    int bound = bestScore >= beta ? LOWER : alpha != origAlpha ? EXACT : UPPER;

    if (!excludedMove.asShort()) {
        if (!inCheck && (!best_move.asShort() || !is_tactical(best_move)) &&
            !(bound == LOWER && bestScore <= ss->staticEval) &&
            !(bound == UPPER && bestScore >= ss->staticEval)) {
            update_corrHistScore(pos, search_data, depth, bestScore - ss->staticEval);
        }

        TT_data new_data;

        new_data.bound   = bound;
        new_data.depth   = depth;
        new_data.eval    = rawEval;
        new_data.pos_key = pos->position_key;
        new_data.move    = best_move.asShort();

        tt_entry->save(new_data);
    }

    return bestScore;
}

template <bool pvNode> int Quiescence(int alpha, int beta, ThreadData* thread_data, SearchStack* ss) {
    Position*   pos         = &thread_data->pos;
    SearchData* search_data = &thread_data->search_data;
    SearchInfo* info        = &thread_data->info;
    const bool  inCheck     = pos->inCheck;
    int         best_score;
    int         rawEval;

    if (thread_data->id == 0 && TimeOver(&thread_data->info)) {
        thread_interrupt();
        thread_data->info.stopped = true;
    }

    if (isDraw(*pos)) {
        return 0;
    }

    if (ss->ply >= MAX_DEPTH - 1) {
        return inCheck ? 0 : network_eval(*pos, nnue, caches);
    }

    ttEntry*   tt_entry = tt.probe(pos->position_key);
    const bool tt_hit   = tt_entry ? true : false;
    TT_data    tt_data  = tt_entry->read();

    if (!pvNode && tt_data.value != SCORE_NONE &&
        ((tt_data.bound == UPPER && tt_data.value <= alpha) ||
         (tt_data.bound == LOWER && tt_data.value >= beta) || tt_data.bound == EXACT)) {
        return tt_data.value;
    }

    const bool ttPv = pvNode || (tt_hit && formerPv(tt_data.bound));

    if (inCheck) {
        rawEval = ss->staticEval = SCORE_NONE;
        best_score               = -MAXSCORE;
    } else if (tt_hit) {
        rawEval        = tt_data.eval != SCORE_NONE ? tt_data.eval : network_eval(*pos, nnue, caches);
        ss->staticEval = best_score = adjustEvalWithCorrHist(pos, search_data, rawEval);

        if (tt_data.value != SCORE_NONE &&
            ((tt_data.bound == UPPER && tt_data.value < best_score) ||
             (tt_data.bound == LOWER && tt_data.value > best_score) || tt_data.bound == EXACT)) {
            best_score = tt_data.value;
        }
    } else {
        rawEval    = network_eval(*pos, nnue, pos.accumulator);
        best_score = ss->staticEval = adjustEvalWithCorrHist(pos, search_data, rawEval);

        TT_data new_data;

        new_data.pos_key = pos->position_key;
        new_data.move    = NOMOVE;
        new_data.value   = SCORE_NONE;
        new_data.eval    = rawEval;
        new_data.bound   = NO_BOUND;
        new_data.depth   = 0;

        tt_entry->save(new_data);
    }

    if (best_score >= beta) {
        return best_score;
    }

    alpha = std::max(alpha, best_score);

    Movepicker move_picker;
    // initialize the move picker

    Move best_move = NOMOVE;
    Move move;
    int  totalMoves = 0;

    while ((move = move_picker.next(!inCheck || best_score > -MATE_FOUND)) != NOMOVE) {
        if (!isLegal(*pos, move)) {
            continue;
        }

        totalMoves++;

        bool     has_pawns = false;
        Bitboard pawns     = pos->current_side == WHITE ? pos->white_pawns : pos->black_pawns;

        if (pawns.board() != 0ULL) {
            has_pawns = true;
        }

        if (best_score > -MATE_FOUND && !inCheck && !has_pawns) {
            const int fut_base = ss->staticEval + 192;

            if (fut_base <= alpha && !SEE(*pos, move, 1)) {
                best_score = std::max(fut_base, best_score);
                continue;
            }
        }

        ss->move = move;
        pos->do_move(move);

        info->nodes++;
        const int score = -Quiescence<pvNode>(-beta, -alpha, thread_data, ss + 1);

        pos->undo_move(move);

        if (info->stopped) {
            return 0;
        }

        if (score > best_move.asShort()) {
            best_score = score;

            if (score > alpha) {
                best_move = move;

                if (score >= beta) {
                    break;
                }

                alpha = score;
            }
        }
    }

    if (totalMoves == 0 && inCheck) {
        return -MATE_SCORE + ss->ply;
    }

    int     bound = best_score >= beta ? LOWER : UPPER;

    TT_data new_data;

    new_data.pos_key = pos->position_key;
    new_data.bound   = bound;
    new_data.depth   = 0;
    new_data.eval    = rawEval;
    new_data.move    = best_move.asShort();
    new_data.value   = best_score;

    tt_entry->save(new_data);

    return best_score;
}
