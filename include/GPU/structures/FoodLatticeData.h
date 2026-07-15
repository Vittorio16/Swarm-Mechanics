#pragma once
#include "Core/Config.h"

using namespace std;

struct FoodLatticeData{
    // Global data
    int num_chunks_x;
    int num_chunks_y;
    int total_chunks;

    vector<float> totalFood;
    vector<float> sumFoodX;
    vector<float>  sumFoodY;

    // Chunk data
    int num_cells_x;
    int num_cells_y;
    int total_cells;
    vector<float> foodGrid;

    FoodLatticeData(int chunks_x, int chunks_y, int cells_x, int cells_y) : 
    num_chunks_x(chunks_x),
    num_chunks_y(chunks_y),
    total_chunks(chunks_x * chunks_y),
    totalFood(chunks_x * chunks_y, 0.0f),
    sumFoodX(chunks_x * chunks_y, 0.0f),
    sumFoodY(chunks_x* chunks_y, 0.0f),
    num_cells_x(cells_x),
    num_cells_y(cells_y),
    total_cells(cells_x * cells_y),
    foodGrid(cells_x * cells_y, 0.0f) {}
};