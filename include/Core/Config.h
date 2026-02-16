#pragma once
#include <vector>

constexpr float MAX_FOOD = 10.0f;

constexpr int NUM_CELLE_X = 700;
constexpr int NUM_CELLE_Y = 350;

constexpr int LATTICE_CELL_WIDTH = 35;
constexpr int LATTICE_CELL_HEIGHT = 20;

constexpr int NUM_PREDATOR = 30;
constexpr int NUM_PREY = 200;

enum Terrain {Standard};

struct Cell{
    Terrain type;
    float foodAmount;

    Cell() : type(Terrain::Standard), foodAmount(0.0f) {};
};