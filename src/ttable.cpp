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
    return TT_data(data.eval, data.bound, data.move, data.value, data.pos_key, data.depth);
}

// save data in entry
void ttEntry::save(TT_data& d) { data = TT_data(d); }

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

void TranspositionTable::save_entry(const ttEntry* entry) {
    int       _hash   = hash(static_cast<int16_t>(entry->read().pos_key));
    ttBucket* cluster = &table[_hash % capacity];
    // We have to do some checking for the entry age to decide the replacement
    if (cluster) {
        cluster->entries[0] = *entry;
    }
}
