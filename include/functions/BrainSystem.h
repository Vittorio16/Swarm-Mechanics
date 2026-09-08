#pragma once
#include "structures/Swarm.h"

using namespace std;

namespace BrainSystem {
    void initRandom(SwarmData& swarm);
    void think(SwarmData& swarm);
    void mutateVector(float* weights_array, int offset, int size, float mutationRate, float mutationStrength);
    void mutateVector(vector<float>& weights_array, int offset, int size, float mutationRate, float mutationStrength);
    vector<float> extractBrain(const SwarmData& swarm, int index);
    void insertBrain(SwarmData& swarm, int index, const std::vector<float>& weights);

    // Inline device version of mutateVector used in GPU kernels
    inline __device__ void mutateVectorDevice(float* weights_array, int offset, int size, float mutationRate, float mutationStrength, curandState* localState) {
        for (int m = 0; m < size; m++) {
            if (curand_uniform(localState) < mutationRate) {
                float change = (curand_uniform(localState) * 2.0f - 1.0f) * mutationStrength;
                int idx = offset + m;
                weights_array[idx] += change;
                
                // Clamping
                weights_array[idx] = fmaxf(-1.0f, fminf(1.0f, weights_array[idx]));
            }
        }
    }
}