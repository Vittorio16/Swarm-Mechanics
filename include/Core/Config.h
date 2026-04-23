#pragma once
#include <SFML/Graphics.hpp>
#include <vector>

using namespace std;


/* ------------------- Main settings ------------------ */

// Used when failed to detect hardware concurrency
constexpr int FALLBACK_CORE_NUMBER = 4;

// Standard fixed step (1/200th of a second)
const float FIXED_TIME_STEP = 1.0f / 200.0f;

// Visualization settings
const float MINIMUM_REAL_TIME_STEP = 0.05f;
constexpr int MAX_FPS = 170;

// Enable to stop time when pressing R
constexpr bool DEBUGGING_ENABLED = false;



/* ------------------- Simulation Settings ------------------ */

// Size of the hall of fame (best agents preserved from previous generations)
constexpr int HALL_OF_FAME_SIZE = 50;

// Generation duration parameters in seconds
constexpr float STARTING_GENERATION_DURATION = 60.0f;
constexpr float MAXIMUM_GENERATION_DURATION = 180.0f;
constexpr float GENERATION_SCALING_FACTOR = 3.0f;

// Batch size - number of simulation ticks processed in parallel when rendering is disabled
const int MAXIMUM_BATCH_SIZE = 1 / FIXED_TIME_STEP; // sets it to 1 second

// Sets how often the hall of fame is updated (in generations)
constexpr int HOF_GENERATION_UPDATE_RATE = 3;

// Percentage of best agents to pool from for next generation, and maximum number of them
const float TOP_PERCENTAGE = 0.1f;
constexpr int MAXIMUM_ELITE_COUNT = 50;

// Mutation rates per generation
constexpr float STARTING_MUTATION_RATE = 0.20f;
constexpr float STARTING_MUTATION_STRENGTH = 0.40f;

constexpr float MINIMUM_MUTATION_RATE = 0.02f;
constexpr float MINIMUM_MUTATION_STRENGTH = 0.05f;

constexpr float MUTATION_RATE_DECAY = 0.005f;
constexpr float MUTATION_STRENGTH_DECAY = 0.01f;

// Parameters for loading pre-trained brains - which pool to chose from
constexpr float HOF_POOL_INJECTION_RATE = 0.10f;
const float RANDOM_INJECTION_RATE = 0.20f; // minus the hof injection rate



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
constexpr float STARTING_ISLAND_SPAWN_RATE = 2.0f;
constexpr float MINIMUM_ISLAND_SPAWN_RATE = 0.5f;
constexpr float ISLAND_SPAWN_RATE_DECAY = 0.01f;


// Only saves dead agents with fitness greater than this to be later analyzed
constexpr float MINIIMUM_FITNESS_TO_BE_SAVED = 5.0f;

// Set to true for debugging to visualize the map grid, food lattice, or agents' FOV
const bool SHOW_GRID = false;
const bool SHOW_FOOD_LATTICE = false;
const bool SHOW_FOV = false;


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