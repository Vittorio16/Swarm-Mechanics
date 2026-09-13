#pragma once
#include "structures/Swarm.h"
#include "Core/Rng.h"

using namespace std;

namespace BrainSystem {
    void initRandom(SwarmData& swarm);
    void think(SwarmData& swarm);
    void mutateVector(float* weights_array, int offset, int size, float mutationRate, float mutationStrength);
    void mutateVector(vector<float>& weights_array, int offset, int size, float mutationRate, float mutationStrength);
    vector<float> extractBrain(const SwarmData& swarm, int index);
    void insertBrain(SwarmData& swarm, int index, const std::vector<float>& weights);

    // Inline device version of mutateVector used in GPU kernels
    inline __device__ void mutateBrainDevice(float* array, int numElements, int agent, int capacity,
                                             float mutationRate, float mutationStrength,
                                             RngStream& rng) {
        for (int e = 0; e < numElements; e++) {
            if (rng.nextFloat() < mutationRate) {
                int idx = e * capacity + agent;
                
                // Calculate random float between -mutationStrength and +mutationStrength
                float v = array[idx] + rng.nextFloat(-mutationStrength, mutationStrength);
                array[idx] = fmaxf(-1.0f, fminf(1.0f, v));
            }
        }
    }
}