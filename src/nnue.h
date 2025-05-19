#pragma once

#include "bitboard.h"
#include "position.h"
#include "types.h"
#include <cassert>
#include <vector>


namespace Shahrazad {
namespace nnue {

const std::size_t size = 768;


class LinearLayer {
   private:
    int16_t** weights;
    uint32_t  weights_dimensions[2];

    std::vector<int16_t> outputs;
    std::vector<int16_t> inputs;
    std::vector<int16_t> biases;

    int    output_dims;
    int    input_dims;
    double eta;

   public:
    LinearLayer() {}
    LinearLayer(int input_size, int output_size, double lr = 0.01);

    int get_num_outputs() const;
    int get_num_inputs() const;
    int16_t* getWeights(const int index) const;
    std::vector<int16_t> getBias() const;
    std::vector<int16_t> feedForward(const std::vector<int16_t>& input);
    std::vector<int16_t> backPropagate(const std::vector<int16_t>& grad);
};

const int MAX_INPUT_SIZE = 768;
const int L1_SIZE        = 1024;
const int NUM_OUTPUTS    = 8;

class NNue {
   public:
    template<std::size_t size>
    struct alignas(64) Accumulator {
        int16_t accumulation[2][size];

        int16_t* operator[](types::Color color) {
            assert(color == WHITE || color == BLACK);
            return accumulation[color];
        }

        const uint16_t* operator[](types::Color color) const {
            assert(color == WHITE || color == BLACK);
            return accumulation[color];
        }
    };

    LinearLayer l_0;
    LinearLayer l_1;
    LinearLayer l_2;

    void                 refresh_accumulator(const LinearLayer& layer, const std::vector<uint32_t>& active_features,
                                             types::Color perspective);
    void                 update_accumulator(const LinearLayer& layer, const std::vector<uint32_t>& removed_features,
                                            const std::vector<uint32_t>& added_features, types::Color perspective);
    std::vector<int16_t> clipped_relu(std::vector<int16_t> output, const std::vector<int16_t> input) const;
    std::vector<int16_t> linear(const LinearLayer& layer, std::vector<int16_t>& output,
                                const std::vector<int16_t>& input) const;
    float                nnue_eval(const position::Position& pos_1, NNue::Accumulator<size>& caches) const;
};

extern NNue                    nnue = NNue();
extern NNue::Accumulator<size> caches;

// useful methods
int feature_index(types::Square king_square, types::Square square, types::Color color, types::PieceType piece_type) {
    int p_idx = static_cast<int>(piece_type) * 2 + static_cast<int>(color);
    return (static_cast<int>(square) + (p_idx + static_cast<int>(king_square) * 10)) * 64;
}

uint32_t feature_key(types::Square king_square, types::PieceType piece_type, types::Square square, types::Color color);
std::vector<uint32_t> get_active_features(const position::Position& pos, types::Color color);
std::vector<uint32_t> get_removed_features(const position::Position& cur_pos, const position::Position& prev_pos,
                                           types::Color color);
std::vector<uint32_t> get_added_features(const position::Position& cur_pos, const position::Position& prev_pos,
                                         types::Color color);

}  // namespace nnue
}  // namespace Shahrazad