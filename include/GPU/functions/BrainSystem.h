#pragma once
#include "GPU/structures/Swarm.h"

using namespace std;

namespace BrainSystem {
    void initRandom(SwarmData& swarm);
    void think(SwarmData& swarm);
    void mutateVector(vector<float>& weights_array, int offset, int size, float mutationRate, float mutationStrength);
    vector<float> extractBrain(const SwarmData& swarm, int index);
    void insertBrain(SwarmData& swarm, int index, const std::vector<float>& weights);
}