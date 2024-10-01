#include "nnue.h"

#include "position.h"

#ifdef __APPLE__
#include <arm_neon.h>
#endif

#include <cassert>
#include <cstdint>
#include <math.h>
#include <vector>


/*
    This is because I am using an Apple m1 (arm64) macbook pro for this development
    If any one has the knowledge and time to develop custom gpu/cpu acceleretors for
    this project it would be great!
*/
#ifdef __APPLE__
#define __builtin_neon_vld1_v
#define __builtin_neon_vld1q_v
#define __builtin_neon_vst1_v
#define __builtin_neon_vst1q_v
#endif

// Computes a unique key for a feature based on the position of the king, the type of piece, its square, and its color.
int feature_key(int king_square, pieceType piece_type, int square, int color) {
    // Calculate the index for the piece based on its type and color
    int p_idx = piece_type * 2 + color;

    // Compute the key by combining the piece's position, king's square, and other characteristics
    return (square + (p_idx + king_square * 10)) * 64;
}

// Retrieves a vector of active feature keys for the given position and color.
// Features represent active pieces on the board.
std::vector<int> get_active_features(const Position& pos, int color) {
    std::vector<int> active_features;

    // Get the occupancy bitboard for the given color (either white or black)
    Bitboard occupancy = color == WHITE ? pos._white_occupancy() : pos._black_occupancy();

    // Loop through all squares on the board (64 squares)
    for (int sq = 0; sq < 64; sq++) {
        // Skip if no piece is present on the current square
        if (!occupancy.is_bitset(sq)) {
            continue;
        }

        // Generate the feature key for the piece on the current square
        int key = feature_key(pos.king_square(color), pos.pieceOn(sq), sq, color);
        active_features.push_back(key);
    }

    // Return a list of active feature keys
    return active_features;
}

// Retrieves a list of added features between the current and previous positions for a given color.
// These are features that exist in the current position but were not present in the previous one.
std::vector<int> get_added_features(const Position& cur_pos, const Position& prev_pos, int color) {
    // Get the active features in the current and previous positions
    std::vector<int> active_features = get_active_features(cur_pos, color);
    std::vector<int> prev_features   = get_active_features(prev_pos, color);
    const int        size            = active_features.size();

    std::vector<int> result;

    // Find features that exist in the current position but not in the previous position
    for (int i = 0; i < size; i++) {
        if (std::find(prev_features.begin(), prev_features.end(), active_features[i]) == prev_features.end()) {
            result.push_back(active_features[i]);
        }
    }

    // Return the list of added features
    return result;
}

// Retrieves a list of removed features between the current and previous positions for a given color.
// These are features that existed in the previous position but are no longer present in the current one.
std::vector<int> get_removed_features(const Position& cur_pos, const Position& prev_pos, int color) {
    // Get the active features in the current and previous positions
    std::vector<int> active_features = get_active_features(cur_pos, color);
    std::vector<int> prev_features   = get_active_features(prev_pos, color);
    const int        size            = prev_features.size();
    std::vector<int> result;

    // Find features that existed in the previous position but no longer exist in the current one
    for (int i = 0; i < size; i++) {
        if (std::find(active_features.begin(), active_features.end(), prev_features[i]) == active_features.end()) {
            result.push_back(prev_features[i]);
        }
    }

    // Return the list of removed features
    return result;
}

// Returns the weights for a given index in the linear layer.
std::vector<int16_t> LinearLayer::getWeights(const int index) const {
    // Ensure that the index is valid
    assert(index >= 0 && index < weights.size());

    // Return the weights for the specified index
    return weights[index];
}

// Returns the number of output dimensions (neurons) in the linear layer.
int LinearLayer::get_num_outputs() const { return outputs.size(); }

// Returns the number of input dimensions (neurons) in the linear layer.
int LinearLayer::get_num_inputs() const { return inputs.size(); }

// Returns the biases for the output neurons in the linear layer.
std::vector<int16_t> LinearLayer::getBias() const { return biases; }

// Constructor for the LinearLayer class.
// Initializes the layer with random weights and biases based on the input and output sizes.
LinearLayer::LinearLayer(int input_size, int output_size, double lr) {
    // Ensure that both input and output sizes are positive
    assert(input_size > 0);
    assert(output_size > 0);

    // Set the dimensions and learning rate (eta)
    output_dims = output_size;
    input_dims  = input_size;
    eta         = lr;

    // Initialize the weights and biases randomly for each output neuron
    for (int i = 0; i < output_size; i++) {
        weights.push_back(std::vector<int16_t>());

        // Initialize the weights for each input, including an extra one for bias
        for (int j = 0; j <= input_size; j++) {
            weights.back().push_back((float)rand() / RAND_MAX);
        }
    }
}

