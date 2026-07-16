#include "Core/GlobalHelpers.h"
#include "GPU/functions/LifeSystem.h"

static uint64_t globalAgentIDCounter = 100000;

void LifeSystem::handleDeaths(SwarmData& swarm){
    for (int i = 0; i < swarm.current_count; ){
        if (!swarm.agentIdentifications.isAlive[i]){
            int last_idx = swarm.current_count - 1;
            swarm.current_count--;

            if (i == last_idx) continue;
            // Identifications
            swarm.agentIdentifications.ID[i] = swarm.agentIdentifications.ID[last_idx];
            swarm.agentIdentifications.speciesID[i] = swarm.agentIdentifications.speciesID[last_idx];
            swarm.agentIdentifications.isAlive[i] = swarm.agentIdentifications.isAlive[last_idx];

            // Fitness
            swarm.fitnessMetrics.timeLived[i] = swarm.fitnessMetrics.timeLived[last_idx];
            swarm.fitnessMetrics.energyGained[i] = swarm.fitnessMetrics.energyGained[last_idx];

            // Energy
            swarm.energyMetrics.energy[i] = swarm.energyMetrics.energy[last_idx];
            swarm.energyMetrics.digestionTime[i] = swarm.energyMetrics.digestionTime[last_idx];
            swarm.energyMetrics.remainingDigestion[i] = swarm.energyMetrics.remainingDigestion[last_idx];
            swarm.energyMetrics.childCount[i] = swarm.energyMetrics.childCount[last_idx];
            swarm.energyMetrics.reproductionCooldown[i] = swarm.energyMetrics.reproductionCooldown[last_idx];

            // Physics
            swarm.physics.friction[i] = swarm.physics.friction[last_idx];
            swarm.physics.force[i] = swarm.physics.force[last_idx];
            swarm.physics.maxSpeed[i] = swarm.physics.maxSpeed[last_idx];
            swarm.physics.x[i] = swarm.physics.x[last_idx];
            swarm.physics.y[i] = swarm.physics.y[last_idx];
            swarm.physics.vx[i] = swarm.physics.vx[last_idx];
            swarm.physics.vy[i] = swarm.physics.vy[last_idx];
            swarm.physics.speed[i] = swarm.physics.speed[last_idx];
            swarm.physics.facingAngle[i] = swarm.physics.facingAngle[last_idx];

            // Perception
            swarm.perceptions.sensingRange[i] = swarm.perceptions.sensingRange[last_idx];
            swarm.perceptions.viewRadius[i] = swarm.perceptions.viewRadius[last_idx];
            swarm.perceptions.fovAngle[i] = swarm.perceptions.fovAngle[last_idx];
            swarm.perceptions.grassViewRadius[i] = swarm.perceptions.grassViewRadius[last_idx];

            // Sensors
            swarm.sensors.lockedEnemyIndex[i] = swarm.sensors.lockedEnemyIndex[last_idx];
            swarm.sensors.closestEnemyIndex[i] = swarm.sensors.closestEnemyIndex[last_idx];
            swarm.sensors.closestEnemyX[i] = swarm.sensors.closestEnemyX[last_idx];
            swarm.sensors.closestEnemyY[i] = swarm.sensors.closestEnemyY[last_idx];
            swarm.sensors.closestEnemyDist[i] = swarm.sensors.closestEnemyDist[last_idx];
            swarm.sensors.enemyClosingSpeed[i] = swarm.sensors.enemyClosingSpeed[last_idx];
            swarm.sensors.enemyTangentialSpeed[i] = swarm.sensors.enemyTangentialSpeed[last_idx];
            swarm.sensors.foodSenseX[i] = swarm.sensors.foodSenseX[last_idx];
            swarm.sensors.foodSenseY[i] = swarm.sensors.foodSenseY[last_idx];
            swarm.sensors.foodDistance[i] = swarm.sensors.foodDistance[last_idx];
            swarm.sensors.foodClosingVelocity[i] = swarm.sensors.foodClosingVelocity[last_idx];
            swarm.sensors.foodTangentialVelocity[i] = swarm.sensors.foodTangentialVelocity[last_idx];
            swarm.sensors.energyReserve[i] = swarm.sensors.energyReserve[last_idx];

            // Neural Outputs
            swarm.neuralOutputs.thrustIntent[i] = swarm.neuralOutputs.thrustIntent[last_idx];
            swarm.neuralOutputs.turnIntent[i] = swarm.neuralOutputs.turnIntent[last_idx];
            swarm.neuralOutputs.previousThrustIntent[i] = swarm.neuralOutputs.previousThrustIntent[last_idx];
            swarm.neuralOutputs.previousTurnIntent[i] = swarm.neuralOutputs.previousTurnIntent[last_idx];

            // Brains
            const int W01_SIZE = INPUT_LAYER_SIZE * HIDDEN_LAYER_SIZE;
            const int W12_SIZE = HIDDEN_LAYER_SIZE * OUTPUT_LAYER_SIZE;
            const int B0_SIZE  = HIDDEN_LAYER_SIZE;
            const int B1_SIZE  = OUTPUT_LAYER_SIZE;

            copy_n(&swarm.brains.w01[last_idx * W01_SIZE], W01_SIZE, &swarm.brains.w01[i * W01_SIZE]);
            copy_n(&swarm.brains.w12[last_idx * W12_SIZE], W12_SIZE, &swarm.brains.w12[i * W12_SIZE]);
            copy_n(&swarm.brains.b0[last_idx * B0_SIZE],   B0_SIZE,  &swarm.brains.b0[i * B0_SIZE]);
            copy_n(&swarm.brains.b1[last_idx * B1_SIZE],   B1_SIZE,  &swarm.brains.b1[i * B1_SIZE]);

        } else{
            i++;
        }
    }
}

