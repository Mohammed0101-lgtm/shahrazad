#ifndef TYPES_H
#define TYPES_H

#include <cstdint>
#include <string>

inline float max(float a, float b) { return a > b ? a : b; }
inline float min(float a, float b) { return a < b ? a : b; }

// static piece values
inline static const float    QUEEN_VAL  = 9.00;
inline static const float    ROOK_VAL   = 5.00;
inline static const float    BISHOP_VAL = 3.10;
inline static const float    PAWN_VAL   = 1.00;
inline static const float    KNIGHT_VAL = 3.00;
inline static const uint64_t MASK       = 1ULL;

inline int reductions[2][64][64];
inline int lmp_margin[64][2];
inline int see_margin[64][2];

constexpr int SEEval[15] = {100, 422, 422, 642, 1015, 0, 100, 422, 422, 642, 1015, 0, 0, 0, 0};

typedef enum {
    KING,
    QUEEN,
    ROOK,
    BISHOP,
    KNIGHT,
    PAWN,
    NOPE
} pieceType;

enum {
    CAPTURE,
    EN_PASSANT,
    CASTLE,
    QUIET
};

class Move {
   protected:
    uint16_t m_Move;

   public:
    Move() = default;

    Move(uint16_t data) :
        m_Move(data) {}

    Move(uint32_t from, uint32_t to, uint32_t flags) {
        m_Move = ((flags & 0xf) << 12) | ((from & 0x3f) << 6) | (to & 0x3f);
    }

    uint32_t getTo() const { return m_Move & 0x3f; }

    uint32_t getFrom() const { return (m_Move >> 6) & 0x3f; }

    uint32_t getFlags() const { return (m_Move >> 12) & 0x0f; }

    void setTo(uint32_t to) {
        m_Move &= ~0x3f;
        m_Move |= to & 0x3f;
    }

    void setFrom(uint32_t from) {
        m_Move &= ~0xfc0;
        m_Move |= (from & 0x3f) << 6;
    }

    constexpr bool is_ok() const { return none().m_Move != m_Move && null().m_Move != m_Move; }

    static constexpr Move null() { return Move(65); }
    static constexpr Move none() { return Move(0); }

    bool isCapture() const { return getFlags() == CAPTURE; }

    bool hasFlag(uint32_t flag) const { return getFlags() == flag; }

    unsigned int getButterflyIndex() const { return m_Move & 0x0fff; }

    bool operator==(Move a) const { return (m_Move & 0xffff) == (a.m_Move & 0xffff); }

    bool operator!=(Move a) const { return (m_Move & 0xffff) != (a.m_Move & 0xffff); }

    constexpr explicit operator bool() const { return m_Move != 0; }

    constexpr uint16_t data() const { return static_cast<uint16_t>(m_Move); }
};

// encode sides
enum {
    WHITE,
    BLACK,
    BOTH
};

enum {
    NO_BOUND,
    UPPER,
    LOWER,
    EXACT
};

enum Square : int {
    a1,
    b1,
    c1,
    d1,
    e1,
    f1,
    g1,
    h1,
    a2,
    b2,
    c2,
    d2,
    e2,
    f2,
    g2,
    h2,
    a3,
    b3,
    c3,
    d3,
    e3,
    f3,
    g3,
    h3,
    a4,
    b4,
    c4,
    d4,
    e4,
    f4,
    g4,
    h4,
    a5,
    b5,
    c5,
    d5,
    e5,
    f5,
    g5,
    h5,
    a6,
    b6,
    c6,
    d6,
    e6,
    f6,
    g6,
    h6,
    a7,
    b7,
    c7,
    d7,
    e7,
    f7,
    g7,
    h7,
    a8,
    b8,
    c8,
    d8,
    e8,
    f8,
    g8,
    h8,
    NONE,
};

inline int diagonal_first[] = {
  1, 2,  3,  4,  5, 6, 7, 8, 2,  3,  4,  5,  6, 7, 8, 9,  3,  4,  5,  6,  7, 8,
  9, 0,  4,  5,  6, 7, 8, 9, 0,  10, 5,  6,  7, 8, 9, 0,  10, 11, 6,  7,  8, 9,
  0, 10, 11, 12, 7, 8, 9, 0, 10, 11, 12, 13, 8, 9, 0, 10, 11, 12, 13, 14,
};

inline int diagonal_second[] = {
  0, 1, 2,  3, 4,  5,  6,  7,  8, 0, 1,  2,  3,  4,  5,  6,  9,  8, 0,  1,  2,  3,
  4, 5, 10, 9, 8,  0,  1,  2,  3, 4, 11, 10, 9,  8,  0,  1,  2,  3, 12, 11, 10, 9,
  8, 0, 1,  2, 13, 12, 11, 10, 9, 8, 0,  1,  14, 13, 12, 11, 10, 9, 8,  0,
};

inline int ranks[] = {0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2,
                      2, 2, 3, 3, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4, 5, 5, 5, 5,
                      5, 5, 5, 5, 6, 6, 6, 6, 6, 6, 6, 6, 7, 7, 7, 7, 7, 7, 7, 7};

inline int files[] = {0, 1, 2, 3, 4, 5, 6, 7, 0, 1, 2, 3, 4, 5, 6, 7, 0, 1, 2, 3, 4, 5,
                      6, 7, 0, 1, 2, 3, 4, 5, 6, 7, 0, 1, 2, 3, 4, 5, 6, 7, 0, 1, 2, 3,
                      4, 5, 6, 7, 0, 1, 2, 3, 4, 5, 6, 7, 0, 1, 2, 3, 4, 5, 6, 7};

const std::string board_pos[] = {
  "a1", "b1", "c1", "d1", "e1", "f1", "g1", "h1", "a2", "b2", "c2", "d2", "e2", "f2", "g2", "h2",
  "a3", "b3", "c3", "d3", "e3", "f3", "g3", "h3", "a4", "b4", "c4", "d4", "e4", "f4", "g4", "h4",
  "a5", "b5", "c5", "d5", "e5", "f5", "g5", "h5", "a6", "b6", "c6", "d6", "e6", "f6", "g6", "h6",
  "a7", "b7", "c7", "d7", "e7", "f7", "g7", "h7", "a8", "b8", "c8", "d8", "e8", "f8", "g8", "h8"};

#endif  // TYPES_H
