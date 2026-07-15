#include <algorithm>
#include "GPU/functions/FoodLatticeSystem.h"

void FoodLatticeSystem::build(FoodLatticeData& foodLattice){
    for (int i = 0; i < CONSTANT_FOOD_AMOUNT; i++){
        int cx = disX(gen);
        int cy = disY(gen);

        int grid_index = cy * foodLattice.num_cells_x + cx;

        float current_food = foodLattice.foodGrid[grid_index];
        float space_left = MAX_FOOD - current_food;
        
        if (space_left <= 0) continue;

        float actual_growth = min(MAX_FOOD, space_left);

        int chunk_x = cx / FOOD_CELL_WIDTH;
        int chunk_y = cy / FOOD_CELL_HEIGHT;
        int chunk_index = chunk_y * foodLattice.num_chunks_x + chunk_x;


        // Update main grid and chunk scoreboard
        foodLattice.foodGrid[grid_index] += actual_growth;
        foodLattice.totalFood[chunk_index] += actual_growth;
        foodLattice.sumFoodX[chunk_index] += cx * actual_growth;
        foodLattice.sumFoodY[chunk_index] += cy * actual_growth;
    }
}

void addFood(FoodLatticeData& foodLattice, int cx, int cy, float growthAmount){
    int grid_index = cy * foodLattice.num_cells_x + cx;

    float current_food = foodLattice.foodGrid[grid_index];
    float space_left = MAX_FOOD - current_food;
    
    if (space_left <= 0) return;

    float actual_growth = min(growthAmount, space_left);

    int chunk_x = cx / FOOD_CELL_WIDTH;
    int chunk_y = cy / FOOD_CELL_HEIGHT;
    int chunk_index = chunk_y * foodLattice.num_chunks_x + chunk_x;

    // Update main grid and chunk scoreboard
    foodLattice.foodGrid[grid_index] += actual_growth;
    foodLattice.totalFood[chunk_index] += actual_growth;
    foodLattice.sumFoodX[chunk_index] += cx * actual_growth;
    foodLattice.sumFoodY[chunk_index] += cy * actual_growth;
}