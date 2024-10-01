#include "nnue.h"

#include "position.h"
#include <arm_neon.h>
#include <cassert>
#include <cstdint>
#include <math.h>
#include <vector>

#define __builtin_neon_vld1_v
#define __builtin_neon_vld1q_v
#define __builtin_neon_vst1_v
#define __builtin_neon_vst1q_v


int feature_key(int king_square, pieceType piece_type, int square, int color) {
    int p_idx = piece_type * 2 + color;
    return (square + (p_idx + king_square * 10)) * 64;
}

std::vector<int> get_active_features(const Position& pos, int color) {
    std::vector<int> active_features;
    Bitboard         occupancy = color == WHITE ? pos._white_occupancy() : pos._black_occupancy();

    for (int sq = 0; sq < 64; sq++) {
        if (!occupancy.is_bitset(sq)) {
            continue;
        }

        int key = feature_key(pos.king_square(color), pos.pieceOn(sq), sq, color);
        active_features.push_back(key);
    }
    return active_features;
}

std::vector<int> get_added_features(const Position& cur_pos, const Position& prev_pos, int color) {
    std::vector<int> active_features = get_active_features(cur_pos, color);
    std::vector<int> prev_features   = get_active_features(prev_pos, color);
    const int        size            = active_features.size();

    std::vector<int> result;

    for (int i = 0; i < size; i++) {
        if (std::find(prev_features.begin(), prev_features.end(), active_features[i]) ==
            prev_features.end()) {
            result.push_back(active_features[i]);
        }
    }
    return result;
}

std::vector<int> get_removed_features(const Position& cur_pos, const Position& prev_pos, int color) {
    std::vector<int> active_features = get_active_features(cur_pos, color);
    std::vector<int> prev_features   = get_active_features(prev_pos, color);
    const int        size            = prev_features.size();
    std::vector<int> result;

    for (int i = 0; i < size; i++) {
        if (std::find(active_features.begin(), active_features.end(), prev_features[i]) ==
            prev_features.end()) {
            result.push_back(prev_features[i]);
        }
    }

    return result;
}

std::vector<int16_t> LinearLayer::getWeights(const int index) const {
    assert(index >= 0 && index < weights.size());
    return weights[index];
}

int                  LinearLayer::get_num_outputs() const { return outputs.size(); }

int                  LinearLayer::get_num_inputs() const { return inputs.size(); }

std::vector<int16_t> LinearLayer::getBias() const { return biases; }

LinearLayer::LinearLayer(int input_size, int output_size, double lr) {
    assert(input_size > 0);
    assert(output_size > 0);

    output_dims = output_size;
    input_dims  = input_size;
    eta         = lr;

    for (int i = 0; i < output_size; i++) {
        weights.push_back(std::vector<int16_t>());

        for (int j = 0; j <= input_size; j++) {
            weights.back().push_back((float)rand() / RAND_MAX);
        }
    }
}

std::vector<int16_t> LinearLayer::feedForward(const std::vector<int16_t>& input) {
    assert(input.size() == input_dims);

    outputs = std::vector<int16_t>();
    inputs  = input;

    for (int i = 0; i < output_dims; i++) {
        double sum = 0.0;

        for (int w = 0; w < input_dims; w++) {
            sum += weights[i][w] * input[w];
        }

        sum += weights[i][input_dims];
        outputs.push_back(sum);
    }

    return outputs;
}

std::vector<int16_t> LinearLayer::backPropagate(const std::vector<int16_t>& grad) {
    assert(grad.size() == output_dims);

    std::vector<int16_t> prev_layer_grad;

    for (int i = 0; i < input_dims; i++) {
        float g = 0.0;

        for (int j = 0; j < output_dims; j++) {
            g += grad[j] * weights[j][i];
        }

        prev_layer_grad.push_back(g);
    }

    for (int i = 0; i < output_dims; i++) {
        for (int j = 0; j < input_dims; j++) {
            weights[i][j] -= (eta * grad[i] * inputs[j]);
        }

        weights[i][input_dims] -= eta * grad[i];
    }

    return prev_layer_grad;
}

