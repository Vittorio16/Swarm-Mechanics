#pragma once
#include <SFML/Graphics.hpp>
#include <vector>
#include <memory>
#include <random>
#include "Core/Config.h"
#include "Entities/Agent.h"

using namespace std;

class World{
    private:
    vector<vector<Cell>> grid;
    sf::RenderTexture gridTexture; 
    bool gridTextureValid = false; 
    
    vector<unique_ptr<Agent>> agents;

    vector<Observation> getObservation(const Agent* observer); 

    // RNG for spawning entities and grass
    mt19937 gen;
    uniform_int_distribution<int> disX;
    uniform_int_distribution<int> disY;

    public:

    World();

    void update(float dt);

    // Methods used for visual representation
    void draw(sf::RenderWindow& window);
    void resizeGridTexture(int width, int height);
    void remapBackground(sf::Vector2u windowSize, float scaleX, float scaleY);
};