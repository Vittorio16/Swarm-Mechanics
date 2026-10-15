#pragma once
#ifdef USE_SFML_GUI
    #include <SFML/Graphics.hpp>
#endif

#include "Core/Config.h"
#include "structures/Swarm.h"
#include "structures/SpatialLatticeData.h"
#include "structures/FoodLatticeData.h"
#include "structures/GraveyardData.h"

using namespace std;

// Breakdown of one sampled tick of the simulation, used for profiling
struct TickProfile {
    float buckets = 0.0f;
    float obs = 0.0f;
    float think = 0.0f;
    float move = 0.0f;
    float cleanup = 0.0f;
    float total = 0.0f;
    bool valid = false;
};

class World{
    private:  
    int world_id;
    // Texture
    #ifdef USE_SFML_GUI
        sf::RenderTexture gridTexture;
    #endif
    bool gridTextureValid = false; 
    
    // CUDA events for profiling
    cudaEvent_t start_total, end_buckets, end_obs, end_think, end_move, end_cleanup;
    bool profilePending = false;
    
    public:
    SwarmData swarm;
    SpatialLatticeData spatialLattice;
    FoodLatticeData foodLattice;
    GraveyardData graveyard;

    TickProfile lastProfile;

    World(int numPrey, int numPredators, int world_id = 0, uint64_t seed = 0, float grassRadius = GRASS_SENSING_RADIUS, float predViewRadius = PREDATOR_VIEW_RADIUS);
    void reset(int num_prey, int num_predators, float grassRadius = GRASS_SENSING_RADIUS, float predViewRadius = PREDATOR_VIEW_RADIUS);
    ~World();

    // Updattes the world each tick of the simulation
    void update(float dt, int generationCount, bool profileTick = false);

    // Safe only after sync
    void readProfile();

    // Methods used for visual representation
    #ifdef USE_SFML_GUI
        void draw(sf::RenderWindow& window);
        void drawFoodLattice(float scaleX, float scaleY);
        void resizeGridTexture(int width, int height);
        void remapBackground(sf::Vector2u windowSize, float scaleX, float scaleY);
    #endif
};