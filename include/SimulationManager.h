#pragma once
#include <vector>
#include <memory>
#include <string>
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <atomic>
#include <chrono>
#include "World.h"
#include "Core/Config.h"
#include "Core/RunConfig.h"

using namespace std;

struct AgentEvaluation {
    float fitness;
    vector<float> brain;
};

class SimulationManager {
private:
    // Handle thread pool - threads stay alive for the whole simulation
    vector<thread> workers;
    queue<function<void()>> tasks;

    mutex queueMutex;
    condition_variable condition;
    bool stopPool = false;
    
    // Keep track of batch of updates
    atomic<int> tasksRemaining;
    condition_variable syncCondition;
    mutex syncMutex;

    // Multiple worlds for parallel processing of updates
    vector<unique_ptr<World>> worlds;
    int numCores;
    
    vector<vector<float>> predatorHallOfFame;
    vector<vector<float>> preyHallOfFame;
    
    
    // Pool to pick from for next generation
    vector<vector<float>> elitePreyBrains;
    vector<vector<float>> elitePredatorBrains;
    
    vector<float> bestWeightsPrey;
    vector<float> bestWeightsPredator;
    
    // Genetic Algorithm Settings
    float generationTimer = 0.0f;
    int generationCount = 0;

    // Run configuration and live statistics
    RunConfig cfg;
    uint64_t runSeed = 0;
    uint64_t totalTicks = 0;
    double ticksPerSecond = 0.0;
    int batchesSinceSample = 0;
    int preyCount = 0;
    int predCount = 0;

    void evolve();
    void countSpecies();
    void emitGenerationRow();

public:
    SimulationManager(int cores, uint64_t runSeed);
    ~SimulationManager();

    void update(float dt, bool renderEnabled);

    void resetSimulation();

    bool finished() const;
    void emitRunHeader() const;
    
    // Loads pre-trained brains and hall of fame from files, and fast-forwards the simulation to a specified generation
    void loadPreTrainedBrains(int startGeneration, const string& preyBrains, const string& predatorBrains, 
        const string& hallOfFamePrey = "", const string& hallOfFamePredator = "",
        const string& elitePrey = "", const string& elitePredator = "");

    #ifdef USE_SFML_GUI
        // Helper to draw the main world
        void draw(sf::RenderWindow& window);
    
        // Helper to pass resize events
        void resizeTexture(int w, int h);
    #endif
};