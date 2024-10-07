#ifndef TTABLE_H
#define TTABLE_H

#include "search.h"
#include "types.h"


// constants
constexpr int     BUCKET_SIZE    = 3;
constexpr size_t  MAX_TABLE_SIZE = 4000;
constexpr uint8_t MAX_AGE        = 1 << 5;
constexpr uint8_t AGE_MASK       = MAX_AGE - 1;

// structs
struct ttEntry;
struct ttBucket;
class TranspositionTable;

// the actual data of a single entry
struct TT_data {
  public:
    int16_t  eval;
    int16_t  bound;
    uint16_t move;
    int8_t   value;
    uint16_t pos_key;
    uint8_t  depth;

    TT_data() {};

    TT_data(int16_t eval, int16_t bound, uint16_t move, int8_t value, uint16_t pos_key, uint8_t depth) {
        this->eval    = eval;
        this->bound   = bound;
        this->move    = move;
        this->value   = value;
        this->pos_key = pos_key;
        this->depth   = depth;
    }
};

// entry in the TT table
struct ttEntry {
  private:
    TT_data data;

  public:
    friend class TranspositionTable;

    TT_data read() const;

    void    save(TT_data& d);
};

// bucket of entries can contain up to BUCKET_SIZE entries
struct ttBucket {
    ttEntry  entries[BUCKET_SIZE] = {};
    uint16_t age; // for replacement decision
};

class TranspositionTable
{
  public:
    ~TranspositionTable() {
        // This would only work with posix complient systems
        // otherwise you would need alligned allocation and freeing
        free(table);
    }

    ttEntry* probe(const uint64_t key) const;

    void     save_entry(const ttEntry* entry);

  private:
    friend struct ttEntry;

    size_t    capacity; // remember to initialize this variable
    size_t    table_size  = 0;
    ttBucket* table       = nullptr;
    uint8_t   generations = 0;
};

// just for the overhead
static_assert(sizeof(TT_data) == 10);
static_assert(sizeof(ttEntry) == 32);

#endif // TTABLE_H
