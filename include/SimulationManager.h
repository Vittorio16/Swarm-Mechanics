#pragma once
#include <vector>
#include <memory>
#include <string>
#include "World.h"
#include "Entities/Prey.h"
#include "Entities/Predator.h"
#include "Core/Config.h"

using namespace std;

class SimulationManager {
private:
    vector<unique_ptr<World>> worlds;
    int numCores;
    
    vector<vector<float>> predatorHallOfFame;
    vector<vector<float>> preyHallOfFame;
    
    // Genetic Algorithm Settings
    float generationTimer = 0.0f;
    int generationCount = 0;
    
    // The "Master Brain" (pools from this for new agents), and average of the best from previous gen
    vector<vector<float>> elitePreyBrains;
    vector<vector<float>> elitePredatorBrains;
    
    vector<float> bestWeightsPrey;
    vector<float> bestWeightsPredator;

public:
    SimulationManager(int cores);

    void update(float dt, bool renderEnabled);
    void evolve();

    void resetSimulation();

    // Loads pre-trained brains and hall of fame from files, and fast-forwards the simulation to a specified generation
    void loadPreTrainedBrains(int startGeneration, const string& preyBrains, const string& predatorBrains, 
        const string& hallOfFamePrey = "", const string& hallOfFamePredator = "",
        const string& elitePrey = "", const string& elitePredator = "");

    // Helper to draw the main world
    void draw(sf::RenderWindow& window);
    
    // Helper to pass resize events
    void resizeTexture(int w, int h);
};