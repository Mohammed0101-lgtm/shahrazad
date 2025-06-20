#pragma once

#include <thread>

#include "move.h"
#include "position.h"


namespace Shahrazad {
namespace search {

// global constants (I hate the word global but that's life)
inline constexpr uint8_t MAXSCORE      = 32670;
inline constexpr uint8_t MAX_DEPTH     = 256;
constexpr uint8_t        MATE_SCORE    = 32000;
constexpr uint8_t        CORRHIST_SIZE = 16384;
constexpr uint8_t        MAXDEPTH      = 256;
constexpr uint8_t        SCORE_NONE    = 32001;
constexpr uint8_t        MATE_FOUND    = 31744;
constexpr uint8_t        CH_MAX        = 16384;


// keep data about the current thread
struct SearchInfo {
    uint64_t starttime       = 0;
    uint64_t stoptimeBaseOpt = 0;
    uint64_t stoptimeOpt     = 0;
    uint64_t stoptimeMax     = 0;
    uint64_t nodes           = 0;
    uint64_t nodeslimit      = 0;
    uint8_t  depth           = -1;
    uint8_t  seldepth        = -1;
    uint8_t  movestogo       = -1;
    bool     timeset         = false;
    bool     nodeset         = false;
    bool     movetimeset     = false;
    bool     stopped         = false;
};

// data about the whole search tree
struct SearchData {
    uint8_t searchHis[2][64 * 64]      = {};
    uint8_t capHist[12 * 64][6]        = {};
    uint8_t counterMoves[64 * 64]      = {};
    uint8_t contHist[12 * 64][12 * 64] = {};
    uint8_t corrHist[2][CORRHIST_SIZE] = {};
};

// principle variation table
struct PvTable {
    uint8_t     pvLength[MAXDEPTH + 1];
    types::Move pvArray[MAXDEPTH + 1][MAXDEPTH + 1];
};

// self explanatory
struct SearchStack {
    int16_t     staticEval;
    types::Move excludedMove;
    types::Move move;
    types::Move searchKiller;
    uint8_t     ply;
    uint8_t     doubleExtensions;
    uint8_t (*contHistEntry)[12 * 64];
};

// the current search node of the tree
struct SearchNode {
    SearchNode*          parent;
    position::Position   pos;
    position::Position** children;
    uint8_t              val;
};


// searching or just chilling
enum class state : int {
    IDLE,
    SEARCH_STATE
};

}  // namespace search
}  // namespace Shahrazad