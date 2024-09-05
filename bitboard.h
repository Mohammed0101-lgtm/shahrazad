#ifndef BITBOARD_H
#define BITBOARD_H

#include "types.h"

class Bitboard {
private:

    uint64_t bit_board;

public:
    
    Bitboard() : bit_board(0) {}
    Bitboard(uint64_t val) : bit_board(val) {}
    
    uint64_t board() const;
    
    int square() const;
    
    bool is_bitset(const int& pos) const;
    
    void flip_bit(const int& pos);
    
    void set_bit(const int &pos);
    
    void clear_bit(const int &pos);
    
    void move_bit(const int &start_pos, const int &end_pos);
    
    void _repr_fen_(const std::string& fen_pos);
    
    void print() const;
    
    unsigned int count() const;
    
};

#endif // BITBOARD_H