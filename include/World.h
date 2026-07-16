#pragma once
#include <SFML/Graphics.hpp>
#include "Core/Config.h"

#include "GPU/structures/Swarm.h"
#include "GPU/structures/SpatialLatticeData.h"
#include "GPU/structures/FoodLatticeData.h"
#include "GPU/structures/GraveyardData.h"

using namespace std;

class World{
    private:  
    // Texture
    sf::RenderTexture gridTexture; 
    bool gridTextureValid = false; 
    
    public:
    SwarmData swarm;
    SpatialLatticeData spatialLattice;
    FoodLatticeData foodLattice;
    GraveyardData graveyard;

    World(int numPrey, int numPredators);

    // Updattes the world each tick of the simulation
    ProfilingData update(float dt, int generationCount);

    // Methods used for visual representation
    void draw(sf::RenderWindow& window);
    void drawFoodLattice(float scaleX, float scaleY);
    void resizeGridTexture(int width, int height);
    void remapBackground(sf::Vector2u windowSize, float scaleX, float scaleY);

};