#pragma once

#include "nnue.h"
#include "position.h"
#include <cassert>

namespace Shahrazad {
namespace eval {

int simple_evaluate(const position::Position& pos) {
    types::Color color = pos.getSide();
    int     eval  = 0;

    eval += pos.numberOf(types::PieceType::PAWN, color) * types::PAWN_VAL;
    eval += pos.numberOf(types::PieceType::KNIGHT, color) * types::KNIGHT_VAL;
    eval += pos.numberOf(types::PieceType::BISHOP, color) * types::BISHOP_VAL;
    eval += pos.numberOf(types::PieceType::ROOK, color) * types::ROOK_VAL;
    eval += pos.numberOf(types::PieceType::QUEEN, color) * types::QUEEN_VAL;

    return eval;
}

int network_eval(const position::Position& pos, nnue::NNue network, nnue::NNue::Accumulator<nnue::size>& caches) {
    assert(!pos.inCheck);

    bool use_smallnet = false;
    int  simple_eval  = simple_evaluate(pos);

    if (std::abs(simple_eval) > 962)
    {
        return network.nnue_eval(pos, caches);
    }

    return simple_eval;
}

}  // namespace eval
}  // namespace Shahrazad