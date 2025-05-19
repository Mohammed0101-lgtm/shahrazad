#pragma once

#include "move.h"
#include "search.h"


namespace Shahrazad {
namespace movepick {

class Movepicker {
   public:
    position::Position   pos;
    search::SearchData*  search_data;
    search::SearchStack* ss;
    movegen::MoveList    move_list;
    types::Move          tt_move;
    types::Move          killer;
    types::Move          count;
    int                  index;
    int                  stage;

    Movepicker() = default;

    types::Move select(position::Position& pos);
    types::Move next(const bool skip);
};

}  // namespace movepick
}  // namespace Shahrazad