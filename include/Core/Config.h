#pragma once
#include <SFML/Graphics.hpp>
#include <vector>

using namespace std;

constexpr float MAX_FOOD = 10.0f;

constexpr int NUM_CELLE_X = 1920;
constexpr int NUM_CELLE_Y = 1080;

// Enemy observation lattice parameters
constexpr int LATTICE_CELL_WIDTH = 30;
constexpr int LATTICE_CELL_HEIGHT = 30;

// Food lattice parameters
constexpr int FOOD_CELL_WIDTH = 10;
constexpr int FOOD_CELL_HEIGHT = 10;

constexpr int NUM_PREDATOR = 15;
constexpr int NUM_PREY = 100;

enum Terrain {Standard};

struct Cell{
    Terrain type;
    float foodAmount;

    Cell() : type(Terrain::Standard), foodAmount(0.0f) {};
};

struct FoodChunk {
    float totalFood;
    float sumFoodX;
    float sumFoodY;

    vector<sf::Vector2i> activeCells;

    FoodChunk() : totalFood(0.0f), sumFoodX(0.0f), sumFoodY(0.0f) {}
};