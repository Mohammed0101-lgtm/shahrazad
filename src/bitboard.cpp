#include "bitboard.h"
#include <iostream>

void Bitboard::set_bit(const int& pos) { bit_board |= (MASK << pos); }

void Bitboard::flip_bit(const int& pos) { bit_board ^= (MASK << pos); }

void Bitboard::clear_bit(const int& pos) { bit_board &= ~(MASK << pos); }

bool Bitboard::is_bitset(const int& pos) const {
    return (bit_board & (MASK << pos)) != 0;
}

uint64_t Bitboard::board() const { return bit_board; }

void     Bitboard::move_bit(const int& start_pos, const int& end_pos) {
    clear_bit(start_pos);
    set_bit(end_pos);
}

int Bitboard::square() const {
    for (int i = 0; i < 64; i++) {
        if (is_bitset(i)) {
            return i;
        }
    }

    return -1;
}

void Bitboard::_repr_fen_(const std::string& fen_pos) {
    int pos = 0;

    for (int i = 0; i < 64; i++) {
        if (fen_pos == board_pos[i]) {
            pos = i;
            break;
        }
    }

    flip_bit(pos);
}

void Bitboard::print() const {
    for (int i = 0; i < 64; i++) {
        if (is_bitset(i)) {
            std::cout << '1' << ' ';
        } else {
            std::cout << '0' << ' ';
        }

        if ((i + 1) % 8 == 0) {
            std::cout << '\n';
        }
    }
}

unsigned int Bitboard::count() const {
    int count = 0;

    for (int i = 0; i < 64; i++) {
        if (is_bitset(i)) {
            count++;
        }
    }
    return count;
}
