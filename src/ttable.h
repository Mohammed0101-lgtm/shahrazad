#pragma once

#include "search.h"
#include "types.h"


namespace Shahrazad {
namespace tt {

// constants
constexpr int         BUCKET_SIZE    = 3;
constexpr std::size_t MAX_TABLE_SIZE = 4000;
constexpr uint8_t     MAX_AGE        = 1 << 5;
constexpr uint8_t     AGE_MASK       = MAX_AGE - 1;

// structs
struct TT_Entry;
struct TT_Bucket;
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
struct TT_Entry {
   protected:
    TT_data data;

   public:
    friend class TranspositionTable;

    TT_data read() const;

    void save(TT_data& d);
};

// bucket of entries can contain up to BUCKET_SIZE entries
struct TT_Bucket {
    TT_Entry entries[BUCKET_SIZE] = {};
    uint16_t age;  // for replacement decision
};

class TranspositionTable {
   public:
    ~TranspositionTable() {
        // This would only work with posix complient systems
        // otherwise you would need alligned allocation and freeing
        free(table);
    }

    TT_Entry* probe(const uint64_t key) const;

    void save_entry(const TT_Entry* entry);

   protected:
    friend struct TT_Entry;

    std::size_t capacity;  // remember to initialize this variable
    std::size_t table_size  = 0;
    TT_Bucket*  table       = nullptr;
    uint8_t     generations = 0;
};

}  // namespace tt
}  // namespace Shahrazad