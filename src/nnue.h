#ifndef NNUE_H
#define NNUE_H

#include "bitboard.h"
#include "position.h"
#include "types.h"
#include <cassert>
#include <vector>

const size_t size = 768;

int          feature_index(
             int king_square, int square, int color, pieceType piece_type) {
    int p_idx = piece_type * 2 + color;
    return (square + (p_idx + king_square * 10)) * 64;
}

class LinearLayer
{
  private:
    std::vector<std::vector<int16_t>> weights;
    std::vector<int16_t>              outputs;
    std::vector<int16_t>              inputs;
    std::vector<int16_t>              biases;

    int                               output_dims;
    int                               input_dims;
    double                            eta;

  public:
    LinearLayer() {}

    LinearLayer(int input_size, int output_size, double lr);

    int                  get_num_outputs() const;

    int                  get_num_inputs() const;

    std::vector<int16_t> getWeights(const int index) const;

    std::vector<int16_t> getBias() const;

    std::vector<int16_t> feedForward(const std::vector<int16_t>& input);

    std::vector<int16_t> backPropagate(const std::vector<int16_t>& grad);
};

const int MAX_INPUT_SIZE = 768;
const int L1_SIZE        = 1024;
const int NUM_OUTPUTS    = 8;

class NNue
{
  public:
    template <size_t size> struct alignas(64) Accumulator {
        int16_t  accumulation[2][size];

        int16_t* operator[](int color) {
            assert(color == WHITE || color == BLACK);
            return accumulation[color];
        }

        const uint16_t* operator[](int color) const {
            assert(color == WHITE || color == BLACK);
            return accumulation[color];
        }
    };

    LinearLayer l_0;

    LinearLayer l_1;

    LinearLayer l_2;

    void        refresh_accumulator(
               const LinearLayer& layer, const std::vector<int>& active_features,
               int side);

    void update_accumulator(
        const LinearLayer& layer, const std::vector<int>& removed_features,
        const std::vector<int>& added_features, int perspective);

    std::vector<int16_t> clipped_relu(
        std::vector<int16_t> output, const std::vector<int16_t> input) const;

    std::vector<int16_t> linear(
        const LinearLayer& layer, std::vector<int16_t>& output,
        const std::vector<int16_t>& input) const;

    float
    nnue_eval(const Position& pos_1, NNue::Accumulator<size>& caches) const;
};

extern NNue                    nnue = NNue();
extern NNue::Accumulator<size> caches;

int feature_key(int king_square, pieceType piece_type, int square, int color);

std::vector<int> get_active_features(const Position& pos, int color);

std::vector<int> get_removed_features(
    const Position& cur_pos, const Position& prev_pos, int color);

std::vector<int> get_added_features(
    const Position& cur_pos, const Position& prev_pos, int color);

#endif // NNUE_H