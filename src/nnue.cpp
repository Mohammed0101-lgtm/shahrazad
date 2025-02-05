#include "nnue.h"
#include "position.h"
#include <cassert>
#include <cstdint>
#include <math.h>
#include <vector>


/*
    This is because I am using an Apple m1 (arm64) macbook pro for this development
    If any one has the knowledge and time to develop custom gpu/cpu acceleretors for
    this project it would be great!
*/

// Computes a unique key for a feature based on the position of the king, the type of piece, its square, and
// its color.
uint32_t feature_key(uint8_t king_square, pieceType piece_type, uint8_t square, uint8_t color) {
    // Calculate the index for the piece based on its type and color
    int p_idx = piece_type * 2 + color;

    // Compute the key by combining the piece's position, king's square, and other characteristics
    return (square + (p_idx + king_square * 10)) * 64;
}

// Retrieves a vector of active feature keys for the given position and color.
// Features represent active pieces on the board.
std::vector<uint32_t> get_active_features(const Position& pos, uint8_t color) {
    std::vector<uint32_t> active_features;

    // Get the occupancy bitboard for the given color (either white or black)
    Bitboard occupancy = color == WHITE ? pos._white_occupancy() : pos._black_occupancy();

    // Loop through all squares on the board (64 squares)
    for (uint8_t sq = 0; sq < 64; sq++)
    {
        // Skip if no piece is present on the current square
        if (!occupancy.is_bitset(sq))
        {
            continue;
        }

        // Generate the feature key for the piece on the current square
        uint32_t key = feature_key(pos.king_square(color), pos.pieceOn(sq), sq, color);
        active_features.push_back(key);
    }

    // Return a list of active feature keys
    return active_features;
}

// Retrieves a list of added features between the current and previous positions for a given color.
// These are features that exist in the current position but were not present in the previous one.
std::vector<uint32_t>
get_added_features(const Position& cur_pos, const Position& prev_pos, uint8_t color) {
    // Get the active features in the current and previous positions
    std::vector<uint32_t> active_features = get_active_features(cur_pos, color);
    std::vector<uint32_t> prev_features   = get_active_features(prev_pos, color);
    const int             size            = active_features.size();

    std::vector<uint32_t> result;

    // Find features that exist in the current position but not in the previous position
    for (int i = 0; i < size; i++)
    {
        if (std::find(prev_features.begin(), prev_features.end(), active_features[i])
            == prev_features.end())
        {
            result.push_back(active_features[i]);
        }
    }

    // Return the list of added features
    return result;
}

// Retrieves a list of removed features between the current and previous positions for a given color.
// These are features that existed in the previous position but are no longer present in the current one.
std::vector<uint32_t>
get_removed_features(const Position& cur_pos, const Position& prev_pos, uint8_t color) {
    // Get the active features in the current and previous positions
    std::vector<uint32_t> active_features = get_active_features(cur_pos, color);
    std::vector<uint32_t> prev_features   = get_active_features(prev_pos, color);
    const int             size            = prev_features.size();
    std::vector<uint32_t> result;

    // Find features that existed in the previous position but no longer exist in the current one
    for (int i = 0; i < size; i++)
    {
        if (std::find(active_features.begin(), active_features.end(), prev_features[i])
            == active_features.end())
        {
            result.push_back(prev_features[i]);
        }
    }

    // Return the list of removed features
    return result;
}

// Returns the weights for a given index in the linear layer.
int16_t* LinearLayer::getWeights(const int index) const {
    // Ensure that the index is valid
    assert(index >= 0 && index < static_cast<int>(weights_dimensions[0]));
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
    output_dims           = output_size;
    input_dims            = input_size;
    eta                   = lr;
    weights_dimensions[0] = output_size;
    weights_dimensions[1] = input_size;

    // Initialize the weights and biases randomly for each output neuron
    for (int i = 0; i < output_size; i++)
    {
        // Initialize the weights for each input, including an extra one for bias
        for (int j = 0; j <= input_size; j++)
            weights[i][j] = (float) rand() / RAND_MAX;
    }
}

