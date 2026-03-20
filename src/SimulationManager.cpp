#include <thread>
#include <future>
#include <numeric>
#include <algorithm>
#include <iostream>
#include <fstream>
#include "SimulationManager.h"
#include "Core/GlobalHelpers.h"
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
        // Only update the first world if rendering is enabled to maintain performance
        worlds[0]->update(dt);
        generationTimer += dt;
    } 
    else {
        vector<future<void>> futures;
        int batchSize = 50;

        for (auto& world : worlds) {
            World* w = world.get(); 
            
            futures.push_back(async(launch::async, 
                [w, dt, batchSize]() { 
                    for (int i = 0; i < batchSize; i++){
                        w->update(dt); 
                    } 
                }
            ));
        }
        
        // Wait for all threads to finish their batch
        for (auto& f : futures) {
            f.get();
        }

        generationTimer += dt * batchSize;
    }

    // Check for evolution
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
        // Collect dea
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
        // Save to hall of fame
        if (generationCount % 3 == 0){
            preyHallOfFame.push_back(allPrey[0]->getBrain().getWeights());
            if (preyHallOfFame.size() > HALL_OF_FAME_SIZE){
                preyHallOfFame.erase(preyHallOfFame.begin());
            }
        }

        cout << "Best Prey Fitness: " << allPrey[0]->getFitness() << endl;

        // Take top 10% -- capped to avoid dilution
        int eliteCount = max(1, (int)(allPrey.size() * 0.1f));
        eliteCount = min(50, eliteCount);

        // To minimize influence of randomly bad simulations,
        // we take the weighted average of the weights based on fitness
        float totalEliteFitness = 0;
        for (int i = 0; i < eliteCount; i++){
            totalEliteFitness += max(0.001f, allPrey[i]->getFitness());
        }

        vector<float> sumWeights(allPrey[0]->getBrain().getWeights().size(), 0.0f); 

        for (int i = 0; i < eliteCount; i++){
            vector<float> w = allPrey[i]->getBrain().getWeights(); 

            float agentFitness = max(0.001f, allPrey[i]->getFitness());
            float influence = agentFitness / totalEliteFitness;

            for (size_t j = 0; j < w.size(); j++){
                sumWeights[j] += w[j] * influence;
            }
        }

        // Average already computed using totalEliteFitness
        this->bestWeightsPrey = sumWeights; 
    }
    
    if (!allPredators.empty()){
        // Save to hall of fame
        if (generationCount % 3 == 0){
            predatorHallOfFame.push_back(allPredators[0]->getBrain().getWeights());
            if (predatorHallOfFame.size() > HALL_OF_FAME_SIZE){
                predatorHallOfFame.erase(predatorHallOfFame.begin());
            }
        }

        cout << "Best Predator Fitness: " << allPredators[0]->getFitness() << endl;

        // Take top 10%
        int eliteCount = max(1, (int)(allPredators.size() * 0.1f));
        eliteCount = min(50, eliteCount);

        // To minimize influence of randomly bad simulations,
        // we take the weighted average of the weights based on fitness
        float totalEliteFitness = 0;
        for (int i = 0; i < eliteCount; i++){
            totalEliteFitness += max(0.001f, allPredators[i]->getFitness());
        }

        vector<float> sumWeights(allPredators[0]->getBrain().getWeights().size(), 0.0f);

        for (int i = 0; i < eliteCount; i++){
            vector<float> w = allPredators[i]->getBrain().getWeights(); 

            float agentFitness = max(0.001f, allPredators[i]->getFitness());
            float influence = agentFitness / totalEliteFitness;

            for (size_t j = 0; j < w.size(); j++){
                sumWeights[j] += w[j] * influence;
            }
        }

        // Average already computed using totalEliteFitness
        this->bestWeightsPredator= sumWeights; 
    }

    // Logginng
    float bestPreyFit = allPrey.empty() ? 0 : allPrey[0]->getFitness();
    float bestPredFit = allPredators.empty() ? 0 : allPredators[0]->getFitness();

    // 1. Log the stats to a CSV (appends a new line every generation)
    std::ofstream logFile("generation_stats.csv", ios_base::app);
    if (logFile.is_open()) {
        // Format: Generation, PreyFitness, PredatorFitness
        logFile << generationCount << "," << bestPreyFit << "," << bestPredFit << "\n";
        logFile.close();
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
            float roll = randomFloat(0.0f, 1.0f);

            if (agent->speciesID == -1 && !bestWeightsPrey.empty()) {
                Prey* p = static_cast<Prey*>(agent.get());

                // Sets the new brains either to hall of fame, random, or best of last gen with mutation
                if (!preyHallOfFame.empty() && roll < 0.10f){
                    int index = (int)(randomFloat(0, preyHallOfFame.size() - 1));
                    SimplePerceptron brain = p->getBrain();
                    
                    brain.setWeights(preyHallOfFame[index]);
                    brain.mutate(); 
                    
                    p->setBrain(brain);
                } else if (roll < 0.20f){
                    p->setBrain(SimplePerceptron());
                } else {
                    SimplePerceptron brain = p->getBrain();
                    
                    brain.setWeights(bestWeightsPrey);
                    brain.mutate(); 
                    
                    p->setBrain(brain);
                }
            }
            
            if (agent->speciesID == 1 && !bestWeightsPredator.empty()) {
                Predator* p = static_cast<Predator*>(agent.get());
                
                // Sets some agents' brains randomly to avoid local minima
                if (!predatorHallOfFame.empty() && roll < 0.10f){
                    int index = (int)(randomFloat(0, predatorHallOfFame.size() - 1));
                    SimplePerceptron brain = p->getBrain();
                    
                    brain.setWeights(predatorHallOfFame[index]);
                    brain.mutate(); 
                    
                    p->setBrain(brain);
                } else if (roll < 0.20f){
                    p->setBrain(SimplePerceptron());
                } else {
                    SimplePerceptron brain = p->getBrain();
                    
                    brain.setWeights(bestWeightsPredator);
                    brain.mutate(); 
                    
                    p->setBrain(brain);
                }
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