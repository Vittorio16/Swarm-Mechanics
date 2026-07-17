#include <algorithm>
#include "functions/FoodLatticeSystem.h"

namespace FoodLatticeSystem {
    thread_local std::mt19937 gen(std::random_device{}());
    thread_local std::uniform_int_distribution<int> disX(0, NUM_CELLE_X - 1);
    thread_local std::uniform_int_distribution<int> disY(0, NUM_CELLE_Y - 1);
}

void FoodLatticeSystem::grow(FoodLatticeData& foodLattice, float growthAmount){
    for (int i = 0; i < foodLattice.foodToSpawn; i++){
        int cx;
        int cy;
        bool foundEmpty = false;

        // Bounded Retry: if after MAX_SPAWN_ATTEMPTS we cannot respawn the food cell, we stop trying to
        for (int attempt = 0; attempt < MAX_SPAWN_ATTEMPTS; attempt++) {
            cx = FoodLatticeSystem::disX(FoodLatticeSystem::gen);
            cy = FoodLatticeSystem::disY(FoodLatticeSystem::gen);
            
            // Se la cella è vuota, abbiamo finito la ricerca!
            if (foodLattice.foodGrid[cy * foodLattice.num_cells_x + cx] <= 0.001f) {
                foundEmpty = true;
                break;
            }
        }
        if (!foundEmpty) continue;

        int grid_index = cy * foodLattice.num_cells_x + cx;

        // float current_food = foodLattice.foodGrid[grid_index];
        // float space_left = MAX_FOOD - current_food;
        
        // if (space_left <= 0) continue;

        // float actual_growth = min(growthAmount, space_left);
        float actual_growth = growthAmount;
        
        int chunk_x = cx / FOOD_CELL_WIDTH;
        int chunk_y = cy / FOOD_CELL_HEIGHT;
        int chunk_index = chunk_y * foodLattice.num_chunks_x + chunk_x;


        // Update main grid and chunk scoreboard
        foodLattice.foodGrid[grid_index] += actual_growth;
        foodLattice.totalFood[chunk_index] += actual_growth;
        foodLattice.sumFoodX[chunk_index] += cx * actual_growth;
        foodLattice.sumFoodY[chunk_index] += cy * actual_growth;
    }
    foodLattice.foodToSpawn = 0;
}