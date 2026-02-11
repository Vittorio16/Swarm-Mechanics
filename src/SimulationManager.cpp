#include <thread>
#include <future>
#include <numeric>
#include <algorithm>
#include <iostream>
#include "SimulationManager.h"

// Constructor
SimulationManager::SimulationManager(int cores) : numCores(cores) {
    // Initialize N worlds
    for (int i = 0; i < numCores; i++) {
        worlds.push_back(make_unique<World>());
    }
}

// Updates all worlds
void SimulationManager::update(float dt, bool renderEnabled) {
    generationTimer += dt;

    if (renderEnabled) {
        worlds[0]->update(dt);
    } 
    else {
        vector<future<void>> futures;
        
        int batchSize = 50;

        for (auto& world : worlds) {
            // Launch async task for each world
            futures.push_back(async(launch::async, 
                [&world, dt, batchSize]() { 
                    for (int i = 0; i < batchSize; i++){
                        world->update(dt); 
                    } 
                }
            ));
        }
        
        // Wait for all to finish (Implicitly happens when futures go out of scope, 
        // but getting them ensures synchronization)
        for (auto& f : futures) {
            f.get();
        }
    }

    if (generationTimer >= GENERATION_DURATION) {
        evolve();
        generationTimer = 0.0f;
    }
}

// Evolves agents by merging best ones and getting their weights 
void SimulationManager::evolve() {
    cout << "--- Generation " << generationCount << " Complete ---" << endl;
    
    // Handle getting best ones and merging their weights
    
    // Reset all worlds
    resetSimulation();
    generationCount++;
}

void SimulationManager::resetSimulation() {
    // Re-create worlds -- might want to optimize this
    worlds.clear(); 
    for (int i = 0; i < numCores; i++) {
        worlds.push_back(make_unique<World>());
    }

    // Give every new prey and predator the "Master Brain" + Mutation
    for (auto& world : worlds) {
        for (auto& agent : world->agents) {
            if (agent->speciesID == -1 && !bestWeightsPrey.empty()) {
                Prey* p = static_cast<Prey*>(agent.get());
                SimplePerceptron brain = p->getBrain();
                
                brain.setWeights(bestWeightsPrey);
                brain.mutate(); 
                
                p->setBrain(brain);
            }
            
            if (agent->speciesID == 1 && !bestWeightsPredator.empty()) {
                Predator* p = static_cast<Predator*>(agent.get());
                SimplePerceptron brain = p->getBrain();

                brain.setWeights(bestWeightsPredator);
                brain.mutate();

                p->setBrain(brain);
            }
        }
    }
}

void SimulationManager::draw(sf::RenderWindow& window) {
    if (!worlds.empty()) {
        worlds[0]->draw(window);
    }
}

void SimulationManager::resizeTexture(int w, int h) {
    if (!worlds.empty()) worlds[0]->resizeGridTexture(w, h);
}