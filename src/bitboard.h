#pragma once

#include "types.h"

class Bitboard {
   protected:
    uint64_t bit_board;

   public:
    Bitboard() :
        bit_board(0) {}
    Bitboard(uint64_t val) :
        bit_board(val) {}

    uint64_t board() const;

    uint8_t square() const;

    bool is_bitset(const uint8_t& pos) const;

    void flip_bit(const uint8_t& pos);

    void set_bit(const uint8_t& pos);

    void clear_bit(const uint8_t& pos);

    void move_bit(const uint8_t& start_pos, const uint8_t& end_pos);

    void print() const;

    uint8_t count() const;
};
