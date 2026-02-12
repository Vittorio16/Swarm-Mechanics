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

    vector<Observation> getObservation(const Agent* observer); 
    // Helpers to check for prey eating grass and growing grass
    void checkPreyFeeding(unique_ptr<Agent>& agent);
    void growGrass(float dt);

    // RNG for spawning entities and grass
    mt19937 gen;
    uniform_int_distribution<int> disX;
    uniform_int_distribution<int> disY;

    public:
    vector<unique_ptr<Agent>> agents;
    vector<unique_ptr<Agent>> graveyard;

    World();

    void update(float dt);

    // Methods used for visual representation
    void draw(sf::RenderWindow& window);
    void resizeGridTexture(int width, int height);
    void remapBackground(sf::Vector2u windowSize, float scaleX, float scaleY);

};