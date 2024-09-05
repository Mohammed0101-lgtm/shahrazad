#ifndef EVAL_H
#define EVAL_H

#include "position.h"
#include <cassert>

int simple_evaluate(const Position& pos) {
    int color = pos.getSide();
    int eval = 0;

    eval += pos.numberOf(PAWN  , color) * PAWN_VAL;
    eval += pos.numberOf(KNIGHT, color) * KNIGHT_VAL;
    eval += pos.numberOf(BISHOP, color) * BISHOP_VAL;
    eval += pos.numberOf(ROOK  , color) * ROOK_VAL;
    eval += pos.numberOf(QUEEN , color) * QUEEN_VAL;

    return eval;
}

int network_eval(const Position& pos, NNue network, NNue::Accumulator<size>& caches) {
    assert(!pos.inCheck);

    bool use_smallnet = false;
    int  simple_eval  = simple_evaluate(pos);

    if (std::abs(simple_eval) > 962)
        return network.nnue_eval(pos, caches);

    return simple_eval;
}

#endif // EVAL_H
