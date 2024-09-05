#ifndef SEARCH_H
#define SEARCH_H

#include "position.h"

inline constexpr unsigned int MAXSCORE      = 32670;
inline constexpr unsigned int MAX_DEPTH     = 256;
constexpr int                 MATE_SCORE    = 32000;
constexpr int                 CORRHIST_SIZE = 16384;
constexpr int                 MAXDEPTH      = 256;
constexpr int                 SCORE_NONE    = 32001;
constexpr int                 MATE_FOUND    = 31744;


inline uint64_t GetTimeMs() {
    return  std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count();
}

struct ScoredMove {
    Move move;
    int score;

    ScoredMove() : move(0) { 
        score = 0; 
    } 
};

struct MoveList {
    ScoredMove moves[256];
    int count = 0;

    MoveList() {
        this->count = 0;
    }
};

typedef struct {
    uint64_t starttime        =   0;
    uint64_t stoptimeBaseOpt  =   0;
    uint64_t stoptimeOpt      =   0;
    uint64_t stoptimeMax      =   0;
    uint64_t nodes            =   0;
    uint64_t nodeslimit       =   0;
    int depth                 =  -1;
    int seldepth              =  -1;
    int movestogo             =  -1;
    bool timeset              =  false;
    bool nodeset              =  false;
    bool movetimeset          =  false;
    bool stopped              =  false;
}SearchInfo;

typedef struct {
    int searchHis[2][64 * 64]      = {};
    int capHist[12 * 64][6]        = {};
    int counterMoves[64 * 64]      = {};
    int contHist[12 * 64][12 * 64] = {};
    int corrHist[2][CORRHIST_SIZE] = {};
}SearchData;

struct PvTable {
    int  pvLength[MAXDEPTH + 1];
    Move pvArray[MAXDEPTH + 1][MAXDEPTH + 1];
};

typedef struct {
    Position pos;
    PvTable  pvTable;
    uint64_t nodeSpentTable[64 * 64];

    int id = 0;
    int rootDepth;
    int nmpPlies;

    SearchData search_data;
    SearchInfo info;
}ThreadData;

typedef struct {
    int16_t staticEval;

    Move excludedMove;
    Move move;
    Move searchKiller;

    int ply;
    int doubleExtensions;
    int (*contHistEntry)[12 * 64];
}SearchStack;

struct SearchNode {
    SearchNode *parent;
    Position    pos;
    Position  **children;
    int         val;
};

typedef struct SearchNode SearchNode;

#endif // SEARCH_H
