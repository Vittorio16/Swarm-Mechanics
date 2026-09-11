#include <cmath>
#include <random>
#include "functions/BrainSystem.h"
#include "Core/GlobalHelpers.h"

// Initializes the neural network weights and biases for each agent in the swarm randomly
__global__ void initRandomKernel(SwarmData swarm){
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i >= *swarm.current_count) return;

    curandState localState = swarm.rng.state[i];

    for (int j = 0; j < W01_SIZE; j++) swarm.brains.w01[i * W01_SIZE + j] = curand_uniform(&localState) * 2.0f - 1.0f;
    for (int j = 0; j < W12_SIZE; j++) swarm.brains.w12[i * W12_SIZE + j] = curand_uniform(&localState) * 2.0f - 1.0f;
    for (int j = 0; j < B0_SIZE; j++) swarm.brains.b0[i * B0_SIZE + j] = curand_uniform(&localState) * 2.0f - 1.0f;
    for (int j = 0; j < B1_SIZE; j++) swarm.brains.b1[i * B1_SIZE + j] = curand_uniform(&localState) * 2.0f - 1.0f;

    swarm.rng.state[i] = localState;
}

void BrainSystem::initRandom(SwarmData& swarm){
    if (*swarm.current_count == 0) return;

    int grid_size = (*swarm.current_count + BLOCK_SIZE - 1) / BLOCK_SIZE;
    
    initRandomKernel<<<grid_size, BLOCK_SIZE>>>(swarm);
    cudaDeviceSynchronize();
}

__global__ void brainThinkKernel(SwarmData swarm){
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i >= *swarm.current_count) return;

    // Prepare the inputs for the neural network
    float inputs[INPUT_LAYER_SIZE];
    inputs[0] = swarm.neuralOutputs.previousThrustIntent[i]; 
    inputs[1] = swarm.neuralOutputs.previousTurnIntent[i];
    inputs[2] = swarm.physics.speed[i] / swarm.physics.maxSpeed[i]; 
    inputs[3] = swarm.sensors.closestEnemyX[i]; 
    inputs[4] = swarm.sensors.closestEnemyY[i]; 
    inputs[5] = swarm.sensors.closestEnemyDist[i];
    inputs[6] = swarm.sensors.enemyClosingSpeed[i];
    inputs[7] = swarm.sensors.enemyTangentialSpeed[i];
    inputs[8] = swarm.sensors.foodSenseX[i];
    inputs[9] = swarm.sensors.foodSenseY[i];
    inputs[10] = swarm.sensors.foodDistance[i];
    inputs[11] = swarm.sensors.foodClosingVelocity[i];
    inputs[12] = swarm.sensors.foodTangentialVelocity[i];
    inputs[13] = swarm.sensors.energyReserve[i];

    // Calculate the indeces of the weights and biases for the current agent
    int w01_start = i * (INPUT_LAYER_SIZE * HIDDEN_LAYER_SIZE);
    int w12_start = i * (HIDDEN_LAYER_SIZE * OUTPUT_LAYER_SIZE);
    int b0_start = i * HIDDEN_LAYER_SIZE;
    int b1_start = i * OUTPUT_LAYER_SIZE;

    float hiddenValues[HIDDEN_LAYER_SIZE];
    float outputValues[OUTPUT_LAYER_SIZE];

    // Updates the neurons in the hidden node
    #pragma unroll
    for (int h = 0; h < HIDDEN_LAYER_SIZE; h++){
        float sum = 0.0f;

        #pragma unroll
        for (int j = 0; j < INPUT_LAYER_SIZE; j++){
            int weight_index = w01_start + (j * HIDDEN_LAYER_SIZE + h);
            sum += inputs[j] * swarm.brains.w01[weight_index];
        }
        sum += swarm.brains.b0[b0_start + h];
        // Activation function
        hiddenValues[h] = tanhf(sum);
    }
    
    // Updates the neurons of the output layer
    #pragma unroll
    for (int o = 0; o < OUTPUT_LAYER_SIZE; o++){
        float sum = 0.0f;
        
        #pragma unroll
        for (int j = 0; j < HIDDEN_LAYER_SIZE; j++){
            int weight_index = w12_start + (j * OUTPUT_LAYER_SIZE + o);
            sum += hiddenValues[j] * swarm.brains.w12[weight_index];
        }
        sum += swarm.brains.b1[b1_start + o];
        // Activation function
        outputValues[o] = tanhf(sum);
    }

    // Outputs are thrust and turn intents
    swarm.neuralOutputs.thrustIntent[i] = max(0.0f, outputValues[0]);
    swarm.neuralOutputs.turnIntent[i] = outputValues[1];

    swarm.neuralOutputs.previousThrustIntent[i] = swarm.neuralOutputs.thrustIntent[i];
    swarm.neuralOutputs.previousTurnIntent[i] = swarm.neuralOutputs.turnIntent[i];
}
// Given sensory inputs, decides on the agent's actions using a feed forward neural network
void BrainSystem::think(SwarmData& swarm){    
    int grid_size = (swarm.max_capacity + BLOCK_SIZE - 1) / BLOCK_SIZE;
    
    brainThinkKernel<<<grid_size, BLOCK_SIZE>>>(swarm);
}

