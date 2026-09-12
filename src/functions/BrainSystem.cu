#include <cmath>
#include <random>
#include "functions/BrainSystem.h"
#include "Core/GlobalHelpers.h"
#include "Core/GpuConfig.h"

// Initializes the neural network weights and biases for each agent in the swarm randomly
__global__ void initRandomKernel(SwarmData swarm){
    int count = *swarm.current_count;
    int stride = gridDim.x * blockDim.x;

    int cap = swarm.max_capacity;

    for (int i = blockIdx.x * blockDim.x + threadIdx.x; i < count; i += stride){
        curandState localState = swarm.rng.state[i];

        for (int j = 0; j < W01_SIZE; j++) swarm.brains.w01[j * cap + i] = curand_uniform(&localState) * 2.0f - 1.0f;
        for (int j = 0; j < W12_SIZE; j++) swarm.brains.w12[j * cap + i] = curand_uniform(&localState) * 2.0f - 1.0f;
        for (int j = 0; j < B0_SIZE; j++) swarm.brains.b0[j * cap + i] = curand_uniform(&localState) * 2.0f - 1.0f;
        for (int j = 0; j < B1_SIZE; j++) swarm.brains.b1[j * cap + i] = curand_uniform(&localState) * 2.0f - 1.0f;

        swarm.rng.state[i] = localState;
    }
}

void BrainSystem::initRandom(SwarmData& swarm){
    initRandomKernel<<<GpuConfig::persistentGrid, BLOCK_SIZE, 0, cudaStreamPerThread>>>(swarm);
    cudaDeviceSynchronize();
}

__global__ void brainThinkKernel(SwarmData swarm) {
    int count = *swarm.current_count;
    int cap = swarm.max_capacity;
    int stride = gridDim.x * blockDim.x;

    for (int i = blockIdx.x * blockDim.x + threadIdx.x; i < count; i += stride) {
        
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

        float hiddenValues[HIDDEN_LAYER_SIZE];
        float outputValues[OUTPUT_LAYER_SIZE];

        // Updates the neurons in the hidden node
        #pragma unroll
        for (int h = 0; h < HIDDEN_LAYER_SIZE; h++){
            float sum = 0.0f;

            #pragma unroll
            for (int j = 0; j < INPUT_LAYER_SIZE; j++){
                // Coalesced index mapping: (element_index) * capacity + agent_index
                // Keeps different agents' parameters accessed by same SM close to each other
                int weight_index = (j * HIDDEN_LAYER_SIZE + h) * cap + i;
                sum += inputs[j] * swarm.brains.w01[weight_index];
            }
            
            sum += swarm.brains.b0[h * cap + i];
            
            // Activation function
            hiddenValues[h] = tanhf(sum);
        }
        
        // Updates the neurons of the output layer
        #pragma unroll
        for (int o = 0; o < OUTPUT_LAYER_SIZE; o++){
            float sum = 0.0f;
            
            #pragma unroll
            for (int j = 0; j < HIDDEN_LAYER_SIZE; j++){
                int weight_index = (j * OUTPUT_LAYER_SIZE + o) * cap + i;
                sum += hiddenValues[j] * swarm.brains.w12[weight_index];
            }
            
            sum += swarm.brains.b1[o * cap + i];
            
            // Activation function
            outputValues[o] = tanhf(sum);
        }

        // Outputs are thrust and turn intents
        swarm.neuralOutputs.thrustIntent[i] = fmaxf(0.0f, outputValues[0]);
        swarm.neuralOutputs.turnIntent[i] = outputValues[1];
        
        swarm.neuralOutputs.previousThrustIntent[i] = swarm.neuralOutputs.thrustIntent[i];
        swarm.neuralOutputs.previousTurnIntent[i] = swarm.neuralOutputs.turnIntent[i];
    }
}
// Given sensory inputs, decides on the agent's actions using a feed forward neural network
void BrainSystem::think(SwarmData& swarm){    
    brainThinkKernel<<<GpuConfig::persistentGrid, BLOCK_SIZE, 0, cudaStreamPerThread>>>(swarm);
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
    const int cap = swarm.max_capacity;
    
    int o = 0;
    for (int j = 0; j < W01_SIZE; j++) brain[o++] = swarm.brains.w01[j * cap + index];
    for (int j = 0; j < W12_SIZE; j++) brain[o++] = swarm.brains.w12[j * cap + index];
    for (int j = 0; j < B0_SIZE; j++) brain[o++] = swarm.brains.b0[j * cap + index];
    for (int j = 0; j < B1_SIZE; j++) brain[o++] = swarm.brains.b1[j * cap + index];

    return brain;
}
    
void BrainSystem::insertBrain(SwarmData& swarm, int index, const vector<float>& weights){
    if(weights.size() != (size_t)(W01_SIZE + W12_SIZE + B0_SIZE + B1_SIZE)) return;
    const int cap = swarm.max_capacity;

    int o = 0;
    for (int j = 0; j < W01_SIZE; j++) swarm.brains.w01[j * cap + index] = weights[o++];
    for (int j = 0; j < W12_SIZE; j++) swarm.brains.w12[j * cap + index] = weights[o++];
    for (int j = 0; j < B0_SIZE; j++)  swarm.brains.b0[j * cap + index] = weights[o++];
    for (int j = 0; j < B1_SIZE; j++) swarm.brains.b1[j * cap + index] = weights[o++];
}