// Feed-forward pass for the linear layer in a neural network.
// Takes the input vector and produces an output vector based on weights and biases.
std::vector<int16_t> LinearLayer::feedForward(const std::vector<int16_t>& input) {
    // Ensure that the input size matches the expected dimensions of the layer
    assert(static_cast<int>(input.size()) == input_dims);

    // Reset the outputs and store the input for later use (e.g., in backpropagation)
    outputs = std::vector<int16_t>();
    inputs  = input;

    // Iterate over each output neuron in the layer
    for (int i = 0; i < output_dims; i++)
    {
        double sum = 0.0;

        // For each output neuron, calculate the weighted sum of inputs
        for (int w = 0; w < input_dims; w++)
            sum += weights[i][w] * input[w];

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
    for (int i = 0; i < input_dims; i++)
    {
        float g = 0.0;

        // Sum up the gradients coming from each output neuron, weighted by the connection
        for (int j = 0; j < output_dims; j++)
            g += grad[j] * weights[j][i];

        // Store the gradient for this neuron in the previous layer
        prev_layer_grad.push_back(g);
    }

    // Update the weights of the current layer using the gradient descent rule
    for (int i = 0; i < output_dims; i++)
    {
        for (int j = 0; j < input_dims; j++)
            // Update each weight by subtracting a scaled version of the gradient
            weights[i][j] -= (eta * grad[i] * inputs[j]);

        // Update the bias for each output neuron
        weights[i][input_dims] -= eta * grad[i];
    }

    // Return the gradient for the previous layer
    return prev_layer_grad;
}

// A specialized linear function for passing data through a layer in the NNue neural network model.
std::vector<int16_t> NNue::linear(const LinearLayer&          layer,
                                  std::vector<int16_t>&       output,
                                  const std::vector<int16_t>& input) const {
    // Initialize the output vector by setting each value to the corresponding bias for the layer
    for (int i = 0; i < layer.get_num_outputs(); i++)
        output[i] = layer.getBias()[i];

    // Compute the weighted sum of inputs for each output neuron
    for (int i = 0; i < layer.get_num_inputs(); i++)
    {
        for (int j = 0; j < layer.get_num_outputs(); ++j)
            // Multiply the input by the weight and add it to the corresponding output neuron
            output[j] += input[i] * layer.getWeights(i)[j];
    }

    // Return the resulting output vector
    return output;
}


// Applies clipped ReLU to the input: ensures values are between 0 and 1
std::vector<int16_t> NNue::clipped_relu(std::vector<int16_t>       output,
                                        const std::vector<int16_t> input) const {
    size_t size = input.size();

    // Apply ReLU with values clipped between 0 and 1
    for (int i = 0; i < size; i++)
        output[i] = min(max(input[i], 0), 1);

    return output;
}
// NNUE evaluation function to compute the score of the position
float NNue::nnue_eval(const Position& pos, NNue::Accumulator<size>& caches) const {
    int color = pos.getSide();  // Get the side to move

    std::vector<int16_t> buffer;
    int16_t              input[2 * MAX_INPUT_SIZE];  // Input buffer for both sides

    // Copy features for both sides (current side and opponent) into the input array
    for (int i = 0; i < MAX_INPUT_SIZE; i++)
    {
        input[i]                  = caches[color][i];
        input[MAX_INPUT_SIZE + i] = caches[color ^ 1][i];
    }

    std::vector<int16_t> current_output = buffer;
    std::vector<int16_t> current_input;

    // Load input into current_input
    for (int i = 0; i < MAX_INPUT_SIZE; i++)
        current_input[i] = input[i];

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
void NNue::refresh_accumulator(const LinearLayer&           layer,
                               const std::vector<uint32_t>& active_features,
                               uint8_t                      perspective) {
    assert(perspective == WHITE || perspective == BLACK);

    // Reset the cache with the layer's biases for the current side
    for (int i = 0; i < size; ++i)
        caches[perspective][i] = layer.getBias()[i];

    // Add contributions from each active feature
    for (uint32_t a : active_features)
    {
        for (int i = 0; i < size; ++i)
            caches[perspective][i] += layer.getWeights(a)[i];
    }
}

// Update the accumulator by removing old features and adding new ones
void NNue::update_accumulator(const LinearLayer&           layer,
                              const std::vector<uint32_t>& removed_features,
                              const std::vector<uint32_t>& added_features,
                              uint8_t                      perspective) {
    NNue::Accumulator<size> new_acc = caches;

    assert(perspective == WHITE || perspective == BLACK);

    // Initialize the accumulator with the current cache
    for (int i = 0; i < size; ++i)
        new_acc[perspective][i] = caches[perspective][i];

    // Subtract weights of removed features
    for (uint32_t r : removed_features)
    {
        for (int i = 0; i < size; ++i)
            new_acc[perspective][i] -= layer.getWeights(r)[i];
    }

    // Add weights of new added features
    for (uint32_t a : added_features)
    {
        for (int i = 0; i < size; ++i)
            new_acc[perspective][i] += layer.getWeights(a)[i];
    }

    // Update the cache with the new accumulator
    caches = new_acc;
}
