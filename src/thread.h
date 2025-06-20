#pragma once

#include <thread>
#include "search.h"


namespace Shahrazad {
namespace thread {

// time spent by the thread (Life is short)
inline uint64_t GetTimeMs() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch())
      .count();
}

// data held in the thread about the position state
struct ThreadData {
    position::Position pos;
    search::PvTable    pvTable;
    uint64_t           nodeSpentTable[64 * 64];
    uint8_t            id = 0;
    uint8_t            rootDepth;
    uint8_t            nmpPlies;
    search::SearchData search_data;
    search::SearchInfo info;
};

// done with this node
inline bool NodesOver(const search::SearchInfo* info) { return info->nodeset && info->nodes > info->nodeslimit; }

// thread ran out of time
inline bool TimeOver(const search::SearchInfo* info) {
    return NodesOver(info)
        || ((info->timeset || info->movetimeset) && ((info->nodes & 1023) == 1023) && GetTimeMs() > info->stoptimeMax);
}

// threads allocated
std::vector<std::thread> threads;
std::vector<ThreadData>  threads_data;

// get node's bitboard
inline uint64_t get_nodes() {
    uint64_t nodes = 0ULL;
    for (auto& t : threads_data)
    {
        nodes += t.info.nodes;
    }

    return nodes;
}

// stop the thread for whatever reason
void thread_interrupt() {
    for (auto& t : threads_data)
    {
        t.info.stopped = true;
    }

    for (auto& t : threads)
    {
        if (t.joinable())
        {
            t.join();
        }
    }

    threads.clear();
}

}  // namespace thread
}  // namespace Shahrazad