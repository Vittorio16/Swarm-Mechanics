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
    if (renderEnabled) {
        generationTimer += dt;
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

        generationTimer += dt * batchSize;
    }

    if (generationTimer >= GENERATION_DURATION) {
        generationTimer = 0.0f;
        generationCount++;
        evolve();
    }
}

// Evolves agents by merging best ones and getting their weights 
void SimulationManager::evolve() {
    cout << "--- Generation " << generationCount << " Complete ---" << endl;
    
    vector<Prey*> allPrey;
    vector<Predator*> allPredators;

    //Collection of all agents from all worlds
    for (auto& world : worlds){
        // Collect living
        for (auto& agent : world->agents){
            if (agent->speciesID == -1){
                allPrey.push_back(static_cast<Prey*>(agent.get()));
            } else {
                allPredators.push_back(static_cast<Predator*>(agent.get()));
            }
        }
        // Collect dead
        for (auto& agent : world->graveyard){
            if (agent->speciesID == -1){
                allPrey.push_back(static_cast<Prey*>(agent.get()));
            } else {
                allPredators.push_back(static_cast<Predator*>(agent.get()));
            }
        }
    }

    // Sort for getting the average
    sort(allPrey.begin(), allPrey.end(),
        [](Prey* a, Prey* b){
            return a->getFitness() > b->getFitness();
        });

    sort(allPredators.begin(), allPredators.end(),
        [](Predator* a, Predator* b){
            return a->getFitness() > b->getFitness();
        });

    // Average and save
    if (!allPrey.empty()){
        cout << "Best Prey Fitness: " << allPrey[0]->getFitness() << endl;

        // Take top 10%
        int eliteCount = max(1, (int)(allPrey.size() * 0.1f));

        // To minimize influence of randomly bad simulations,
        // we take the weighted average of the weights based on fitness
        float totalEliteFitness = 0;
        for (int i = 0; i < eliteCount; i++){
            totalEliteFitness = max(0.001f, allPrey[i]->getFitness());
        }

        vector<float> sumWeights = allPrey[0]->getBrain().getWeights(); 

        for (int i = 1; i < eliteCount; i++){
            vector<float> w = allPrey[i]->getBrain().getWeights(); 

            float agentFitness = max(0.001f, allPrey[i]->getFitness());
            float influence = agentFitness / totalEliteFitness;

            for (size_t j = 0; j < w.size(); j++){
                sumWeights[j] += w[j] * influence;
            }
        }

        // Compute Average
        for (size_t j = 0; j < sumWeights.size(); j++){
            sumWeights[j] /= (float)eliteCount;
        }
        this->bestWeightsPrey = sumWeights; 
    }
    
    if (!allPredators.empty()){
        cout << "Best Predator Fitness: " << allPredators[0]->getFitness() << endl;

        // Take top 10%
        int eliteCount = max(1, (int)(allPredators.size() * 0.1f));
        vector<float> sumWeights = allPredators[0]->getBrain().getWeights(); 

        for (int i = 1; i < eliteCount; i++){
            vector<float> w = allPredators[i]->getBrain().getWeights(); 
            for (size_t j = 0; j < w.size(); j++){
                sumWeights[j] += w[j];
            }
        }

        // Compute Average
        for (size_t j = 0; j < sumWeights.size(); j++){
            sumWeights[j] /= (float)eliteCount;
        }
        this->bestWeightsPredator= sumWeights; 
    }
    // Reset simulation to apply these new weights
    resetSimulation();
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