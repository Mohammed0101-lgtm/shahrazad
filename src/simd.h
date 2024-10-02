#include "nnue.h"
#include <cstdint>

#define USE_NEON // remember to remove this

#ifdef USE_NEON

#include <arm_neon.h>

#define __builtin_neon_vld1_v
#define __builtin_neon_vld1q_v
#define __builtin_neon_vst1_v
#define __builtin_neon_vst1q_v

using InputType  = std::int32_t;
using OutputType = std::uint8_t;

// Define the width of a SIMD register (128 bits) in terms of bytes (16 bytes = 128 bits)
constexpr uint8_t register_width = 16;

// Approximates the natural logarithm of the input 'x' using a polynomial expansion.
float log_approx(float x) {
    // Coefficients for the polynomial approximation of ln(x)
    const float ln_coeffs[] = {0.2402265069591007f, 0.2851821182387283f, 0.4000097213251230f, 0.6666671873348118f};
    // Calculate (x - 1), which is used for the approximation
    float x1 = x - 1.0f;
    // Square of (x - 1)
    float x2 = x1 * x1;
    // Polynomial approximation: ln(x) ≈ c0 + c1 * (x - 1) + c2 * (x - 1)^2 + c3 * (x - 1)^3
    float result = ln_coeffs[0] + ln_coeffs[1] * x1 + ln_coeffs[2] * x2 + ln_coeffs[3] * x2 * x1;
    // Return the approximated logarithm value
    return result;
}

// Computes the base-2 logarithm of 'input' scaled by 'scale'.
// Uses the log_approx function for natural logarithm approximation and converts it to log2.
float log2_weight_scale(int32_t input, float scale) {
    // Convert input integer to float
    float input_float = (float)input;
    // Scale the input value by the provided scale factor
    float scaled_input = input_float * scale;
    // Approximate the natural logarithm of the scaled input
    float ln_scaled_input = log_approx(scaled_input);
    // Convert natural logarithm to base-2 logarithm using the constant ln(2) ≈ 1.4427
    float log2_scaled_input = ln_scaled_input * 1.4426950408889634f;
    // Return the base-2 logarithm of the scaled input
    return log2_scaled_input;
}

// Horizontally adds the sum of four 64-bit integers (sum0, sum1, sum2, sum3) and adds the bias.
// This function is likely using NEON intrinsics to vectorize the addition.
int64x2_t _128_haddx2(int64_t sum0, int64_t sum1, int64_t sum2, int64_t sum3, int64x2_t bias) {
    // Add sum0 and sum1 to get the first intermediate sum
    int64_t sum0123 = vadd_s64(sum0, sum1);
    // Add sum2 and sum3 to get the second intermediate sum
    int64_t sum2345 = vadd_s64(sum2, sum3);
    // Add both intermediate sums together to combine all four values
    int64x2_t sum = vadd_s64(sum0123, sum2345);
    // Add the bias to the final sum and return the result
    return vadd_s64(sum, bias);
}

std::vector<int16_t> linear(const LinearLayer& layer, std::vector<int16_t>& output, const std::vector<int8_t>& input) {
    // Ensure that the number of inputs and outputs are divisible by register width and 4, respectively
    assert(layer.get_num_inputs() % register_width == 0);
    assert(layer.get_num_outputs() % 4 == 0);

    const int num_input_subsets  = layer.get_num_inputs() / register_width;
    const int num_output_subsets = layer.get_num_outputs() / 4;

    // Loop through each output subset (processing 4 outputs at a time)
    for (int i = 0; i < num_output_subsets; ++i) {
        const int offset = i * 4 * layer.get_num_inputs();

        // Initialize 16-bit accumulators for summing input-weight products (for 4 output neurons)
        int16x8_t sum0 = vdupq_n_s16(0);
        int16x8_t sum1 = vdupq_n_s16(0);
        int16x8_t sum2 = vdupq_n_s16(0);
        int16x8_t sum3 = vdupq_n_s16(0);

        // Loop through each input subset (processing register_width elements at a time)
        for (int j = 0; j < num_input_subsets; ++j) {
            // Load input data into a 128-bit vector register
            int8x16_t in = vld1q_s8(&input[j * register_width]);

            // Load weights for the current input subset
            const int16_t* weight_ptr = reinterpret_cast<const int16_t*>(layer.getWeights(offset + j * register_width));

            // Multiply-accumulate input data with weights for each output neuron
            sum0 = vmlal_s8(sum0, vget_low_s8(in), vld1_s8(reinterpret_cast<const int8_t*>(weight_ptr)));
            sum1 = vmlal_s8(sum1, vget_low_s8(in), vld1_s8(reinterpret_cast<const int8_t*>(weight_ptr + register_width)));
            sum2 = vmlal_s8(sum2, vget_low_s8(in), vld1_s8(reinterpret_cast<const int8_t*>(weight_ptr + 2 * register_width)));
            sum3 = vmlal_s8(sum3, vget_low_s8(in), vld1_s8(reinterpret_cast<const int8_t*>(weight_ptr + 3 * register_width)));
        }

        // Load the bias for the current 4 output neurons
        int32x4_t bias = vld1q_s32(reinterpret_cast<const int32_t*>(&layer.getBias()[i * 4]));

        // Add the accumulated values for all output neurons and apply bias
        int32x4_t outval0 = vaddq_s32(vpaddlq_s16(sum0), bias);
        int32x4_t outval1 = vaddq_s32(vpaddlq_s16(sum1), bias);
        int32x4_t outval2 = vaddq_s32(vpaddlq_s16(sum2), bias);
        int32x4_t outval3 = vaddq_s32(vpaddlq_s16(sum3), bias);

        // Store the output values into the output vector
        vst1q_s32(reinterpret_cast<int32_t*>(&output[i * 4]), outval0);
        vst1q_s32(reinterpret_cast<int32_t*>(&output[i * 4 + 1]), outval1);
        vst1q_s32(reinterpret_cast<int32_t*>(&output[i * 4 + 2]), outval2);
        vst1q_s32(reinterpret_cast<int32_t*>(&output[i * 4 + 3]), outval3);
    }

    return output;
}

