#pragma once
#include <vector>
#include <memory>
#include "World.h"
#include "Entities/Prey.h"
#include "Entities/Predator.h"

using namespace std;

class SimulationManager {
private:
    vector<unique_ptr<World>> worlds;
    int numCores;
    
    // Genetic Algorithm Settings
    float generationTimer = 0.0f;
    const float GENERATION_DURATION = 60.0f; // Seconds per generation
    int generationCount = 0;
    
    // The "Master Brain" (Average of the best from previous gen)
    vector<float> bestWeightsPrey;
    vector<float> bestWeightsPredator;

public:
    SimulationManager(int cores);

    void update(float dt, bool renderEnabled);
    void evolve();

    void resetSimulation();

    // Helper to draw the main world
    void draw(sf::RenderWindow& window);
    
    // Helper to pass resize events
    void resizeTexture(int w, int h);
};