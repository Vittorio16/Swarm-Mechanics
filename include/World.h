#pragma once
#include <SFML/Graphics.hpp>
#include <vector>
#include <memory>
#include "Entities/Agent.h"
using namespace std;


constexpr int NUM_CELLE_X = 350;
constexpr int NUM_CELLE_Y = 200;
constexpr int NUM_PREDATOR = 30;
constexpr int NUM_PREY = 10;
enum Terrain {Standard};

struct Cell{
    Terrain type;

    Cell() : type(Terrain::Standard) {};
};

class World{
    private:
    vector<vector<Cell>> grid;
    sf::RenderTexture gridTexture; 
    bool gridTextureValid = false; 
    
    vector<unique_ptr<Agent>> agents;

    vector<Observation> getObservation(const Agent* observer); 
    public:

    World();

    void update(float dt);

    // Methods used for visual representation
    void draw(sf::RenderWindow& window);
    void resizeGridTexture(int width, int height);
    void remapBackground(sf::Vector2u windowSize, float scaleX, float scaleY);
};