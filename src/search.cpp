// search.cpp

#include "search.h"
#include "eval.h"
#include "move.h"
#include "movepick.h"
#include "ttable.h"
#include "types.h"
#include "thread.h"
#include "position.h"

#include <cassert>
#include <vector>


namespace Shahrazad {
namespace search {

// forward declarations
template<bool pvNode>
int search::Quiescence(int alpha, int beta, thread::ThreadData* thread_data, search::SearchStack* ss);

tt::TranspositionTable transposition_table;
const int              ageMask = 0b11111000;

uint8_t search::ttAge(uint8_t ageBoundPV) { return (ageBoundPV & ageMask) >> 3; }

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

// check if the position is a repetition
static bool search::isRepetition(const position::Position& pos) {
    assert(pos.half_moves >= pos.fifty_moves_counter);

    int counter = 0;
    int dis     = std::min(pos.fifty_moves_counter, pos.ply_fromNull);
    int start   = pos.played_positions.size();

    for (int i = 0; i <= dis; i += 2)
    {
        if (pos.played_positions[start - i] == pos.position_key)
        {
            if (i <= pos.stacked_his && i <= pos.stacked_his)
            {
                return true;
            }

            counter++;

            if (counter >= 2)
            {
                return true;
            }
        }
    }

    return false;
}

// check if the position is a draw
bool search::isMaterialDraw(const position::Position& pos) {
    std::vector<types::PieceType> w_pieces;
    std::vector<types::PieceType> b_pieces;
    board::Bitboard               black_occ = pos._black_occupancy();
    board::Bitboard               white_occ = pos._white_occupancy();

    for (int square = 0; square < 64; ++square)
    {
        types::PieceType piece = pos.pieceOn(square);

        if (piece != types::PieceType::NOPE)
        {
            if (white_occ.is_bitset(square))
            {
                w_pieces.push_back(piece);
            }
            else
            {
                b_pieces.push_back(piece);
            }
        }
    }

    if (w_pieces.empty() && b_pieces.empty())
    {
        return true;
    }

    const int sum = w_pieces.size() + b_pieces.size();

    if (sum == 1)
    {
        types::PieceType single_piece = w_pieces.empty() ? b_pieces[0] : w_pieces[0];
        return (single_piece != types::PieceType::ROOK && single_piece != types::PieceType::QUEEN);
    }

    if (sum == 2)
    {
        if (w_pieces.empty() || b_pieces.empty())
        {
            const auto& pieces = w_pieces.empty() ? b_pieces : w_pieces;

            for (auto piece : pieces)
            {
                if (piece != types::PieceType::KNIGHT)
                    return false;
            }

            return true;
        }
        else
        {
            types::PieceType w_piece = w_pieces[0];
            types::PieceType b_piece = b_pieces[0];

            return (w_piece == types::PieceType::KNIGHT && b_piece == types::PieceType::KNIGHT)
                || (w_piece == types::PieceType::KNIGHT && b_piece == types::PieceType::BISHOP)
                || (w_piece == types::PieceType::BISHOP && b_piece == types::PieceType::KNIGHT);
        }
    }

    return false;
}

bool search::isFiftyMovesDraw(const position::Position& pos) { 
    return (pos.fifty_moves_counter >= 100);
}

bool search::isDraw(const position::Position& pos) {
    return isRepetition(pos) || isFiftyMovesDraw(pos) || isMaterialDraw(pos);
}

uint8_t search::pack(uint8_t bound, bool wasPv, uint8_t age) {
    return static_cast<uint8_t>(bound + (wasPv << 2) + (age << 3));
}

int search::get_history_score(const position::Position& pos, SearchData* search_data, const types::Move& move,
                              SearchStack* ss) {
    int score = 0;
    int offset = 0;
    int piece = static_cast<int>(pos.pieceOn(move.getFrom()));
    int target = static_cast<int>(pos.pieceOn(move.getTo()));
    int piece_type = static_cast<int>(pos.pieceOn(move.getFrom()));
    int target_type = static_cast<int>(pos.pieceOn(move.getTo()));
    int piece_color = static_cast<int>(pos.getColor(move.getFrom()));
    int target_color = static_cast<int>(pos.getColor(move.getTo()));
    int piece_value = static_cast<int>(types::SEEval[piece_type]);
    int target_value = static_cast<int>(types::SEEval[target_type]);
    int piece_value_color = static_cast<int>(types::SEEval[piece_color]);
    int target_value_color = static_cast<int>(types::SEEval[target_color]);

    if (piece_value > 0)
    {
        score += search_data->searchHis[piece_color][piece_value] * piece_value_color;
    }
}

void search::update_corrHistScore(const position::Position* pos, SearchData* search_data, int depth, int score) {
    return;
}

void search::update_histories(const position::Position* pos, SearchData* search_data, SearchStack* ss, int depth,
                              const types::Move& best_move, movegen::MoveList* quietMoves,
                              movegen::MoveList* noisyMoves) {

    return;
}

int search::get_single_score(SearchStack* ss, types::Move& move, int offset) { return 1; }

void search::update_score(search::SearchStack* ss, types::Move& move, int bonus, const int offset,
                          const position::Position& pos) {
    if ((ss - offset)->move.data())
    {
        const int scaledBonus = bonus - get_single_score(ss, move, offset) * std::abs(bonus) / CH_MAX;
        (*((ss - offset)->contHistEntry))[static_cast<int>(pos.pieceOn(move.getFrom()))] += scaledBonus;
    }
}

int search::history_bonus(int depth) { return std::min(16 * depth * depth + 32 * depth + 16, 1200); }

board::Bitboard search::get_piece_by_type(const position::Position* pos, types::PieceType piece) {
    switch (piece)
    {
    case types::PieceType::KING :
        return pos->white_king.board() | pos->black_king.board();
    case types::PieceType::QUEEN :
        return pos->white_queens.board() | pos->black_queens.board();
    case types::PieceType::ROOK :
        return pos->white_rooks.board() | pos->black_rooks.board();
    case types::PieceType::BISHOP :
        return pos->white_bishops.board() | pos->black_bishops.board();
    case types::PieceType::KNIGHT :
        return pos->white_knights.board() | pos->black_knights.board();
    case types::PieceType::PAWN :
        return pos->white_pawns.board() | pos->black_pawns.board();
    default :
        return board::Bitboard(0ULL);
    }
}

bool search::SEE(const position::Position pos, types::Move& move, int thresh_hold) {
    unsigned int     _flag = move.getFlags();
    unsigned int     to    = move.getTo();
    unsigned int     from  = move.getFrom();
    types::PieceType target =
      _flag == static_cast<int>(types::MoveType::EN_PASSANT) ? types::PieceType::PAWN : pos.pieceOn(to);
    int promo;  //= promoted_piece_type(move);
    int val  = types::SEEval[static_cast<int>(target)] - thresh_hold;
    int side = static_cast<int>(pos.current_side);

    if (_flag == static_cast<int>(types::MoveType::KSCASTLE) || _flag == static_cast<int>(types::MoveType::QSCASTLE))
    {
        return thresh_hold <= 0;
    }

    if (val < 0)
    {
        return false;
    }

    types::PieceType attacker = pos.pieceOn(from);
    val -= _flag == static_cast<int>(types::MoveType::PROMOTION)
           ? static_cast<int>(types::SEEval[promo])
           : static_cast<int>(types::SEEval[static_cast<int>(attacker)]);

    if (val >= 0)
    {
        return true;
    }

    board::Bitboard occ = pos.occupancy().board() ^ (1ULL << from);

    if (_flag == static_cast<int>(types::MoveType::EN_PASSANT))
    {
        occ;  // ^= get_enpassant_sq(pos);}

        uint64_t attackers;  //  = pos.attacks_to(to, occ);

        board::Bitboard bishops = board::Bitboard(side == static_cast<int>(types::Color::WHITE)
                                                    ? pos.white_bishops.board() | pos.white_queens.board()
                                                    : pos.black_bishops.board() | pos.black_queens.board());

        board::Bitboard rooks = board::Bitboard(side == static_cast<int>(types::Color::WHITE)
                                                  ? pos.white_rooks.board() | pos.white_queens.board()
                                                  : pos.black_rooks.board() | pos.black_queens.board());

        board::Bitboard our_occ =
          side == static_cast<int>(types::Color::WHITE) ? pos._white_occupancy() : pos._black_occupancy();

        while (true)
        {
            attackers &= occ.board();

            board::Bitboard OurAttackers = attackers & our_occ.board();

            if (!OurAttackers.board())
            {
                break;
            }

            types::PieceType piece_type;
            for (piece_type = types::PieceType::PAWN; piece_type < types::PieceType::KING;
                 piece_type = static_cast<types::PieceType>(static_cast<int>(piece_type) + 1))
            {
                if (OurAttackers.board() & get_piece_by_type(&pos, piece_type).board())
                {
                    break;
                }
            }

            side ^= 1;
            val = -val - 1 - types::SEEval[static_cast<int>(piece_type)];

            if (val >= 0)
            {
                if (piece_type == types::PieceType::KING && (attackers & our_occ.board()))
                {
                    side ^= 1;
                }

                break;
            }

            // occ ^= 1ULL << ();

            if (piece_type == types::PieceType::PAWN || piece_type == types::PieceType::BISHOP
                || piece_type == types::PieceType::QUEEN)
            {
                attackers |= movegen::get_piece_attacks(pos, types::Square(to)).board() & bishops.board();
            }

            if (piece_type == types::PieceType::ROOK || piece_type == types::PieceType::QUEEN)
            {
                attackers |= movegen::get_piece_attacks(pos, types::Square(to)).board() & rooks.board();
            }
        }

        return side != static_cast<int>(pos.getColor(types::Square(from)));
    }
}

types::Move get_best_move(const PvTable* pvTable) { return pvTable->pvArray[0][0]; }

template<bool pvNode>
int Shahrazad::search::search(int alpha, int beta, int depth, const bool cutNode, thread::ThreadData* thread_data,
                              search::SearchStack* ss) {
    // Constants for magic numbers used in the search
    constexpr int FUTILITY_MARGIN_BASE         = 91;
    constexpr int RAZOR_MARGIN                 = 256;
    constexpr int HISTORY_DIVISOR_QUIET        = 8192;
    constexpr int HISTORY_DIVISOR_TACTICAL     = 6144;
    constexpr int DOUBLE_EXTENSION_THRESHOLD   = 17;
    constexpr int SINGULAR_EXTENSION_THRESHOLD = 100;
    constexpr int LMR_SCORE_BONUS_THRESHOLD    = 53;
    constexpr int MAX_DOUBLE_EXTENSIONS        = 11;

    // Thread data and search stack
    position::Position* pos              = &thread_data->pos;
    search::SearchData* search_data      = &thread_data->search_data;
    search::SearchInfo* info             = &thread_data->info;
    search::PvTable*    pv_table         = &thread_data->pvTable;
    const bool          inCheck          = pos->inCheck;
    const bool          isRootNode       = (ss->ply == 0);
    const types::Move   excludedMove     = ss->excludedMove;
    const short         excludedMove_val = excludedMove.data();
    int                 score            = -search::MAXSCORE;
    int                 eval;
    int                 rawEval;
    bool                improve;
    movegen::MoveList   list;

    // Initialize PV length for this ply if not in singular extension search
    if (!excludedMove_val)
    {
        pv_table->pvLength[ss->ply] = ss->ply;
    }

    // Update selective depth if necessary
    if (ss->ply > info->seldepth)
    {
        info->seldepth = ss->ply;
    }

    // Base cases
    if (depth <= 0)
    {
        return 0;
    }

    // Check for time management and interruption
    if (thread_data->id == 0 && thread::TimeOver(&thread_data->info))
    {
        thread::thread_interrupt();
        thread_data->info.stopped = true;
    }

    // Early termination checks for non-root nodes
    if (!isRootNode)
    {
        // Draw detection
        if (search::isDraw(*pos))
        {
            return 0;
        }

        // Maximum depth check
        if (ss->ply >= search::MAX_DEPTH - 1)
        {
            return inCheck ? 0 : eval::network_eval(*pos, nnue::nnue, nnue::caches);
        }

        // Mate distance pruning
        alpha = std::max(alpha, -search::MATE_SCORE + ss->ply);
        beta  = std::min(beta, search::MATE_SCORE - ss->ply - 1);

        if (alpha >= beta)
        {
            return alpha;
        }

        // Check for upcoming repetition would be handled here
    }

    // Transposition table lookup
    tt::TT_Entry*     tt_entry = search::transposition_table.probe(pos->position_key);
    tt::TT_data       tt_data  = tt_entry ? tt_entry->read() : tt::TT_data();
    const bool        tt_exist = !excludedMove_val && tt_entry;
    const uint16_t    move_val = tt_exist ? tt_data.move : static_cast<int>(types::MoveType::NOMOVE);
    const types::Move tt_move  = tt_exist ? types::Move(move_val) : types::Move();

    // TT cutoff (if score is exact or creates a cutoff and we're not in PV node)
    if (!pvNode && tt_data.value != search::SCORE_NONE && tt_data.depth >= depth
        && ((tt_data.bound == static_cast<int16_t>(types::Bound::UPPER) && tt_data.value <= alpha)
            || (tt_data.bound == static_cast<int16_t>(types::Bound::LOWER) && tt_data.value >= beta)
            || tt_data.bound == static_cast<int16_t>(types::Bound::EXACT)))
    {
        return tt_data.eval;
    }

    // Adjust depth for some conditions
    if (depth >= 4 && tt_data.bound == static_cast<int16_t>(types::Bound::NO_BOUND))
    {
        depth--;
    }

    // Initialize next ply search stack
    (ss + 1)->excludedMove = types::Move(static_cast<int>(types::MoveType::NOMOVE));
    (ss + 1)->searchKiller = types::Move(static_cast<int>(types::MoveType::NOMOVE));

    // Static evaluation
    if (inCheck)
    {
        // In check - no reliable static evaluation
        eval = rawEval = ss->staticEval = search::SCORE_NONE;
    }
    else if (excludedMove_val)
    {
        // Excluded move - use previously computed values
        eval = rawEval = ss->staticEval;
    }
    else if (tt_exist)
    {
        // Use or compute eval from transposition table
        rawEval =
          (tt_data.value != search::SCORE_NONE) ? tt_data.eval : eval::network_eval(*pos, nnue::nnue, nnue::caches);

        eval = ss->staticEval = rawEval;

        // If TT has more reliable evaluation, use it
        if (tt_data.eval != search::SCORE_NONE
            && ((tt_data.bound == static_cast<int16_t>(types::Bound::UPPER) && tt_data.eval < eval)
                || (tt_data.bound == static_cast<int16_t>(types::Bound::LOWER) && tt_data.eval > eval)
                || tt_data.bound == static_cast<int16_t>(types::Bound::EXACT)))
        {
            eval = tt_data.eval;
        }
    }
    else
    {
        // Compute fresh evaluation
        rawEval = eval::network_eval(*pos, nnue::nnue, nnue::caches);
        eval    = rawEval;

        // Store basic evaluation in TT
        tt::TT_data new_data(rawEval, types::Bound::NO_BOUND, types::MoveType::NOMOVE, search::SCORE_NONE,
                             pos->position_key, 0);
        tt_entry->save(new_data);
        transposition_table.save_entry(tt_entry);
    }

    // Determine if position is improving
    if (inCheck)
    {
        improve = false;
    }
    else if ((ss - 2)->staticEval != search::SCORE_NONE)
    {
        improve = ss->staticEval > (ss - 2)->staticEval;
    }
    else if ((ss - 4)->staticEval != search::SCORE_NONE)
    {
        improve = ss->staticEval > (ss - 4)->staticEval;
    }
    else
    {
        improve = true;
    }

    // ---------- Pruning techniques for non-PV nodes -----------
    if (!pvNode && !excludedMove_val && !inCheck)
    {
        // Reverse Futility Pruning / Static Null Move Pruning
        if (depth < 10 && std::abs(eval) < search::MATE_FOUND
            && eval - FUTILITY_MARGIN_BASE * (depth - improve) >= beta)
        {
            return eval;
        }

        // Get pawn presence information for pruning decisions
        board::Bitboard pawns    = pos->current_side == types::Color::WHITE ? pos->white_pawns : pos->black_pawns;
        bool            no_pawns = (pawns.board() == 0ULL);

        // Null Move Pruning
        if (eval >= ss->staticEval && eval >= beta
            && (ss - 1)->move.getFlags() != static_cast<int16_t>(types::MoveType::NOMOVE)
            && ss->ply >= thread_data->nmpPlies && no_pawns)
        {

            ss->move          = types::Move(static_cast<int16_t>(types::MoveType::NOMOVE));
            const int R       = 3 + depth / 3 + std::min((eval - beta) / 200, 3);
            ss->contHistEntry = &search_data->contHist[static_cast<int>(types::PieceType::NOPE)];

            // Make null move and search with reduced depth
            pos->make_null_move();
            int nmpScore = -search::search<false>(-beta, -beta + 1, depth - R, !cutNode, thread_data, ss + 1);
            pos->take_null_move();

            if (nmpScore >= beta)
            {
                // Adjust mate scores
                if (nmpScore > search::MATE_FOUND)
                {
                    nmpScore = beta;
                }

                // Return score directly for shallow depths or non-critical lines
                if (thread_data->nmpPlies || depth < 15)
                {
                    return nmpScore;
                }

                // Verify null move pruning with a reduced depth search
                thread_data->nmpPlies = ss->ply + (depth - R) * 3;
                int verificationScore = search<false>(beta - 1, beta, depth - R, false, thread_data, ss);
                thread_data->nmpPlies = 0;

                if (verificationScore >= beta)
                {
                    return nmpScore;
                }
            }
        }

        // Razoring - try quiescence search for hopeless positions
        if (depth <= 5 && eval + RAZOR_MARGIN * depth < alpha)
        {
            const int razorScore = search::Quiescence<false>(alpha, beta, thread_data, ss);

            if (razorScore <= alpha)
            {
                return razorScore;
            }
        }
    }

    // ---------- Move generation and search phase -----------
    const int   origAlpha  = alpha;
    int         bestScore  = -search::MAXSCORE;
    int         totalMoves = 0;
    bool        skipQuiets = false;
    types::Move best_move;
    best_move.null_();
    movepick::Movepicker move_picker = movepick::Movepicker();

    types::Move       move;
    movegen::MoveList quietMoves, noisyMoves;

    // Main move loop
    while (!(move = move_picker.next(skipQuiets)).is_null())
    {
        // Skip illegal moves and excluded moves (for singular extensions)
        if (move == excludedMove || !movegen::isLegal(*pos, move))
        {
            continue;
        }

        totalMoves++;

        const bool      isQuiet     = !movegen::is_tactical(move);
        const int       moveHistory = search::get_history_score(*pos, search_data, move, ss);
        board::Bitboard pawns       = pos->current_side == types::Color::WHITE ? pos->white_pawns : pos->black_pawns;
        bool            no_pawns    = (pawns.board() == 0ULL);

        // Move pruning and reduction logic
        if (!isRootNode && no_pawns && bestScore > -search::MATE_FOUND)
        {
            const int lmrDepth =
              std::max(0, depth - types::reductions[isQuiet][std::min(depth, 63)][std::min(totalMoves, 63)]
                            + moveHistory / HISTORY_DIVISOR_QUIET);

            // Late Move Pruning for quiet moves
            if (!skipQuiets)
            {
                if (!pvNode && !inCheck && totalMoves > types::lmp_margin[std::min(depth, 63)][improve])
                {
                    skipQuiets = true;
                }

                if (!inCheck && lmrDepth < 11 && ss->staticEval + 250 + 150 * lmrDepth <= alpha)
                {
                    skipQuiets = true;
                }
            }

            // Static Exchange Evaluation (SEE) pruning
            if (depth <= 8 && !search::SEE(*pos, move, types::see_margin[std::min(lmrDepth, 63)][isQuiet]))
            {
                continue;
            }
        }

        // Singular extensions
        int extension = 0;

        if (ss->ply < thread_data->rootDepth * 2)
        {
            // Only consider extensions for the TT move and in appropriate situations
            if (!isRootNode && depth >= 7 && move == tt_data.move && !excludedMove_val
                && (tt_data.bound & types::Bound::LOWER) && std::abs(tt_data.value) < search::MATE_SCORE
                && tt_data.depth >= depth - 3)
            {

                const int singular_beta = tt_data.value - depth;
                const int singularDepth = (depth - 1) / 2;

                // Singular extension search
                ss->excludedMove = tt_data.move;
                int singular_score =
                  search::search<false>(singular_beta - 1, singular_beta, singularDepth, cutNode, thread_data, ss);
                ss->excludedMove.null_();

                // Apply extensions based on singular extension search results
                if (singular_score < singular_beta)
                {
                    extension = 1;

                    // Double extension for very singular moves (much better than alternatives)
                    if (!pvNode && singular_score < singular_beta - DOUBLE_EXTENSION_THRESHOLD
                        && ss->doubleExtensions <= MAX_DOUBLE_EXTENSIONS)
                    {
                        extension = 2
                                  + (!movegen::is_tactical(tt_data.move)
                                     && singular_score < singular_beta - SINGULAR_EXTENSION_THRESHOLD);
                        ss->doubleExtensions = (ss - 1)->doubleExtensions + 1;
                        depth += depth < 10;
                    }
                }
                else if (singular_score >= beta)
                {
                    // Move causes a beta cutoff in the singular search
                    return singular_score;
                }
                else if (tt_data.value >= beta)
                {
                    // TT move might not be singular, but it's still good
                    extension = -2;
                }
                else if (cutNode)
                {
                    // Slight reduction in cut nodes when move isn't singular
                    extension = -1;
                }
            }
        }

        // Adjust depth based on extensions
        int new_depth = depth - 1 + extension;

        // Make the move
        movegen::do_move(std::ref<position::Position>(*pos), move);
        ss->contHistEntry = &search_data->contHist[static_cast<int>(pos->pieceOn(move.getFrom()))];
        list.append(move);
        info->nodes++;
        const uint64_t nodes_before_search = info->nodes;

        // Late Move Reductions (LMR)
        if (totalMoves > 1 + pvNode && depth >= 3 && (isQuiet))
        {
            // Calculate base reduction
            int depth_reduction = types::reductions[isQuiet][std::min(depth, 63)][std::min(totalMoves, 63)];

            // Adjust reduction based on move characteristics
            if (isQuiet)
            {
                if (cutNode)
                {
                    depth_reduction += 2;
                }

                if (!improve)
                {
                    depth_reduction++;
                }

                if (move == move_picker.killer || move == move_picker.count)
                {
                    depth_reduction--;
                }

                if (pos->inCheck)
                {
                    depth_reduction--;
                }

                depth_reduction -= moveHistory / HISTORY_DIVISOR_QUIET;
            }
            else
            {
                if (cutNode)
                {
                    depth_reduction += 2;
                }

                depth_reduction -= moveHistory / HISTORY_DIVISOR_TACTICAL;
            }

            // Apply reduction within limits
            depth_reduction   = std::clamp(depth_reduction, 0, new_depth - 1);
            int reduced_depth = new_depth - depth_reduction;

            // Reduced depth search with zero window
            score = -search::search<false>(-alpha - 1, -alpha, reduced_depth, true, thread_data, ss + 1);

            // Re-search at full depth if the reduced search exceeded alpha
            if (score > alpha && new_depth > reduced_depth)
            {
                const bool goDeepe   = score > (bestScore + LMR_SCORE_BONUS_THRESHOLD + 2 * new_depth);
                const bool goShallow = score < (bestScore + new_depth);
                new_depth += goDeepe - goShallow;

                if (new_depth > reduced_depth)
                {
                    score -= search::search<false>(-alpha - 1, -alpha, new_depth, !cutNode, thread_data, ss + 1);
                }

                // Update history scores based on search result
                int bonus = score > alpha ? search::history_bonus(depth) : -search::history_bonus(depth);
                search::update_score(ss, move, bonus);
            }
        }
        else if (!pvNode || totalMoves > 1)
        {
            // Regular zero window search for non-first moves or non-PV nodes
            score = -search::search<false>(-alpha - 1, -alpha, new_depth, !cutNode, thread_data, ss + 1);
        }

        // PV search - either first move in PV node or promising move
        if (pvNode && (totalMoves == 1 || score > alpha))
        {
            score = -search::search<true>(-beta, -alpha, new_depth, false, thread_data, ss + 1);
        }

        // Undo the move
        pos->undo_move();

        // Track node usage for root moves
        if (thread_data->id == 0 && isRootNode)
        {
            thread_data->nodeSpentTable[move.getFrom()] += info->nodes - nodes_before_search;
        }

        // Check for search stop
        if (info->stopped)
        {
            return 0;
        }

        // Update best score and alpha if this move is better
        if (score > bestScore)
        {
            bestScore = score;

            if (score > alpha)
            {
                best_move = move;

                // Update PV line
                if (pvNode)
                {
                    pv_table->pvArray[ss->ply][ss->ply] = move;

                    for (int next = ss->ply + 1; next < pv_table->pvLength[ss->ply + 1]; next++)
                    {
                        pv_table->pvArray[ss->ply][next] = pv_table->pvArray[ss->ply + 1][next];
                    }

                    pv_table->pvLength[ss->ply] = pv_table->pvLength[ss->ply + 1];
                }

                // Handle beta cutoff
                if (score >= beta)
                {
                    // Update killer moves and counter moves
                    if (isQuiet)
                    {
                        ss->searchKiller = best_move;

                        if (ss->ply >= 1)
                        {
                            search_data->counterMoves[(ss - 1)->move.getFrom()] = move.data();
                        }
                    }

                    // Update history tables
                    search::update_histories(pos, search_data, ss, depth + (eval <= alpha), best_move, &quietMoves,
                                             &noisyMoves);

                    break;
                }

                alpha = score;
            }
        }
    }

    // Handle special cases - no legal moves or excluded move search
    if (totalMoves == 0)
    {
        return excludedMove_val ? -search::MAXSCORE : inCheck ? -search::MATE_SCORE + ss->ply : 0;
    }

    // Determine bound type for TT entry
    int bound = bestScore >= beta  ? static_cast<int>(types::Bound::LOWER)
              : alpha != origAlpha ? static_cast<int>(types::Bound::EXACT)
                                   : static_cast<int>(types::Bound::UPPER);

    // Store search results in transposition table
    if (!excludedMove.data())
    {
        // Update correction history for non-tactical moves
        if (!inCheck && (!best_move.data() || !movegen::is_tactical(best_move))
            && !(bound == static_cast<int>(types::Bound::LOWER) && bestScore <= ss->staticEval)
            && !(bound == static_cast<int>(types::Bound::UPPER) && bestScore >= ss->staticEval))
        {
            search::update_corrHistScore(pos, search_data, depth, bestScore - ss->staticEval);
        }

        // Save position to transposition table
        tt::TT_data new_data(rawEval, bound, best_move.data(), bestScore, pos->position_key, depth);
        tt_entry->save(new_data);
        transposition_table.save_entry(tt_entry);
    }

    return bestScore;
}

// Quiescence search function
template<bool pvNode>
int Quiescence(int alpha, int beta, thread::ThreadData* thread_data, search::SearchStack* ss) {
    position::Position* pos         = &thread_data->pos;
    search::SearchData* search_data = &thread_data->search_data;
    search::SearchInfo* info        = &thread_data->info;
    const bool          inCheck     = pos->inCheck;
    int                 best_score;
    int                 rawEval;

    if (thread_data->id == 0 && thread::TimeOver(&thread_data->info))
    {
        thread::thread_interrupt();
        thread_data->info.stopped = true;
    }

    if (search::isDraw(*pos))
    {
        return 0;
    }

    if (ss->ply >= search::MAX_DEPTH - 1)
    {
        return inCheck ? 0 : eval::network_eval(*pos, nnue::nnue, nnue::caches);
    }

    tt::TT_Entry* tt_entry = transposition_table.probe(pos->position_key);
    const bool    tt_hit   = tt_entry ? true : false;
    tt::TT_data   tt_data  = tt_entry->read();

    if (!pvNode && tt_data.value != search::SCORE_NONE
        && ((tt_data.bound == static_cast<int16_t>(types::Bound::UPPER) && tt_data.value <= alpha)
            || (tt_data.bound == static_cast<int16_t>(types::Bound::LOWER) && tt_data.value >= beta)
            || tt_data.bound == static_cast<int16_t>(types::Bound::EXACT)))
    {
        return tt_data.value;
    }

    const bool ttPv = pvNode || (tt_hit && formerPv(tt_data.bound));

    if (inCheck)
    {
        rawEval = ss->staticEval = search::SCORE_NONE;
        best_score               = -search::MAXSCORE;
    }
    else if (tt_hit)
    {
        rawEval        = tt_data.eval != SCORE_NONE ? tt_data.eval : eval::network_eval(*pos, nnue, caches);
        ss->staticEval = best_score = adjustEvalWithCorrHist(pos, search_data, rawEval);

        if (tt_data.value != search::SCORE_NONE
            && ((tt_data.bound == static_cast<int16_t>(types::Bound::UPPER) && tt_data.value < best_score)
                || (tt_data.bound == static_cast<int16_t>(types::Bound::LOWER) && tt_data.value > best_score)
                || tt_data.bound == static_cast<int16_t>(types::Bound::EXACT)))
        {
            best_score = tt_data.value;
        }
    }
    else
    {
        rawEval    = eval::network_eval(*pos, nnue, pos.accumulator);
        best_score = ss->staticEval = adjustEvalWithCorrHist(pos, search_data, rawEval);

        tt::TT_data new_data(rawEval, static_cast<int16_t>(types::Bound::NO_BOUND), types::MoveType::NOMOVE,
                             search::SCORE_NONE, pos->position_key, 0);
        tt_entry->save(new_data);
        transposition_table.save_entry(tt_entry);
    }

    if (best_score >= beta)
    {
        return best_score;
    }

    alpha = std::max(alpha, best_score);

    movepick::Movepicker move_picker;
    // initialize the move picker

    types::Move best_move;
    best_move.null_();
    types::Move move;
    int         totalMoves = 0;

    while (!(move = move_picker.next(!inCheck || best_score > -search::MATE_FOUND)).is_null())
    {
        if (!movegen::isLegal(*pos, move))
        {
            continue;
        }

        totalMoves++;

        bool            has_pawns = false;
        board::Bitboard pawns     = pos->current_side == types::Color::WHITE ? pos->white_pawns : pos->black_pawns;

        if (pawns.board() != 0ULL)
        {
            has_pawns = true;
        }

        if (best_score > -search::MATE_FOUND && !inCheck && !has_pawns)
        {
            const int fut_base = ss->staticEval + 192;

            if (fut_base <= alpha && !search::SEE(*pos, move, 1))
            {
                best_score = std::max(fut_base, best_score);
                continue;
            }
        }

        ss->move = move;
        movegen::do_move(*pos, move);

        info->nodes++;
        const int score = -search::Quiescence<pvNode>(-beta, -alpha, thread_data, ss + 1);

        pos->undo_move();

        if (info->stopped)
        {
            return 0;
        }

        if (score > best_move.data())
        {
            best_score = score;

            if (score > alpha)
            {
                best_move = move;

                if (score >= beta)
                {
                    break;
                }

                alpha = score;
            }
        }
    }

    if (totalMoves == 0 && inCheck)
    {
        return -search::MATE_SCORE + ss->ply;

        int bound = best_score >= beta ? static_cast<int>(types::Bound::LOWER) : static_cast<int>(types::Bound::UPPER);

        tt::TT_data new_data(rawEval, bound, best_move.data(), best_score, pos->position_key, 0);
        tt_entry->save(new_data);
        transposition_table.save_entry(tt_entry);

        return best_score;
    }
}

}  // namespace search
}  // namespace Shahrazad