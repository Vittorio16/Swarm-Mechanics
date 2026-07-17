#include <cmath>
#include "functions/PhysicSystem.h"

// Moves the agents based on their current velocity and updates their facing angle based on neural outputs
void PhysicsSystem::update(SwarmData& swarm, float dt){
    for (int i = 0; i < swarm.current_count; i++){
        if (!swarm.agentIdentifications.isAlive[i]) continue;

        swarm.fitnessMetrics.timeLived[i] += dt;

        // Update heading and acceleration based on brain output
        swarm.physics.facingAngle[i] += swarm.neuralOutputs.turnIntent[i] * MAXIMUM_TURNING_SPEED * dt;
        swarm.physics.facingAngle[i] = fmodf(swarm.physics.facingAngle[i], 2 * M_PI);
        if (swarm.physics.facingAngle[i] <= -M_PI) swarm.physics.facingAngle[i] += 2 * M_PI;
        if (swarm.physics.facingAngle[i] > M_PI) swarm.physics.facingAngle[i] -= 2 * M_PI;

        // Apply thrust in the direction of facingAngle
        float ax = swarm.neuralOutputs.thrustIntent[i] * cosf(swarm.physics.facingAngle[i]) * swarm.physics.force[i];
        float ay = swarm.neuralOutputs.thrustIntent[i] * sinf(swarm.physics.facingAngle[i]) * swarm.physics.force[i];

        // Update velocity using acceleration and friction
        swarm.physics.vx[i] += ax * dt;
        swarm.physics.vy[i] += ay * dt;
        
        float retention = max(0.0f, 1.0f - (swarm.physics.friction[i] * dt));
        swarm.physics.vx[i] *= retention;
        swarm.physics.vy[i] *= retention;

        // Checks constraint on speed
        swarm.physics.speed[i] = hypotf(swarm.physics.vx[i], swarm.physics.vy[i]);
        
        if (swarm.physics.speed[i] > swarm.physics.maxSpeed[i]){
            float excessRatio = swarm.physics.maxSpeed[i] / swarm.physics.speed[i];
            swarm.physics.vx[i] *= excessRatio;
            swarm.physics.vy[i] *= excessRatio;
            swarm.physics.speed[i] = swarm.physics.maxSpeed[i];
        }

        // Update position
        swarm.physics.x[i] += swarm.physics.vx[i] * dt;
        swarm.physics.y[i] += swarm.physics.vy[i] * dt;

        // pacman style world
        if (isnan(swarm.physics.x[i]) || isinf(swarm.physics.x[i])) swarm.physics.x[i] = 0.0f;
        swarm.physics.x[i] = fmodf(swarm.physics.x[i], NUM_CELLE_X);
        if(swarm.physics.x[i] < 0) swarm.physics.x[i] += NUM_CELLE_X;

        if (isnan(swarm.physics.y[i]) || isinf(swarm.physics.y[i])) swarm.physics.y[i] = 0.0f;
        swarm.physics.y[i] = fmodf(swarm.physics.y[i], NUM_CELLE_Y);
        if(swarm.physics.y[i] < 0) swarm.physics.y[i] += NUM_CELLE_Y;
    }
}