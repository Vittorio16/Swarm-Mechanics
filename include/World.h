#pragma once
#include <SFML/Graphics.hpp>
#include <vector>
#include "Entities/Agent.h"
using namespace std;

constexpr int NUM_CELLE_X = 600;
constexpr int NUM_CELLE_Y = 400;

enum Terrain {Standard};

struct Cell{
    Terrain type;

    Cell() : type(Terrain::Standard) {};
};

class World{
    private:
    vector<vector<Cell>> grid;
    vector<Agent> agents;

    public:

    World() : grid(NUM_CELLE_X, vector<Cell>(NUM_CELLE_Y)) {}

    void run();
    void draw(sf::RenderWindow& window);
};