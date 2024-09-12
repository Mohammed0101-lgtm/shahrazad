#ifndef MOVEPICKER_H
#define MOVEPICKER_H

#include "move.h"
#include "search.h"

class Movepicker
{
  public:
    Position     pos;
    SearchData*  search_data;
    SearchStack* ss;
    MoveList     move_list;
    Move         tt_move;
    Move         killer;
    Move         count;
    int          index;
    int          stage;

    Movepicker() {}

    Move select(Position& pos);
    Move next(const bool skip);
};

#endif // MOVEPICKER_H