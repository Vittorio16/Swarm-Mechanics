#pragma once
#include <SFML/Graphics.hpp>
#include <vector>

using namespace std;

constexpr float MAX_FOOD = 10.0f;

constexpr int NUM_CELLE_X = 800;
constexpr int NUM_CELLE_Y = 400;

constexpr int LATTICE_CELL_WIDTH = 40;
constexpr int LATTICE_CELL_HEIGHT = 40;

constexpr int NUM_PREDATOR = 15;
constexpr int NUM_PREY = 100;

enum Terrain {Standard};

struct Cell{
    Terrain type;
    float foodAmount;

    Cell() : type(Terrain::Standard), foodAmount(0.0f) {};
};

struct FoodChunk {
    float totalFood;
    float centerX;
    float centerY;

    vector<sf::Vector2i> activeCells;

    FoodChunk() : totalFood(0.0f), centerX(0.0f), centerY(0.0f) {}
};