#pragma once
#include <SFML/Graphics.hpp>
#include "Core/Config.h"

#include "structures/Swarm.h"
#include "structures/SpatialLatticeData.h"
#include "structures/FoodLatticeData.h"
#include "structures/GraveyardData.h"

using namespace std;

class World{
    private:  
    int world_id;
    // Texture
    sf::RenderTexture gridTexture; 
    bool gridTextureValid = false; 
    
    // CUDA events for profiling
    cudaEvent_t start_total, end_buckets, end_obs, end_think, end_move, end_cleanup;
    
    public:
    SwarmData swarm;
    SpatialLatticeData spatialLattice;
    FoodLatticeData foodLattice;
    GraveyardData graveyard;

    World(int numPrey, int numPredators, int world_id = 0, uint64_t seed = 0);
    void reset(int num_prey, int num_predators);
    ~World();
    // Updattes the world each tick of the simulation
    ProfilingData update(float dt, int generationCount);

    // Methods used for visual representation
    void draw(sf::RenderWindow& window);
    void drawFoodLattice(float scaleX, float scaleY);
    void resizeGridTexture(int width, int height);
    void remapBackground(sf::Vector2u windowSize, float scaleX, float scaleY);

};