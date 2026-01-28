#pragma once
#include <SFML/Graphics.hpp>
#include <vector>
#include <memory>
#include "Entities/Agent.h"
using namespace std;

constexpr int NUM_CELLE_X = 350;
constexpr int NUM_CELLE_Y = 200;
constexpr int NUM_PREDATOR = 6;
constexpr int NUM_PREY = 10;
enum Terrain {Standard};

struct Cell{
    Terrain type;

    Cell() : type(Terrain::Standard) {};
};

class World{
    private:
    vector<vector<Cell>> grid;
    vector<unique_ptr<Agent>> agents;

    vector<Observation> getObservation(const Agent* observer); 
    public:

    World();

    void update(float dt);
    void draw(sf::RenderWindow& window);
};