std::vector<int16_t> NNue::linear(
    const LinearLayer& layer, std::vector<int16_t>& output, const std::vector<int16_t>& input) const {

    for (int i = 0; i < layer.get_num_outputs(); i++) {
        output[i] = layer.getBias()[i];
    }

    for (int i = 0; i < layer.get_num_inputs(); i++) {
        for (int j = 0; j < layer.get_num_outputs(); ++j) {
            output[j] += input[i] * layer.getWeights(i)[j];
        }
    }

    return output;
}

float log_approx(float x) {
    const float ln_coeffs[] = {
      0.2402265069591007f, 0.2851821182387283f, 0.4000097213251230f, 0.6666671873348118f};

    float x1     = x - 1.0f;
    float x2     = x1 * x1;
    float result = ln_coeffs[0] + ln_coeffs[1] * x1 + ln_coeffs[2] * x2 + ln_coeffs[3] * x2 * x1;

    return result;
}

float log2_weight_scale(int32_t input, float scale) {
    float input_float       = (float)input;
    float scaled_input      = input_float * scale;
    float ln_scaled_input   = log_approx(scaled_input);
    float log2_scaled_input = ln_scaled_input * 1.4426950408889634f;

    return log2_scaled_input;
}


int64x2_t _128_haddx2(int64_t sum0, int64_t sum1, int64_t sum2, int64_t sum3, int64x2_t bias) {
    int64_t   sum0123 = vadd_s64(sum0, sum1);
    int64_t   sum2345 = vadd_s64(sum2, sum3);
    int64x2_t sum     = vadd_s64(sum0123, sum2345);

    return vadd_s64(sum, bias);
}


std::vector<int16_t>
NNue::linear(const LinearLayer& layer, std::vector<int16_t>& output, const std::vector<int8_t>& input) const {

    constexpr int register_width = 64 / 8;

    assert(layer.get_num_inputs() % register_width == 0);
    assert(layer.get_num_outputs() % 4 == 0);

    const int num_in_subsets  = layer.get_num_inputs() / register_width;
    const int num_out_subsets = layer.get_num_outputs() / 4;

    for (int i = 0; i < num_out_subsets; ++i) {
        const int offset = i * 4 * layer.get_num_inputs();

        int64_t   sum0   = vdup_n_s64(0);
        int64_t   sum1   = vdup_n_s64(0);
        int64_t   sum2   = vdup_n_s64(0);
        int64_t   sum3   = vdup_n_s64(0);

        for (int j = 0; j < num_in_subsets; ++j) {
            int64_t in = vld1_s64(reinterpret_cast<const int64_t*>(&input[j * register_width]));

            sum0       = vmlal_s8(
                sum0,
                vld1_s16(reinterpret_cast<const int16_t*>(&layer.getWeights(offset + j * register_width))),
                in);
            sum1 = vmlal_s8(
                sum1,
                vld1_s16(reinterpret_cast<const int16_t*>(&layer.getWeights(offset + j * register_width))),
                in);
            sum2 = vmlal_s8(
                sum2,
                vld1_s16(reinterpret_cast<const int16_t*>(&layer.getWeights(offset + j * register_width))),
                in);
            sum3 = vmlal_s8(
                sum3,
                vld1_s16(reinterpret_cast<const int16_t*>(&layer.getWeights(offset + j * register_width))),
                in);
        }

        int32_t   bias  = vld1_s32(reinterpret_cast<const int32_t*>(&layer.getBias()[i * 4]));

        int32x4_t out_0 = vaddq_s32(
            vpadd_s32(vget_low_s32(sum0), vget_high_s32(sum0)),
            vpadd_s32(vget_low_s32(sum1), vget_high_s32(sum1)));
        int32x4_t out_1 = vaddq_s32(
            vpadd_s32(vget_low_s32(sum2), vget_high_s32(sum2)),
            vpadd_s32(vget_low_s32(sum3), vget_high_s32(sum3)));

        int32x4_t outval = vaddq_s32(out_0, out_1);
        outval           = vaddq_s32(outval, bias);

        vst1q_s32(reinterpret_cast<int32_t*>(&output[i * 4]), outval);

        int64x2_t outval = _128_haddx2(sum0, sum1, sum2, sum3, bias);

        vst1_s64(&output[i * 4], outval);
    }

    return output;
}

std::vector<int16_t> NNue::clipped_relu(std::vector<int16_t> output, const std::vector<int16_t> input) const {
    size_t size = input.size();

    for (int i = 0; i < size; i++) {
        output[i] = min(max(input[i], 0), 1);
    }

    return output;
}

