#ifndef SEARCH_H
#define SEARCH_H

#include <thread>

#include "move.h"
#include "position.h"

// global constants (I hate the word global but that's life)
inline constexpr unsigned int MAXSCORE      = 32670;
inline constexpr unsigned int MAX_DEPTH     = 256;
constexpr int                 MATE_SCORE    = 32000;
constexpr int                 CORRHIST_SIZE = 16384;
constexpr int                 MAXDEPTH      = 256;
constexpr int                 SCORE_NONE    = 32001;
constexpr int                 MATE_FOUND    = 31744;
constexpr int                 CH_MAX        = 16384;

// time spent by the thread (Life is short)
inline uint64_t GetTimeMs() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
}

// keep data about the current thread
struct SearchInfo {
    uint64_t starttime       = 0;
    uint64_t stoptimeBaseOpt = 0;
    uint64_t stoptimeOpt     = 0;
    uint64_t stoptimeMax     = 0;
    uint64_t nodes           = 0;
    uint64_t nodeslimit      = 0;
    int      depth           = -1;
    int      seldepth        = -1;
    int      movestogo       = -1;
    bool     timeset         = false;
    bool     nodeset         = false;
    bool     movetimeset     = false;
    bool     stopped         = false;
};

// data about the whole search tree
struct SearchData {
    int searchHis[2][64 * 64]      = {};
    int capHist[12 * 64][6]        = {};
    int counterMoves[64 * 64]      = {};
    int contHist[12 * 64][12 * 64] = {};
    int corrHist[2][CORRHIST_SIZE] = {};
};

// principle variation table
struct PvTable {
    int  pvLength[MAXDEPTH + 1];
    Move pvArray[MAXDEPTH + 1][MAXDEPTH + 1];
};

// data held in the thread about the position state
struct ThreadData {
    Position   pos;
    PvTable    pvTable;
    uint64_t   nodeSpentTable[64 * 64];
    int        id = 0;
    int        rootDepth;
    int        nmpPlies;
    SearchData search_data;
    SearchInfo info;
};

// self explanatory
struct SearchStack {
    int16_t staticEval;
    Move    excludedMove;
    Move    move;
    Move    searchKiller;
    int     ply;
    int     doubleExtensions;
    int (*contHistEntry)[12 * 64];
};

// the current search node of the tree
struct SearchNode {
    SearchNode* parent;
    Position    pos;
    Position**  children;
    int         val;
};

// done with this node
bool NodesOver(const SearchInfo* info) { return info->nodeset && info->nodes > info->nodeslimit; }

// thread ran out of time
bool TimeOver(const SearchInfo* info) {
    return NodesOver(info) || ((info->timeset || info->movetimeset) && ((info->nodes & 1023) == 1023) && GetTimeMs() > info->stoptimeMax);
}

// searching or just chilling
enum state
{
    idle,
    search_state
};

// threads allocated
inline std::vector<std::thread> threads;
inline std::vector<ThreadData>  threads_data;

// get node's bitboard
inline uint64_t get_nodes() {
    uint64_t nodes = 0ULL;
    for (auto& t : threads_data)
        nodes += t.info.nodes;

    return nodes;
}

// stop the thread for whatever reason
inline void thread_interrupt() {
    for (auto& t : threads_data)
        t.info.stopped = true;

    for (auto& t : threads)
        if (t.joinable())
            t.join();

    threads.clear();
}

/*-- note that yes these thread things should be in a seperate file but i did put them here temporarily to solve a circle include problem
 * --*/

#endif // SEARCH_H
