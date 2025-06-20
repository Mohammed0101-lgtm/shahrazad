#include "ttable.h"


namespace Shahrazad {
namespace tt {

// hashing func for indexing
uint64_t hash(const int16_t pos_key) {
    uint64_t h   = 5381;
    uint64_t key = static_cast<uint64_t>(pos_key);
    h            = ((h << 5) + h) + key;
    return h;
}

// get data from entry
inline TT_data TT_Entry::read() const {
    return TT_data(data.eval, data.bound, data.move, data.value, data.pos_key, data.depth);
}

// save data in entry
inline void TT_Entry::save(TT_data& d) { data = TT_data(d); }

// get entry from table (as in looking around)
TT_Entry* TranspositionTable::probe(const uint64_t key) const {
    int        _hash   = hash(static_cast<int16_t>(key));
    int        index   = _hash % capacity;
    TT_Bucket* cluster = &table[index];

    for (int i = 0; i < BUCKET_SIZE; i++)
    {
        if (cluster->entries[i].data.pos_key == key)
            return &cluster->entries[i];
    }

    return nullptr;
}

void TranspositionTable::save_entry(const TT_Entry* entry) {
    int        _hash   = hash(static_cast<int16_t>(entry->read().pos_key));
    TT_Bucket* cluster = &table[_hash % capacity];
    // We have to do some checking for the entry age to decide the replacement
    if (cluster)
        cluster->entries[0] = *entry;
}

}  // namespace tt
}  // namespace Shahrazad