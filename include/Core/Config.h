#pragma once
#include <SFML/Graphics.hpp>
#include <vector>

using namespace std;

/* ------------------- Simulation Settings ------------------ */


/* ------------------- World Settings ------------------ */

// World dimensions
constexpr int NUM_CELLE_X = 960;
constexpr int NUM_CELLE_Y = 540;

// Enemy observation lattice parameters
constexpr int LATTICE_CELL_WIDTH = 30;
constexpr int LATTICE_CELL_HEIGHT = 30;

// Food lattice parameters
constexpr int FOOD_CELL_WIDTH = 10;
constexpr int FOOD_CELL_HEIGHT = 10;

// Food settings per cell
constexpr float MAX_FOOD = 10.0f;
constexpr float GROWTH_MULTIPLIER = 1000.0f;

// Incremental food spawning settings across generations
constexpr float STARTING_FOOD_EXPANSION_RATE = 2.0f;
constexpr float MINIMUM_FOOD_EXPANSION_RATE = 0.5f;
constexpr float FOOD_EXPANSION_RATE_DECAY = 0.01f;

// Incremental food island spawning settings across generations
constexpr float MINIMUM_ISLAND_SPAWN_RATE = 0.5f;
constexpr float ISLAND_SPAWN_RATE_DECAY = 0.01f;

/* ------------------- Agent Settings ------------------ */

// Starting agents
constexpr int NUM_PREDATOR = 100;
constexpr int NUM_PREY = 100;


/* ------------------- Prey Settings ------------------ */

// The radius within which a prey can eat grass
constexpr float PREY_EAT_RADIUS_SQ = 4.0f;

/* ------------------- Learning Settings ------------------ */



/* ------------------- Utilities ------------------ */
enum Terrain {Standard};

struct Cell{
    Terrain type;
    float foodAmount;

    Cell() : type(Terrain::Standard), foodAmount(0.0f) {};
};

// A chunk of the world grid, used to optimize food sensing
struct FoodChunk {
    float totalFood;
    float sumFoodX;
    float sumFoodY;

    vector<sf::Vector2i> activeCells;

    FoodChunk() : totalFood(0.0f), sumFoodX(0.0f), sumFoodY(0.0f) {}
};