// Optimized version of clipped ReLU using ARM NEON SIMD instructions for Apple M1 (ARM64)
std::vector<int16_t> clipped_relu(std::vector<int16_t> output, const std::vector<int16_t>& input) {
    int16x8_t zero = vdupq_n_s16(0); // Vector of zeros
    int16x8_t one  = vdupq_n_s16(1); // Vector of ones

    // Process 8 elements at a time using SIMD instructions
    for (size_t i = 0; i < size; i += 8) {
        int16x8_t input_vec = vld1q_s16(&input[i]);                       // Load 8 values from input
        int16x8_t clipped   = vmaxq_s16(zero, vminq_s16(input_vec, one)); // Apply min(1, max(0, input))
        vst1q_s16(&output[i], clipped);                                   // Store the result
    }

    return output;
}

// Optimized refresh of accumulator using SIMD instructions for Apple M1
void refresh_accumulator(const LinearLayer& layer, NNue::Accumulator<size>& new_acc, const std::vector<int>& active_features, int side) {
    static_assert(size % register_width == 0, "Size must be divisible by register width");
    constexpr int        subSets_number = size / register_width; // Number of subsets
    int16x4_t            regs[subSets_number];                   // SIMD registers

    std::vector<int16_t> biases = layer.getBias();

    // Load biases into SIMD registers
    for (int i = 0; i < subSets_number; i++) {
        regs[i] = vld1_s64(&biases[i]);
    }

    // Add contributions from active features
    for (int a : active_features) {
        for (int i = 0; i < subSets_number; i++) {
            regs[i] = vaddq_s16(regs[i], vld1_s64(&layer.getWeights(a)[i * register_width]));
        }
    }

    // Store the results back into the accumulator
    for (int i = 0; i < subSets_number; i++) {
        vst1q_s64(&new_acc[side][i * register_width], regs[i]);
    }
}

// Optimized update of accumulator using SIMD instructions for Apple M1
void update_accumulator(
    const LinearLayer& layer, NNue::Accumulator<size>& new_acc, const NNue::Accumulator<size>& prev_acc,
    const std::vector<int>& removed_features, const std::vector<int>& added_features, int side) {

    static_assert(size % register_width == 0, "Size must be divisible by register width");
    constexpr int subSets_number = size / register_width;
    int16x4_t     regs[subSets_number]; // SIMD registers

    // Load previous accumulator into SIMD registers
    for (int i = 0; i < subSets_number; i++) {
        regs[i] = vld1_s64(&prev_acc[side][i * register_width]);
    }

    // Subtract weights for removed features
    for (int r : removed_features) {
        for (int j = 0; j < subSets_number; j++) {
            regs[j] = vsubq_s16(regs[j], vld1_s64(&layer.getWeights(r)[j * register_width]));
        }
    }

    // Add weights for new added features
    for (int a : added_features) {
        for (int j = 0; j < subSets_number; j++) {
            regs[j] = vaddq_s16(regs[j], vld1_s64(&(layer.getWeights(a)[j * register_width])));
        }
    }

    // Store the updated results back into the accumulator
    for (int i = 0; i < subSets_number; i++) {
        vst1q_s64(&new_acc[side][i * register_width], regs[i]);
    }
}

#endif