float NNue::nnue_eval(const Position& pos, NNue::Accumulator<size>& caches) const {
    int                  color = pos.getSide();

    std::vector<int16_t> buffer;

    int16_t              input[2 * MAX_INPUT_SIZE];

    for (int i = 0; i < MAX_INPUT_SIZE; i++) {
        input[i]                  = caches[color][i];
        input[MAX_INPUT_SIZE + i] = caches[color ^ 1][i];
    }

    std::vector<int16_t> current_output = buffer;
    std::vector<int16_t> current_input;

    for (int i = 0; i < MAX_INPUT_SIZE; i++) {
        current_input[i] = input[i];
    }

    std::vector<int16_t> next_output;

    next_output    = clipped_relu(current_output, current_input);
    current_input  = current_output;
    current_output = next_output;

    next_output    = linear(l_1, current_output, current_input);
    current_input  = current_output;
    current_output = next_output;

    next_output    = clipped_relu(current_output, current_input);
    current_input  = current_output;
    current_output = next_output;

    next_output    = linear(l_2, current_output, current_input);

    return current_output[0];
}

void NNue::refresh_accumulator(const LinearLayer& layer, const std::vector<int>& active_features, int side) {

    for (int i = 0; i < size; ++i) {
        caches[side][i] = layer.getBias()[i];
    }

    for (int a : active_features) {
        for (int i = 0; i < size; ++i) {
            caches[side][i] += layer.getWeights(a)[i];
        }
    }
}

void NNue::update_accumulator(
    const LinearLayer& layer, const std::vector<int>& removed_features,
    const std::vector<int>& added_features, int perspective) {
    NNue::Accumulator<size> new_acc = caches;
    for (int i = 0; i < size; ++i) {
        new_acc[perspective][i] = caches[perspective][i];
    }

    for (int r : removed_features) {
        for (int i = 0; i < size; ++i) {
            new_acc[perspective][i] -= layer.getWeights(r)[i];
        }
    }

    for (int a : added_features) {
        for (int i = 0; i < size; ++i) {
            new_acc[perspective][i] += layer.getWeights(a)[i];
        }
    }

    caches = new_acc;
}

void refresh_accumulator(
    const LinearLayer& layer, NNue::Accumulator<size>& new_acc, const std::vector<int>& active_features,
    int side) {
    constexpr int register_width = 64 / 16;
    static_assert(size % register_width == 0, "4 elements at a time");
    constexpr int        subSets_number = size / register_width;
    int16x4_t            regs[subSets_number];

    std::vector<int16_t> biases = layer.getBias();

    for (int i = 0; i < subSets_number; i++) {
        regs[i] = vld1_s64(&biases[i]);
    }

    for (int a : active_features) {
        for (int i = 0; i < subSets_number; i++) {
            regs[i] = vaddq_s16(regs[i], vld1_s64(&layer.getWeights(a)[i * register_width]));
        }
    }

    for (int i = 0; i < subSets_number; i++) {
        vst1q_s64(&new_acc[side][i * register_width], regs[i]);
    }
}

void update_accumulator(
    const LinearLayer& layer, NNue::Accumulator<size>& new_acc, const NNue::Accumulator<size>& prev_acc,
    const std::vector<int>& removed_features, const std::vector<int>& added_features, int side) {

    constexpr int register_width = 64 / 16;
    static_assert(size % register_width == 0, "8 elements at a time");
    constexpr int subSets_number = size / register_width;
    int16x4_t     regs[subSets_number];

    for (int i = 0; i < subSets_number; i++) {
        regs[i] = vld1_s64(&prev_acc[side][i * register_width]);
    }

    for (int r : removed_features) {
        for (int j = 0; j < subSets_number; j++) {
            regs[j] = vsubq_s16(regs[j], vld1_s64(&layer.getWeights(r)[j * register_width]));
        }
    }

    for (int a : added_features) {
        for (int j = 0; j < subSets_number; j++) {
            regs[j] = vaddq_s16(regs[j], vld1_s64(&(layer.getWeights(a)[j * register_width])));
        }
    }

    for (int i = 0; i < subSets_number; i++) {
        vst1q_s64(&new_acc[side][i * register_width], regs[i]);
    }
}