// Feed-forward pass for the linear layer in a neural network.
// Takes the input vector and produces an output vector based on weights and biases.
std::vector<int16_t> LinearLayer::feedForward(const std::vector<int16_t>& input) {
    // Ensure that the input size matches the expected dimensions of the layer
    assert(input.size() == input_dims);

    // Reset the outputs and store the input for later use (e.g., in backpropagation)
    outputs = std::vector<int16_t>();
    inputs  = input;

    // Iterate over each output neuron in the layer
    for (int i = 0; i < output_dims; i++) {
        double sum = 0.0;

        // For each output neuron, calculate the weighted sum of inputs
        for (int w = 0; w < input_dims; w++) {
            sum += weights[i][w] * input[w];
        }

        // Add the bias term to the weighted sum
        sum += weights[i][input_dims];

        // Store the result in the output vector
        outputs.push_back(sum);
    }

    // Return the final output of the layer
    return outputs;
}

// Backpropagation step for updating weights in the linear layer.
// Takes the gradient from the next layer and computes the gradient for the previous layer.
std::vector<int16_t> LinearLayer::backPropagate(const std::vector<int16_t>& grad) {
    // Ensure that the gradient vector size matches the output dimensions of the layer
    assert(grad.size() == output_dims);

    // Initialize a vector to store the gradient for the previous layer (backpropagated)
    std::vector<int16_t> prev_layer_grad;

    // Compute the gradient for the previous layer based on the current layer's weights
    for (int i = 0; i < input_dims; i++) {
        float g = 0.0;

        // Sum up the gradients coming from each output neuron, weighted by the connection
        for (int j = 0; j < output_dims; j++) {
            g += grad[j] * weights[j][i];
        }

        // Store the gradient for this neuron in the previous layer
        prev_layer_grad.push_back(g);
    }

    // Update the weights of the current layer using the gradient descent rule
    for (int i = 0; i < output_dims; i++) {
        for (int j = 0; j < input_dims; j++) {
            // Update each weight by subtracting a scaled version of the gradient
            weights[i][j] -= (eta * grad[i] * inputs[j]);
        }

        // Update the bias for each output neuron
        weights[i][input_dims] -= eta * grad[i];
    }

    // Return the gradient for the previous layer
    return prev_layer_grad;
}

// A specialized linear function for passing data through a layer in the NNue neural network model.
std::vector<int16_t> NNue::linear(const LinearLayer& layer, std::vector<int16_t>& output, const std::vector<int16_t>& input) const {
    // Initialize the output vector by setting each value to the corresponding bias for the layer
    for (int i = 0; i < layer.get_num_outputs(); i++) {
        output[i] = layer.getBias()[i];
    }

    // Compute the weighted sum of inputs for each output neuron
    for (int i = 0; i < layer.get_num_inputs(); i++) {
        for (int j = 0; j < layer.get_num_outputs(); ++j) {
            // Multiply the input by the weight and add it to the corresponding output neuron
            output[j] += input[i] * layer.getWeights(i)[j];
        }
    }

    // Return the resulting output vector
    return output;
}

#ifdef __APPLE__&& __arm64__
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

    // Define the width of a SIMD register (64 bits) in terms of bytes (8 bytes = 64 bits)
    constexpr int register_width = 64 / 8;

    // Ensure that the number of inputs and outputs are divisible by register width and 4, respectively
    assert(layer.get_num_inputs() % register_width == 0);
    assert(layer.get_num_outputs() % 4 == 0);

    // Calculate the number of input subsets to be processed in each loop iteration
    const int num_in_subsets = layer.get_num_inputs() / register_width;
    // Calculate the number of output subsets to be processed in each loop iteration
    const int num_out_subsets = layer.get_num_outputs() / 4;

    // Loop through each output subset (processing 4 outputs at a time)
    for (int i = 0; i < num_out_subsets; ++i) {
        // Calculate the offset in the weight matrix for the current output subset
        const int offset = i * 4 * layer.get_num_inputs();

        // Initialize 64-bit accumulators for summing input-weight products (4 output neurons)
        int64_t sum0 = vdup_n_s64(0); // Accumulator for first output neuron
        int64_t sum1 = vdup_n_s64(0); // Accumulator for second output neuron
        int64_t sum2 = vdup_n_s64(0); // Accumulator for third output neuron
        int64_t sum3 = vdup_n_s64(0); // Accumulator for fourth output neuron

        // Loop through each input subset (processing register_width elements at a time)
        for (int j = 0; j < num_in_subsets; ++j) {
            // Load a chunk of input data (register width) into a 64-bit vector register
            int64_t in = vld1_s64(reinterpret_cast<const int64_t*>(&input[j * register_width]));

            // Multiply-accumulate input data with corresponding weights for each output neuron
            sum0 = vmlal_s8(sum0, vld1_s16(reinterpret_cast<const int16_t*>(&layer.getWeights(offset + j * register_width))), in);
            sum1 = vmlal_s8(sum1, vld1_s16(reinterpret_cast<const int16_t*>(&layer.getWeights(offset + j * register_width))), in);
            sum2 = vmlal_s8(sum2, vld1_s16(reinterpret_cast<const int16_t*>(&layer.getWeights(offset + j * register_width))), in);
            sum3 = vmlal_s8(sum3, vld1_s16(reinterpret_cast<const int16_t*>(&layer.getWeights(offset + j * register_width))), in);
        }

        // Load the bias for the current 4 output neurons
        int32_t bias = vld1_s32(reinterpret_cast<const int32_t*>(&layer.getBias()[i * 4]));

        // Pairwise add and sum accumulators for output neurons 0 and 1
        int32x4_t out_0 = vaddq_s32(vpadd_s32(vget_low_s32(sum0), vget_high_s32(sum0)), vpadd_s32(vget_low_s32(sum1), vget_high_s32(sum1)));
        // Pairwise add and sum accumulators for output neurons 2 and 3
        int32x4_t out_1 = vaddq_s32(vpadd_s32(vget_low_s32(sum2), vget_high_s32(sum2)), vpadd_s32(vget_low_s32(sum3), vget_high_s32(sum3)));

        // Add the partial sums for all four output neurons
        int32x4_t outval = vaddq_s32(out_0, out_1);
        // Add the bias to the final output values
        outval = vaddq_s32(outval, bias);

        // Store the computed output values back to the output vector
        vst1q_s32(reinterpret_cast<int32_t*>(&output[i * 4]), outval);

        // Further reduce the sums using a helper function (possibly for final summing) and add bias
        int64x2_t outval = _128_haddx2(sum0, sum1, sum2, sum3, bias);

        // Store the final result in the output vector
        vst1_s64(&output[i * 4], outval);
    }

    // Return the computed output vector
    return output;
}
#endif // apple and arm64
// Applies clipped ReLU to the input: ensures values are between 0 and 1
std::vector<int16_t> NNue::clipped_relu(std::vector<int16_t> output, const std::vector<int16_t> input) const {
    size_t size = input.size();

    // Apply ReLU with values clipped between 0 and 1
    for (int i = 0; i < size; i++) {
        output[i] = min(max(input[i], 0), 1);
    }

    return output;
}

