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

// Set to start with pre-trained brains, selected in main.cpp
constexpr bool REPLAY_MODE_ENABLED = false;
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
constexpr int MAXIMUM_ELITE_COUNT = 500;

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

// Starting agents
constexpr int NUM_PREDATOR = 100;
constexpr int NUM_PREY = 100;

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

// Grows food for cells nearby grass
constexpr float SPREAD_GROWTH_MULTIPLIER = 15.0f;

// Help create islands on world gen
constexpr int STARTING_ISLANDS = 20;
constexpr int MINIMUM_ISLAND_SIZE = 3;
constexpr int MAXIMUM_ISLAND_SIZE = 9;

// Incremental food spawning settings across generations
constexpr float STARTING_FOOD_EXPANSION_RATE = 0.05f;
constexpr float MINIMUM_FOOD_EXPANSION_RATE = 0.01f;
constexpr float FOOD_EXPANSION_RATE_DECAY = 0.0002f;

// Incremental food island spawning settings across generations
constexpr float STARTING_ISLAND_SPAWN_RATE = 2.0f;
constexpr float MINIMUM_ISLAND_SPAWN_RATE = 0.5f;
constexpr float ISLAND_SPAWN_RATE_DECAY = 0.01f;

// How often all of the grass grows
constexpr float GRASS_GROWTH_INTERVAL = 0.5f; 
constexpr float GROWTH_MULTIPLIER = 2.0f;

// Only saves dead agents with fitness greater than this to be later analyzed
constexpr float MINIIMUM_FITNESS_TO_BE_SAVED = 5.0f;

// Set to true for debugging to visualize the map grid, food lattice, or agents' FOV
const bool SHOW_GRID = false;
const bool SHOW_FOOD_LATTICE = false;
const bool SHOW_FOV = false;



/* ------------------- Agent Settings ------------------ */

// Influences much the agent slows down when not accelerating - pivotal for turning
constexpr float FRICTION_COEFFICIENT = 3.0f;

// Enery settings per agents
constexpr float MAX_ENERGY = 100.0f;
constexpr float METABOLISM_COST = 0.05f;
constexpr float MAX_EFFORT_COST = 5.0f;
const float STARTING_ENERGY =  2 * MAX_ENERGY / 3;

// Reproduction settings - energy cost, cooldown, and child count scaling
const float BASE_REPRODUCTION_COST = MAX_ENERGY / 2.0f;
constexpr float REPRODUCTION_COST_SCALING = 0.3f;
constexpr float REPRODUCTION_COOLDOWN = 5.0f;

// Range around the agent in which it perceives enemies all-around
constexpr float SENSING_RANGE = 36.0f;

// Range around the agent in which it perceives grass - one of the main bottlenecks for simulation efficiency
constexpr float GRASS_SENSING_RADIUS = 50.0f;

// Minimum speed required to update heading, to avoid jitter when almost still - also needs work
constexpr float MAXIMUM_TURNING_SPEED = 3.14f;
constexpr float TURNING_COST_PENALTY = 0.5f;


/* ------------------- Predator Settings ------------------ */

// Range in which a predator can eat a prey
const float KILL_RANGE_SQ = 5.0f;

// Predator-specific parameters
constexpr int PREDATOR_ID = 1;
constexpr float PREDATOR_MAX_SPEED = 35.0f;
constexpr float PREDATOR_FORCE = 400.0f;
constexpr float PREDATOR_VIEW_RADIUS = 30.0f;
constexpr float PREDATOR_FOV_ANGLE = 120.0f;
constexpr float PREDATOR_DIGESTION_TIME = 0.5f;

// Prredators consume more energy passively, less to sprint
constexpr float PREDATOR_METABOLISM_MULTIPLIER = 5.0f;
constexpr float PREDATOR_EFFORT_MULTIPLIER = 2.0f;

// Energy gained by predators when eating prey
const float PREDATOR_ENERGY_GAIN = MAX_ENERGY / 2;
constexpr float PREDATOR_ENERGY_FITNESS_MULTIPLIER = 10.0f;

/* ------------------- Prey Settings ------------------ */

// The radius within which a prey can eat grass
constexpr float PREY_EAT_RADIUS_SQ = 4.0f;

// Minimum food amount in a cell to be considered worth eating
constexpr float MINIMUM_BITE_SIZE = 1.5f;

// Prey-specific parameters
constexpr int PREY_ID = -1;
constexpr float PREY_MAX_SPEED = 25.0f;
constexpr float PREY_FORCE = 600.0f;
constexpr float PREY_VIEW_RADIUS = 20.0f;
constexpr float PREY_FOV_ANGLE = 180.0f;
constexpr float PREY_DIGESTION_TIME = 0.25f;

const float PREY_EFFORT_MULTIPLIER = 0.5f;

// Rewards gaining energy, to discourage standstill
const float PREY_ENERGY_GAIN = MAX_ENERGY / 2;
constexpr float PREY_ENERGY_FITNESS_MULTIPLIER = 10.0f;


/* ------------------- Learning Settings ------------------ */

// Brain size parameters - take care to keep these consistent with the ones inputted from the agents
const int INPUT_LAYER_SIZE = 14;
const int HIDDEN_LAYER_SIZE = 8;
const int OUTPUT_LAYER_SIZE = 2;

// Mutation parameters are kept in the simulation section, since they change across generations



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