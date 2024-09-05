#ifndef THREAD_H
#define THREAD_H

#include "search.h"
#include <thread>


bool TimeOver(const SearchInfo* info) {
    // check if more than Maxtime passed and we have to stop
    return NodesOver(info) || ((info->timeset || info->movetimeset)
                               && ((info->nodes & 1023) == 1023)
                               && GetTimeMs() > info->stoptimeMax);
}

bool NodesOver(const SearchInfo* info) {
    // check if we used all the nodes/movetime we had or if we used more than our lowerbound of time
    return info->nodeset && info->nodes > info->nodeslimit;
}


enum state {
    idle,
    search
};

inline std::vector<std::thread> threads;
inline std::vector<ThreadData> threads_data;

inline uint64_t get_nodes() {
    uint64_t nodes = 0ULL;
    for (auto& t : threads_data) {
        nodes += t.info.nodes;
    }

    return nodes;
}

inline void thread_interrupt() {
    for (auto& t : threads_data) {
        t.info.stopped = true;
    }

    for (auto& t : threads) {
        if (t.joinable()) {
            t.join();
        }
    }

    threads.clear();
}

#endif // THREAD_H
