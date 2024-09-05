#include <iostream>
#include "bitboard.h"

/*
int main(void) {
	Bitboard knight;

	int pos = 28;
	knight.set_bit(pos);
	
	Bitboard opp;
	Bitboard own;
	own.set_bit(13);
	std::vector<move_t> moves = knight.knight_moves(knight.square(), own, opp); 	
	for (const auto& pos : moves) {
		knight.set_bit(static_cast<int>(pos));
	}

	knight.print();

	Bitboard b;
	b.set_bit(13);
	std::cout << std::endl;
	b.print();

	return 0;
}
*/