// Mutates weights based on give mutation rate and strength
void BrainSystem::mutateVector(float* weights_array, int offset, int size, float mutationRate, float mutationStrength) {
    for (int m = 0; m < size; m++) {
        if (randomFloat(0.0f, 1.0f) < mutationRate) {
            float change = randomFloat(-mutationStrength, mutationStrength);
            int idx = offset + m;
            weights_array[idx] += change;
            // Clamping
            weights_array[idx] = fmaxf(-1.0f, std::fminf(1.0f, weights_array[idx]));
        }
    }
}

// Overloading for mutateVector to be used with vectors
void BrainSystem::mutateVector(std::vector<float>& weights_vector, int offset, int size, float mutationRate, float mutationStrength) {
    if (weights_vector.empty()) return;
    
    // Safely forward the call by passing the vector's underlying raw pointer
    mutateVector(weights_vector.data(), offset, size, mutationRate, mutationStrength);
}

vector<float> BrainSystem::extractBrain(const SwarmData& swarm, int index){
    vector<float> brain(W01_SIZE + W12_SIZE + B0_SIZE + B1_SIZE);

    for (int j = 0; j < W01_SIZE; j++) brain[j] = swarm.brains.w01[index * W01_SIZE + j];
    for (int j = 0; j < W12_SIZE; j++) brain[W01_SIZE + j] = swarm.brains.w12[index * W12_SIZE + j];
    for (int j = 0; j < B0_SIZE; j++) brain[W01_SIZE + W12_SIZE + j] = swarm.brains.b0[index * B0_SIZE + j];
    for (int j = 0; j < B1_SIZE; j++) brain[W01_SIZE + W12_SIZE + B0_SIZE + j] = swarm.brains.b1[index * B1_SIZE + j];

    return brain;
}
    
void BrainSystem::insertBrain(SwarmData& swarm, int index, const vector<float>& weights){
    if(weights.size() != (W01_SIZE + W12_SIZE + B0_SIZE + B1_SIZE)) return;

    for (int j = 0; j < W01_SIZE; j++) swarm.brains.w01[index * W01_SIZE + j] = weights[j];
    for (int j = 0; j < W12_SIZE; j++) swarm.brains.w12[index * W12_SIZE + j] = weights[W01_SIZE + j];
    for (int j = 0; j < B0_SIZE; j++)  swarm.brains.b0[index * B0_SIZE + j] = weights[W01_SIZE + W12_SIZE + j];
    for (int j = 0; j < B1_SIZE; j++) swarm.brains.b1[index * B1_SIZE + j] = weights[W01_SIZE + W12_SIZE + B0_SIZE + j];
}