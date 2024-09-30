#include "ttable.h"

// hashing func for indexing
unsigned long long hash(const int16_t pos_key) {
    unsigned long long h   = 5381;
    unsigned long long key = static_cast<unsigned long long>(pos_key);

    h                      = ((h << 5) + h) + key;

    return h;
}

// get data from entry
TT_data ttEntry::read() const {
    TT_data d;

    d.bound   = data.bound;
    d.depth   = data.depth;
    d.eval    = data.eval;
    d.move    = data.eval;
    d.pos_key = data.pos_key;
    d.value   = data.value;

    return d;
}

// save data in entry
void ttEntry::save(TT_data& d) {
    unsigned long long _hash = hash(static_cast<uint16_t>(data.pos_key));

    data.bound    = int16_t(d.bound);
    data.depth    = uint8_t(d.depth);
    data.eval     = int16_t(d.eval);
    data.move     = uint16_t(d.move);
    data.pos_key  = uint16_t(d.pos_key);
    data.value    = int8_t(d.value);
}

// get entry from table (as in looking around)
ttEntry* TranspositionTable::probe(const uint64_t key) const {
    int       _hash   = hash(static_cast<int16_t>(key));
    int       index   = _hash % capacity;
    ttBucket* cluster = &table[index];

    for (int i = 0; i < BUCKET_SIZE; i++) {
        if (cluster->entries[i].data.pos_key == key) {
            return &cluster->entries[i];
        }
    }

    return nullptr;
}