void LifeSystem::handleBirths(SwarmData& swarm){
    int initial_count = swarm.current_count;

    for (int i = 0; i < initial_count; i++){
        if (swarm.energyMetrics.energy[i] > MAX_ENERGY && swarm.energyMetrics.reproductionCooldown[i] <= 0.001f){
            if (swarm.current_count >= swarm.max_capacity) break;

            int child_idx = swarm.current_count;

            float energyCost = BASE_REPRODUCTION_COST;
            swarm.energyMetrics.energy[i] -= energyCost * (1.0f + REPRODUCTION_COST_SCALING * swarm.energyMetrics.childCount[i]);
            swarm.energyMetrics.reproductionCooldown[i] = REPRODUCTION_COOLDOWN;
            swarm.energyMetrics.childCount[i]++;

            float babyX = swarm.physics.x[i] + randomFloat(-1.0f, 1.0f);
            float babyY = swarm.physics.y[i] + randomFloat(-1.0f, 1.0f);

            // Thoroidal wrapping
            babyX = fmodf(babyX, (float)NUM_CELLE_X);
            if (babyX < 0) babyX += NUM_CELLE_X;
            babyY = fmodf(babyY, (float)NUM_CELLE_Y);
            if (babyY < 0) babyY += NUM_CELLE_Y;

            swarm.physics.x[child_idx] = babyX;
            swarm.physics.y[child_idx] = babyY;
            swarm.physics.vx[child_idx] = 0.0f;
            swarm.physics.vy[child_idx] = 0.0f;
            swarm.physics.speed[child_idx] = 0.0f;
            swarm.physics.facingAngle[child_idx] = randomFloat(-M_PI, M_PI);

            // Inherits parent's characteristics
            swarm.physics.friction[child_idx] = swarm.physics.friction[i];
            swarm.physics.force[child_idx] = swarm.physics.force[i];
            swarm.physics.maxSpeed[child_idx] = swarm.physics.maxSpeed[i];
            
            swarm.perceptions.sensingRange[child_idx] = swarm.perceptions.sensingRange[i];
            swarm.perceptions.viewRadius[child_idx] = swarm.perceptions.viewRadius[i];
            swarm.perceptions.fovAngle[child_idx] = swarm.perceptions.fovAngle[i];
            swarm.perceptions.grassViewRadius[child_idx] = swarm.perceptions.grassViewRadius[i];

            // Energy metrics
            swarm.energyMetrics.energy[child_idx] = energyCost; 
            swarm.energyMetrics.digestionTime[child_idx] = swarm.energyMetrics.digestionTime[i];
            swarm.energyMetrics.remainingDigestion[child_idx] = 0.0f;
            swarm.energyMetrics.childCount[child_idx] = 0;
            swarm.energyMetrics.reproductionCooldown[child_idx] = 0.0f;

            swarm.fitnessMetrics.timeLived[child_idx] = 0.0f;
            swarm.fitnessMetrics.energyGained[child_idx] = 0.0f;

            // Neural outputs
            swarm.neuralOutputs.previousThrustIntent[child_idx] = 0.0f;
            swarm.neuralOutputs.previousTurnIntent[child_idx] = 0.0f;
            swarm.neuralOutputs.thrustIntent[child_idx] = 0.0f;
            swarm.neuralOutputs.turnIntent[child_idx] = 0.0f;

            // Brain copy and mutation
            const int W01_SIZE = INPUT_LAYER_SIZE * HIDDEN_LAYER_SIZE;
            const int W12_SIZE = HIDDEN_LAYER_SIZE * OUTPUT_LAYER_SIZE;
            const int B0_SIZE  = HIDDEN_LAYER_SIZE;
            const int B1_SIZE  = OUTPUT_LAYER_SIZE;

            // Copies parent's brain
            copy_n(&swarm.brains.w01[i * W01_SIZE], W01_SIZE, &swarm.brains.w01[child_idx * W01_SIZE]);
            copy_n(&swarm.brains.w12[i * W12_SIZE], W12_SIZE, &swarm.brains.w12[child_idx * W12_SIZE]);
            copy_n(&swarm.brains.b0[i * B0_SIZE],   B0_SIZE,  &swarm.brains.b0[child_idx * B0_SIZE]);
            copy_n(&swarm.brains.b1[i * B1_SIZE],   B1_SIZE,  &swarm.brains.b1[child_idx * B1_SIZE]);

            // Mutates the brain
            float mutationRate = STARTING_MUTATION_RATE; // Potrai renderlo dinamico passandolo dal SimulationManager
            float mutationStrength = STARTING_MUTATION_STRENGTH;

            auto mutateSection = [&](vector<float>& weights_array, int offset, int size) {
                for (int m = 0; m < size; m++) {
                    if (randomFloat(0.0f, 1.0f) < mutationRate) {
                        float change = randomFloat(-mutationStrength, mutationStrength);
                        int idx = offset + m;
                        weights_array[idx] += change;
                        // Clamping
                        weights_array[idx] = fmaxf(-1.0f, std::fminf(1.0f, weights_array[idx]));
                    }
                }
            };

            mutateSection(swarm.brains.w01, child_idx * W01_SIZE, W01_SIZE);
            mutateSection(swarm.brains.w12, child_idx * W12_SIZE, W12_SIZE);
            mutateSection(swarm.brains.b0,  child_idx * B0_SIZE,  B0_SIZE);
            mutateSection(swarm.brains.b1,  child_idx * B1_SIZE,  B1_SIZE);

            swarm.current_count++;
        }
    }
}