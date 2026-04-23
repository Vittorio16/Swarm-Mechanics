#pragma once
#include <SFML/Graphics.hpp>
#include <vector>
#include <memory>
#include <random>
#include "Core/Config.h"
#include "Entities/Agent.h"
#include "Entities/Predator.h"
#include "Entities/Prey.h"

using namespace std;

class World{
    private:
    vector<vector<Cell>> grid;
    
    // Uniform lattices for efficient enemy lookup and prey feeding
    int lattice_x_cells, lattice_y_cells;
    vector<vector<Agent*>> spatial_lattice;
    
    int food_lattice_x_cells, food_lattice_y_cells;
    vector<FoodChunk> food_lattice;

    void update_buckets();
    
    sf::RenderTexture gridTexture; 
    bool gridTextureValid = false; 
    
    // RNG for spawning entities and grass
    mt19937 gen;
    uniform_int_distribution<int> disX;
    uniform_int_distribution<int> disY;
    
    vector<Observation> getObservation(const Agent* observer); 
    
    // Helpers to check for prey eating grass and growing grass
    vector<float> getBestFoodScent(const Agent* agent); 
    void checkPreyFeeding(unique_ptr<Agent>& agent);
    void growGrass(float dt, int generationCount);

    public:
    vector<unique_ptr<Agent>> agents;
    vector<unique_ptr<Agent>> graveyard;

    World();

    void update(float dt, int generationCount);

    // Methods used for visual representation
    void draw(sf::RenderWindow& window);
    void drawFoodLattice(float scaleX, float scaleY);
    void resizeGridTexture(int width, int height);
    void remapBackground(sf::Vector2u windowSize, float scaleX, float scaleY);

};