#ifdef __APPLE__&& __arm64__
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
#endif // apple and arm64

// NNUE evaluation function to compute the score of the position
float NNue::nnue_eval(const Position& pos, NNue::Accumulator<size>& caches) const {
    int                  color = pos.getSide(); // Get the side to move

    std::vector<int16_t> buffer;
    int16_t              input[2 * MAX_INPUT_SIZE]; // Input buffer for both sides

    // Copy features for both sides (current side and opponent) into the input array
    for (int i = 0; i < MAX_INPUT_SIZE; i++) {
        input[i]                  = caches[color][i];
        input[MAX_INPUT_SIZE + i] = caches[color ^ 1][i];
    }

    std::vector<int16_t> current_output = buffer;
    std::vector<int16_t> current_input;

    // Load input into current_input
    for (int i = 0; i < MAX_INPUT_SIZE; i++) {
        current_input[i] = input[i];
    }

    std::vector<int16_t> next_output;

    // Apply first clipped ReLU activation
    next_output    = clipped_relu(current_output, current_input);
    current_input  = current_output;
    current_output = next_output;

    // Apply first linear layer
    next_output    = linear(l_1, current_output, current_input);
    current_input  = current_output;
    current_output = next_output;

    // Apply second clipped ReLU activation
    next_output    = clipped_relu(current_output, current_input);
    current_input  = current_output;
    current_output = next_output;

    // Apply second linear layer (final output)
    next_output = linear(l_2, current_output, current_input);

    // Return the final evaluation score
    return current_output[0];
}

// Refresh accumulator by resetting it and updating with active features
void NNue::refresh_accumulator(const LinearLayer& layer, const std::vector<int>& active_features, int side) {
    // Reset the cache with the layer's biases for the current side
    for (int i = 0; i < size; ++i) {
        caches[side][i] = layer.getBias()[i];
    }

    // Add contributions from each active feature
    for (int a : active_features) {
        for (int i = 0; i < size; ++i) {
            caches[side][i] += layer.getWeights(a)[i];
        }
    }
}

// Update the accumulator by removing old features and adding new ones
void NNue::update_accumulator(
    const LinearLayer& layer, const std::vector<int>& removed_features, const std::vector<int>& added_features, int perspective) {
    NNue::Accumulator<size> new_acc = caches;

    // Initialize the accumulator with the current cache
    for (int i = 0; i < size; ++i) {
        new_acc[perspective][i] = caches[perspective][i];
    }

    // Subtract weights of removed features
    for (int r : removed_features) {
        for (int i = 0; i < size; ++i) {
            new_acc[perspective][i] -= layer.getWeights(r)[i];
        }
    }

    // Add weights of new added features
    for (int a : added_features) {
        for (int i = 0; i < size; ++i) {
            new_acc[perspective][i] += layer.getWeights(a)[i];
        }
    }

    // Update the cache with the new accumulator
    caches = new_acc;
}

#ifdef __APPLE__&& __arm64__
// Optimized refresh of accumulator using SIMD instructions for Apple M1
void refresh_accumulator(const LinearLayer& layer, NNue::Accumulator<size>& new_acc, const std::vector<int>& active_features, int side) {
    constexpr int register_width = 64 / 16; // Number of elements per SIMD register
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

    constexpr int register_width = 64 / 16; // Number of elements per SIMD register
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
#endif // apple and arm64