#pragma once
#include "structures/Swarm.h"
#include "structures/GraveyardData.h"

namespace LifeSystem {
    void advanceTick(SwarmData& swarm);
    void initSwarm(SwarmData& swarm, int num_prey, int num_predators);
    void handleDeaths(SwarmData& swarm, GraveyardData& graveyard);
    void handleBirths(SwarmData& swarm, float mutationRate, float mutationStrength);
    float getFitness(SwarmData& swarm, int index);
}