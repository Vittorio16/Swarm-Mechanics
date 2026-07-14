#include "GPU/functions/SensorySystem.h"
#include "Core/Physics.h"

void SensorySystem::update(SwarmData& swarm, const SpatialLatticeData& lattice){
    // Gets an observation of the closest enemy for each agent in the swarm
    for (int i = 0; i < swarm.current_count; i++){
        float observerHeading = swarm.physics.facingAngle[i];
    
        int bx = (int)(swarm.physics.x[i] / LATTICE_CELL_WIDTH);
        int by = (int)(swarm.physics.y[i] / LATTICE_CELL_HEIGHT);
        
        bx = (bx % lattice.num_cells_x + lattice.num_cells_x) % lattice.num_cells_x;
        by = (by % lattice.num_cells_y + lattice.num_cells_y) % lattice.num_cells_y;
        
        int max_x_distance = (int)ceil(swarm.perceptions.viewRadius[i] / LATTICE_CELL_WIDTH);
        int max_y_distance = (int)ceil(swarm.perceptions.viewRadius[i] / LATTICE_CELL_HEIGHT);
        
        for (int h = - max_x_distance; h <= max_x_distance; h++){
            for (int j = -max_y_distance; j <= max_y_distance; j++){
                int temp_bx = ((bx + h) % lattice.num_cells_x + lattice.num_cells_x) % lattice.num_cells_x;
                int temp_by = ((by + j) % lattice.num_cells_y + lattice.num_cells_y) % lattice.num_cells_y;

                int cell_index = temp_by * lattice.num_cells_x + temp_bx;

                for (int cur_idx = 0; cur_idx < lattice.cell_counts[cell_index]; cur_idx++){
                    int idx_in_swarm = lattice.cell_agent_indices[cell_index * MAX_AGENTS_PER_CELL + cur_idx];

                    if (idx_in_swarm == i || !swarm.agentIdentifications.isAlive[idx_in_swarm]) continue;
        
                    ThoroidalData coords = getThoroidalCoordinates(
                        swarm.physics.x[i], swarm.physics.y[i], swarm.physics.x[idx_in_swarm], swarm.physics.y[idx_in_swarm], NUM_CELLE_X, NUM_CELLE_Y
                    );
            
                    float dist = coords.dist;
                    float distSq = coords.distSq;
                    float angleToTarget = coords.angleToTarget;
                    float dx = coords.dx;
                    float dy = coords.dy;
            
                    // Senses an area around the agent
                    if (distSq < swarm.perceptions.sensingRange[i]){
                        observations.emplace_back(dx, dy, dist, distSq, otherAgent);
                        continue;
                    }
            
                    if (distSq > (swarm.perceptions.viewRadius[i]*swarm.perceptions.viewRadius[i])) continue;
            
                    float angleDiff = angleToTarget - observerHeading;
                    // Normalize angle difference to be between -PI and PI
                    if (isnan(angleDiff) || isinf(angleDiff)) angleDiff = 0.0f;
            
                    angleDiff = fmodf(angleDiff, 2*M_PI);
                    if (angleDiff <= -M_PI) angleDiff += 2 * M_PI;
                    if (angleDiff > M_PI) angleDiff -= 2 * M_PI;
            
                    if (abs(angleDiff) < swarm.perceptions.fovAngle[i] * M_PI / 360.0f){
                        observations.emplace_back(dx, dy, dist, distSq, otherAgent);
                    }
                }
            }
        }
    }
}