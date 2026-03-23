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
    float currentDuration = min(180.0f, GENERATION_DURATION + generationCount * 3.0f);

    if (renderEnabled) {
        // Only update the first world if rendering is enabled to maintain performance
        worlds[0]->update(dt, this->generationCount);
        this->generationTimer += dt;
    } 
    else {
        vector<future<void>> futures;

        // Dynamically determine batch size
        float timeRemaining = currentDuration - this->generationTimer;
        int ticksRemaining = (int)ceil(timeRemaining / dt);

        int batchSize = std::min(200, ticksRemaining);
        
        if (batchSize <= 0) batchSize = 1;

        for (auto& world : worlds) {
            World* w = world.get(); 
            
            futures.push_back(async(launch::async, 
                [w, dt, batchSize, this]() { 
                    for (int i = 0; i < batchSize; i++){
                        w->update(dt, this->generationCount); 
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
    if (generationTimer >= currentDuration) {
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

    // Collection of all agents from all worlds
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

    cout << allPrey.size() << " Prey and " << allPredators.size() << " Predators to select from." << endl;

    // Sort for selection
    sort(allPrey.begin(), allPrey.end(),
        [](Prey* a, Prey* b){
            return a->getFitness() > b->getFitness();
        });

    sort(allPredators.begin(), allPredators.end(),
        [](Predator* a, Predator* b){
            return a->getFitness() > b->getFitness();
        });

    // --- PREY EVOLUTION ---
    if (!allPrey.empty()){
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

        this->elitePreyBrains.clear();
        for (int i = 0; i < eliteCount; i++){
            elitePreyBrains.push_back(allPrey[i]->getBrain().getWeights());
        }

        // Logging only
        this->bestWeightsPrey = allPrey[0]->getBrain().getWeights();
    }
    
    // --- PREDATOR EVOLUTION ---
    if (!allPredators.empty()){
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

        this->elitePredatorBrains.clear();
        for (int i = 0; i < eliteCount; i++){
            elitePredatorBrains.push_back(allPredators[i]->getBrain().getWeights());
        }

        // Logging only 
        this->bestWeightsPredator = allPredators[0]->getBrain().getWeights();
    }

    // --- LOGGING & CHECKPOINTING ---
    float bestPreyFit = allPrey.empty() ? 0 : allPrey[0]->getFitness();
    float bestPredFit = allPredators.empty() ? 0 : allPredators[0]->getFitness();

    ofstream logFile("../logs/generation_stats.csv", ios_base::app);
    if (logFile.is_open()) {
        logFile.seekp(0, std::ios_base::end); 
        if (logFile.tellp() == 0) {
            logFile << "Generation,PreyFitness,PredatorFitness\n";
        }
        logFile << generationCount << "," << bestPreyFit << "," << bestPredFit << "\n";
        logFile.close();
    }

    // Checkpoint the weights every 3 generations
    if (generationCount % 3 == 0) {
        string preyFilename = "../logs/weights_prey_gen_" + to_string(generationCount) + ".txt";
        string predFilename = "../logs/weights_pred_gen_" + to_string(generationCount) + ".txt";
        
        if (!bestWeightsPrey.empty()) saveWeightsToFile(preyFilename, bestWeightsPrey);
        if (!bestWeightsPredator.empty()) saveWeightsToFile(predFilename, bestWeightsPredator);

        string preyHoFFilename = "../logs/hof_prey_gen_" + to_string(generationCount) + ".txt";
        string predHoFFilename = "../logs/hof_pred_gen_" + to_string(generationCount) + ".txt";
        
        if (!preyHallOfFame.empty()) saveHallOfFameToFile(preyHoFFilename, preyHallOfFame);
        if (!predatorHallOfFame.empty()) saveHallOfFameToFile(predHoFFilename, predatorHallOfFame);

        // UPDATED: Dump the Elite Pools!
        string preyEliteFilename = "../logs/elite_prey_gen_" + to_string(generationCount) + ".txt";
        string predEliteFilename = "../logs/elite_pred_gen_" + to_string(generationCount) + ".txt";
        
        if (!elitePreyBrains.empty()) saveHallOfFameToFile(preyEliteFilename, elitePreyBrains);
        if (!elitePredatorBrains.empty()) saveHallOfFameToFile(predEliteFilename, elitePredatorBrains);
    }

    // Reset simulation to apply these new weights
    resetSimulation();
}

void SimulationManager::resetSimulation() {
    // Re-create worlds
    worlds.clear(); 
    for (int i = 0; i < numCores; i++) {
        worlds.push_back(make_unique<World>());
    }

    // Adjust mutation parameters based on current generation count
    float dynamicRate = std::max(0.02f, 0.20f - (generationCount * 0.005f));
    float dynamicStrength = std::max(0.05f, 0.40f - (generationCount * 0.01f));

    // Give every new prey and predator the "Master Brain" + Mutation
    for (auto& world : worlds) {
        for (auto& agent : world->agents) {
            float roll = randomFloat(0.0f, 1.0f);

            // PREY
            if (agent->speciesID == -1 && !elitePreyBrains.empty()) {
                Prey* p = static_cast<Prey*>(agent.get());

                if (!preyHallOfFame.empty() && roll < 0.10f){
                    int index = (int)(randomFloat(0.0f, preyHallOfFame.size() - 0.001f));

                    SimplePerceptron brain = p->getBrain();
                    brain.setWeights(preyHallOfFame[index]);
                    p->setBrain(brain);
                
                } else if (roll < 0.20f){
                    p->setBrain(SimplePerceptron());
                
                } else {
                    int parentIndex = (int)(randomFloat(0.0f, elitePreyBrains.size() - 0.001f));
                
                    SimplePerceptron brain = p->getBrain();
                    brain.setWeights(elitePreyBrains[parentIndex]);
                
                    brain.mutate(dynamicRate, dynamicStrength); 
                    p->setBrain(brain);
                }
            }
            
            // PREDATORS
            if (agent->speciesID == 1 && !elitePredatorBrains.empty()) {
                Predator* p = static_cast<Predator*>(agent.get());
                
                if (!predatorHallOfFame.empty() && roll < 0.10f){
                    int index = (int)(randomFloat(0.0f, predatorHallOfFame.size() - 0.001f));
                    
                    SimplePerceptron brain = p->getBrain();
                    brain.setWeights(predatorHallOfFame[index]);
                    p->setBrain(brain);
                
                } else if (roll < 0.20f){
                    p->setBrain(SimplePerceptron());
                
                } else {
                    int parentIndex = (int)(randomFloat(0.0f, elitePredatorBrains.size() - 0.001f));
                
                    SimplePerceptron brain = p->getBrain();
                    brain.setWeights(elitePredatorBrains[parentIndex]);
                
                    brain.mutate(dynamicRate, dynamicStrength); 
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

// Loads pre-trained brains and hall of fame from files, and fast-forwards the simulation to a specified generation
void SimulationManager::loadPreTrainedBrains(int startGeneration, const string& preyBrains, const string& predatorBrains, 
    const string& hallOfFamePrey, const string& hallOfFamePredator,
    const string& elitePrey, const string& elitePredator) {
    
    // Load absolute best
    vector<float> preyWeights = loadWeightsFromFile(preyBrains);
    vector<float> predatorWeights = loadWeightsFromFile(predatorBrains);

    if (!preyWeights.empty()) {
        this->bestWeightsPrey = preyWeights;
        cout << "Loaded Pre-trained Prey Brain!" << endl;
    }
    if (!predatorWeights.empty()) {
        this->bestWeightsPredator = predatorWeights;
        cout << "Loaded Pre-trained Predator Brain!" << endl;
    }

    // Load Elite Pools
    if (!elitePrey.empty()) {
        this->elitePreyBrains = loadHallOfFameFromFile(elitePrey);
        cout << "Loaded Prey Elite Pool! (" << this->elitePreyBrains.size() << " masters preserved)" << endl;
    } else if (!preyWeights.empty()) {
        this->elitePreyBrains.clear();
        this->elitePreyBrains.push_back(preyWeights);
    }

    if (!elitePredator.empty()) {
        this->elitePredatorBrains = loadHallOfFameFromFile(elitePredator);
        cout << "Loaded Predator Elite Pool! (" << this->elitePredatorBrains.size() << " masters preserved)" << endl;
    } else if (!predatorWeights.empty()) {
        this->elitePredatorBrains.clear();
        this->elitePredatorBrains.push_back(predatorWeights);
    }

    // Load Hall of Fame if specified
    if (!hallOfFamePrey.empty()) {
        this->preyHallOfFame = loadHallOfFameFromFile(hallOfFamePrey);
        cout << "Loaded Prey Hall of Fame! (" << this->preyHallOfFame.size() << " masters preserved)" << endl;
    }
    
    if (!hallOfFamePredator.empty()) {
        this->predatorHallOfFame = loadHallOfFameFromFile(hallOfFamePredator);
        cout << "Loaded Predator Hall of Fame! (" << this->predatorHallOfFame.size() << " masters preserved)" << endl;
    }

    // Fast-forward the simulation generation
    this->generationCount = startGeneration;
    cout << "Simulation fast-forwarded to Generation " << this->generationCount << "!" << endl;

    resetSimulation();
}