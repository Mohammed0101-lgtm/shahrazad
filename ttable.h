#ifndef TTABLE_H
#define TTABLE_H

#include "search.h"
#include "types.h"

constexpr int     BUCKET_SIZE    = 3;
constexpr size_t  MAX_TABLE_SIZE = 4000;
constexpr uint8_t MAX_AGE        = 1 << 5;
constexpr uint8_t AGE_MASK       = MAX_AGE - 1;

struct ttEntry;
struct ttBucket;
class TranspositionTable;

struct TT_data {
public:
    int16_t  eval; 
    int16_t  bound;
    uint16_t move;
    int8_t   value;
    uint16_t pos_key; 
    uint8_t  depth;
};

struct ttEntry {
private:    
    TT_data data;
public:
    friend class TranspositionTable;

    TT_data read() const;

    void save(TT_data& d);

};

struct ttBucket {
    ttEntry  entries[BUCKET_SIZE] = {};
    uint16_t age;
};

class TranspositionTable {
public:
    ~TranspositionTable() { 
        // This would only work with posix complient systems
        // otherwise you would need alligned allocation and freeing
        free(table); 
    }
    
    ttEntry *probe(const uint64_t key) const;
    
private:
    friend struct ttEntry;

    size_t    capacity; // remember to initialize this variable
    size_t    table_size  = 0; 
    ttBucket *table       = nullptr;
    uint8_t   generations = 0;
};


#endif // TTABLE_H
