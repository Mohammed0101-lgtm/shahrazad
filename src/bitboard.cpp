#include "bitboard.h"
#include <iostream>

void Bitboard::set_bit(const uint8_t& pos) { bit_board |= (MASK << pos); }

void Bitboard::flip_bit(const uint8_t& pos) { bit_board ^= (MASK << pos); }

void Bitboard::clear_bit(const uint8_t& pos) { bit_board &= ~(MASK << pos); }

bool Bitboard::is_bitset(const uint8_t& pos) const { return (bit_board & (MASK << pos)) != 0; }

uint64_t Bitboard::board() const { return bit_board; }

void Bitboard::move_bit(const uint8_t& start_pos, const uint8_t& end_pos) {
    clear_bit(start_pos);
    set_bit(end_pos);
}

uint8_t Bitboard::square() const {
    for (int i = 0; i < 64; i++)
    {
        if (is_bitset(i))
        {
            return i;
        }
    }

    return -1;
}

void Bitboard::print() const {
    for (int i = 0; i < 64; i++)
    {
        if (is_bitset(i))
        {
            std::cout << '1' << ' ';
        }
        else
        {
            std::cout << '0' << ' ';
        }

        if ((i + 1) % 8 == 0)
        {
            std::cout << '\n';
        }
    }
}

uint8_t Bitboard::count() const {
    int count = 0;

    for (int i = 0; i < 64; i++)
    {
        if (is_bitset(i))
        {
            count++;
        }
    }
    return count;
}
