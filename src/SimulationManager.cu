#include <future>
#include <numeric>
#include <algorithm>
#include <iostream>
#include <fstream>
#include "SimulationManager.h"
#include "Core/GlobalHelpers.h"
#include "Core/GpuConfig.h"
#include "functions/BrainSystem.h"
#include "functions/LifeSystem.h"

// Constructor
SimulationManager::SimulationManager(int cores, uint64_t seed, const RunConfig& c) : numCores(cores), cfg(c), runSeed(seed) {
    // Initialize N worlds
    int prey = (cfg.numPrey >= 0) ? cfg.numPrey : NUM_PREY;
    int pred = (cfg.numPredators >= 0) ? cfg.numPredators : NUM_PREDATOR;
    float gr = (cfg.grassRadius > 0.0f) ? cfg.grassRadius : GRASS_SENSING_RADIUS;
    float pv = (cfg.predViewRadius > 0.0f) ? cfg.predViewRadius : PREDATOR_VIEW_RADIUS;

    for (int i = 0; i < numCores; i++) {
        worlds.push_back(make_unique<World>(prey, pred, i, splitmix64(runSeed + (uint64_t)i), gr, pv));

        // Launch persistent threads
        workers.emplace_back([this] {
            while(true){
                function<void()> task;
                {
                    unique_lock<mutex> lock(this->queueMutex);
                    this->condition.wait(lock, [this]{ 
                        return this->stopPool || !this->tasks.empty();
                    });

                    if (this->stopPool && this->tasks.empty()) return;

                    task = std::move(this->tasks.front());
                    this->tasks.pop();
                }
                // Execute the task
                task();
                
                if (--tasksRemaining == 0) {
                    unique_lock<mutex> lock(this->syncMutex);
                    this->syncCondition.notify_one();
                }
            }
        });
    }
}

// Destructor
SimulationManager::~SimulationManager() {
    {
        unique_lock<mutex> lock(queueMutex);
        stopPool = true;
    }
    // Wake up all threads to let them exit
    condition.notify_all();
    for (thread &worker : workers) {
        worker.join();
    }
}

// Checks whether to end the simulation
bool SimulationManager::finished() const {
    if (cfg.maxGenerations > 0 && generationCount >= cfg.maxGenerations) return true;
    if (cfg.maxTicks > 0 && totalTicks >= cfg.maxTicks) return true;
    return false;
}

// Updates all worlds
void SimulationManager::update(float dt, bool renderEnabled) {
    if (worlds.empty()) return;

    float baseDuration = (cfg.genDuration > 0.0f) ? cfg.genDuration : STARTING_GENERATION_DURATION;
    float currentDuration = min(MAXIMUM_GENERATION_DURATION, baseDuration + generationCount * GENERATION_SCALING_FACTOR);

    if (renderEnabled) {
        // Only update the first world if rendering is enabled to maintain performance
        worlds[0]->update(dt, this->generationCount);

        CUDA_CHECK(cudaStreamSynchronize(cudaStreamPerThread));
        this->generationTimer += dt;
        totalTicks++;
    } 
    else {
        tasksRemaining = (int)worlds.size();

        // Dynamically determine batch size
        float timeRemaining = currentDuration - generationTimer;
        int ticksRemaining = (int)ceil(timeRemaining / dt);

        int batchSize = std::min(MAXIMUM_BATCH_SIZE, ticksRemaining);
        
        // Enssure correct batch size when generation is ending
        if (batchSize <= 0) batchSize = 1;

        bool sample = (++batchesSinceSample >= 10);
        if (sample) batchesSinceSample = 0;

        auto wallStart = std::chrono::high_resolution_clock::now();

        for (size_t wi = 0; wi < worlds.size(); wi++) {
            World* w = worlds[wi].get(); 
            bool sampleThis = sample && (wi == 0);
            
            {
                unique_lock<mutex> lock(queueMutex);
                tasks.emplace([w, dt, batchSize, sampleThis, this] {
                    for (int i = 0; i < batchSize; i++) {
                        w->update(dt, this->generationCount, sampleThis && (i == 0));
                    }
                    CUDA_CHECK(cudaStreamSynchronize(cudaStreamPerThread));
                    w->readProfile();
                });
            }
        }

        // Wake up all threads
        condition.notify_all();
        
        // Wait for all threads to finish their batch
        unique_lock<mutex> lock(syncMutex);
        syncCondition.wait(lock, [this] { return tasksRemaining == 0; });

        double wallMs = std::chrono::duration<double, std::milli>(
            std::chrono::high_resolution_clock::now() - wallStart).count();
    
        if (wallMs > 0.0) ticksPerSecond = batchSize / (wallMs / 1000.0);
        totalTicks += (uint64_t)batchSize;
        generationTimer += dt * batchSize;
    }

    // Check for evolution
    if (generationTimer >= currentDuration) {
        generationTimer = 0.0f;
        generationCount++;
        evolve();
    }
}

// Counts the number of living prey and predators across all worlds
void SimulationManager::countSpecies() {
    preyCount = 0;
    predCount = 0;
    for (auto& world : worlds) {
        SwarmData& s = world->swarm;
        for (int i = 0; i < *s.current_count; i++) {
            if (!s.agentIdentifications.isAlive[i]) continue;
            if (s.agentIdentifications.speciesID[i] == PREY_ID) preyCount++;
            else predCount++;
        }
    }
}

// USed for profiling
void SimulationManager::emitRunHeader() const {
    if (!cfg.emitJson) return;
    cout << "{\"type\":\"header\""
         << ",\"tag\":\"" << cfg.tag << "\""
         << ",\"seed\":" << runSeed
         << ",\"device\":\"" << GpuConfig::deviceName << "\""
         << ",\"sms\":" << GpuConfig::smCount
         << ",\"clock_khz\":" << GpuConfig::clockKHz
         << ",\"cma\":" << GpuConfig::concurrentManagedAccess
         << ",\"grid\":" << GpuConfig::persistentGrid
         << ",\"worlds\":" << worlds.size()
         << ",\"world_x\":" << NUM_CELLE_X
         << ",\"world_y\":" << NUM_CELLE_Y
         << ",\"lattice_cell\":" << LATTICE_CELL_WIDTH
         << ",\"food_cell\":" << FOOD_CELL_WIDTH
         << ",\"max_per_cell\":" << MAX_AGENTS_PER_CELL
         << ",\"hidden\":" << HIDDEN_LAYER_SIZE
         << ",\"capacity\":" << MAX_SWARM_CAPACITY
         << ",\"food_refresh\":" << FOOD_CHUNK_REFRESH_INTERVAL
         << ",\"food_cells\":" << CONSTANT_FOOD_AMOUNT
         << "}" << endl;
}

void SimulationManager::emitGenerationRow() {
    const TickProfile& p = worlds[0]->lastProfile;

    if (cfg.emitJson) {
        cout << "{\"type\":\"gen\""
             << ",\"tag\":\"" << cfg.tag << "\""
             << ",\"gen\":" << generationCount
             << ",\"ticks\":" << totalTicks
             << ",\"ticks_per_s\":" << (int)ticksPerSecond
             << ",\"prey\":" << preyCount
             << ",\"pred\":" << predCount
             << ",\"agents\":" << (preyCount + predCount)
             << ",\"us_lattice\":" << p.buckets
             << ",\"us_sense\":"   << p.obs
             << ",\"us_think\":"   << p.think
             << ",\"us_move\":"    << p.move
             << ",\"us_cleanup\":" << p.cleanup
             << ",\"us_total\":"   << p.total
             << "}" << endl;
    } else {
        cout << "--- Generation " << generationCount << " ---" << endl;
        cout << "  " << (int)ticksPerSecond << " ticks/s"
             << "  | prey " << preyCount << "  pred " << predCount << endl;
        if (p.valid && p.total > 0.0f) {
            auto pct = [&](float v){ return (int)(100.0f * v / p.total); };
            cout << "  tick " << (int)p.total << " us:"
                 << "  lattice " << pct(p.buckets) << "%"
                 << "  sense "   << pct(p.obs)     << "%"
                 << "  think "   << pct(p.think)   << "%"
                 << "  move "    << pct(p.move)    << "%"
                 << "  cleanup " << pct(p.cleanup) << "%" << endl;
        }
    }
}

// Evolves agents by polling best ones 
void SimulationManager::evolve() {
    CUDA_CHECK(cudaDeviceSynchronize());

    countSpecies();
    emitGenerationRow();

    cout << "--- Generation " << generationCount << " Complete ---" << endl;
    
    // Testing predator numbers
    int prey = 0, predators = 0;
    const SwarmData& swarm = worlds[0]->swarm;
    for (int i = 0; i < *swarm.current_count; i++){
        if (!swarm.agentIdentifications.isAlive[i]) continue;
        if (swarm.agentIdentifications.speciesID[i] == PREY_ID) prey++;
        else predators++;
    }
    cout << "Living Prey: " << prey << ", Living Predators: " << predators << endl;
    
    vector<AgentEvaluation> allPrey;
    vector<AgentEvaluation> allPredators;

    // Collection of all agents from all worlds
    for (auto& world : worlds){
        // Collect living
        SwarmData& swarm = world->swarm;
        GraveyardData& graveyard = world->graveyard;

        // Living agents extraction
        for (int i = 0; i < *swarm.current_count; i++){
            if (!swarm.agentIdentifications.isAlive[i]) continue;
            float fitness = LifeSystem::getFitness(swarm, i);

            vector<float> brain = BrainSystem::extractBrain(swarm, i);
            if (swarm.agentIdentifications.speciesID[i] == PREY_ID) {
                allPrey.push_back({fitness, std::move(brain)});
            } else {
                allPredators.push_back({fitness, std::move(brain)});
            }
        }

        // Dead agents' extraction from the graveyard
        for (int i = 0; i < *graveyard.current_count; i++){
            vector<float> brain;
            brain.reserve(W01_SIZE + W12_SIZE + B0_SIZE + B1_SIZE);
            const int gcap = graveyard.max_capacity;

            for(int j = 0; j < W01_SIZE; j++) brain.push_back(graveyard.w01[j * gcap + i]);
            for(int j = 0; j < W12_SIZE; j++) brain.push_back(graveyard.w12[j * gcap + i]);
            for(int j = 0; j < B0_SIZE; j++)  brain.push_back(graveyard.b0[j * gcap + i]);
            for(int j = 0; j < B1_SIZE; j++)  brain.push_back(graveyard.b1[j * gcap + i]);

            if (graveyard.speciesID[i] == PREY_ID) {
                allPrey.push_back({graveyard.fitness[i], std::move(brain)});
            } else {
                allPredators.push_back({graveyard.fitness[i], std::move(brain)});
            }
        }
    }

    cout << allPrey.size() << " Prey and " << allPredators.size() << " Predators to select from." << endl;

    // Sort for selection
    sort(allPrey.begin(), allPrey.end(),
        [](const AgentEvaluation& a, const AgentEvaluation& b){
            return a.fitness > b.fitness;
        });

    sort(allPredators.begin(), allPredators.end(),
        [](const AgentEvaluation& a, const AgentEvaluation& b){
            return a.fitness > b.fitness;
        });

    // Prey Evolution
    if (!allPrey.empty()){
        if (generationCount % HOF_GENERATION_UPDATE_RATE == 0){
            preyHallOfFame.push_back(allPrey[0].brain);
            if (preyHallOfFame.size() > HALL_OF_FAME_SIZE){
                preyHallOfFame.erase(preyHallOfFame.begin());
            }
        }

        cout << "Best Prey Fitness: " << allPrey[0].fitness << endl;

        // Take top 10% -- capped to avoid dilution
        int eliteCount = max(1, (int)(allPrey.size() * TOP_PERCENTAGE));
        eliteCount = min(MAXIMUM_ELITE_COUNT, eliteCount);

        this->elitePreyBrains.clear();
        for (int i = 0; i < eliteCount; i++){
            elitePreyBrains.push_back(allPrey[i].brain);
        }

        this->bestWeightsPrey = allPrey[0].brain;
    }
    
    // Predator evolution
    if (!allPredators.empty()){
        if (generationCount % HOF_GENERATION_UPDATE_RATE == 0){
            predatorHallOfFame.push_back(allPredators[0].brain);
            if (predatorHallOfFame.size() > HALL_OF_FAME_SIZE){
                predatorHallOfFame.erase(predatorHallOfFame.begin());
            }
        }

        cout << "Best Predator Fitness: " << allPredators[0].fitness << endl;

        // Take top 10%
        int eliteCount = max(1, (int)(allPredators.size() * TOP_PERCENTAGE));
        eliteCount = min(MAXIMUM_ELITE_COUNT, eliteCount);

        this->elitePredatorBrains.clear();
        for (int i = 0; i < eliteCount; i++){
            elitePredatorBrains.push_back(allPredators[i].brain);
        }

        // Logging only 
        this->bestWeightsPredator = allPredators[0].brain;
    }

    // Logging
    float bestPreyFit = allPrey.empty() ? 0 : allPrey[0].fitness;
    float bestPredFit = allPredators.empty() ? 0 : allPredators[0].fitness;

    ofstream logFile("../logs/generation_stats.csv", ios_base::app);
    if (logFile.is_open()) {
        logFile.seekp(0, std::ios_base::end); 
        if (logFile.tellp() == 0) {
            logFile << "Generation,PreyFitness,PredatorFitness\n";
        }
        logFile << generationCount << "," << bestPreyFit << "," << bestPredFit << "\n";
        logFile.close();
    }

    // Checkpoint the weights every HOF_GENERATION_UPDATE_RATE generations
    if (generationCount % HOF_GENERATION_UPDATE_RATE == 0) {
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
    // Re-use worlds
    int prey = (cfg.numPrey >= 0) ? cfg.numPrey : NUM_PREY;
    int pred = (cfg.numPredators >= 0) ? cfg.numPredators : NUM_PREDATOR;
    float gr = (cfg.grassRadius > 0.0f) ? cfg.grassRadius : GRASS_SENSING_RADIUS;
    float pv = (cfg.predViewRadius > 0.0f) ? cfg.predViewRadius : PREDATOR_VIEW_RADIUS;

    for (int i = 0; i < (int)worlds.size(); i++){
        worlds[i]->reset(prey, pred, gr, pv);
    }

    // Adjust mutation parameters based on current generation count
    float dynamicRate = std::max(MINIMUM_MUTATION_RATE, STARTING_MUTATION_RATE - (generationCount * MUTATION_RATE_DECAY));
    float dynamicStrength = std::max(MINIMUM_MUTATION_STRENGTH, STARTING_MUTATION_STRENGTH - (generationCount * MUTATION_STRENGTH_DECAY));

    // Give every new prey and predator the previous agent's brain + mutation
    for (auto& world : worlds) {
        SwarmData& swarm = world->swarm;

        for (int i = 0; i < *swarm.current_count; i++){
            float roll = randomFloat(0.0f, 1.0f);
            int species = swarm.agentIdentifications.speciesID[i];
            // Prey
            if (species == PREY_ID && !elitePreyBrains.empty()) {
                if (!preyHallOfFame.empty() && roll < HOF_POOL_INJECTION_RATE){
                    int index = (int)(randomFloat(0.0f, preyHallOfFame.size() - 0.001f));
                    BrainSystem::insertBrain(swarm, i, preyHallOfFame[index]);
                } else if (roll >= RANDOM_INJECTION_RATE) {
                    // Over a threshold, pick from elite
                    int parentIndex = (int)(randomFloat(0.0f, elitePreyBrains.size() - 0.001f));
                    vector<float> brainToMutate = elitePreyBrains[parentIndex];
                    BrainSystem::mutateVector(brainToMutate, 0 , brainToMutate.size(), dynamicRate, dynamicStrength);
                    BrainSystem::insertBrain(swarm, i, brainToMutate);
                }
            }
            
            // Predators
            if (species == PREDATOR_ID && !elitePredatorBrains.empty()) {
                if (!predatorHallOfFame.empty() && roll < HOF_POOL_INJECTION_RATE){
                    int index = (int)(randomFloat(0.0f, predatorHallOfFame.size() - 0.001f));
                    BrainSystem::insertBrain(swarm, i, predatorHallOfFame[index]);
                } else if (roll >= RANDOM_INJECTION_RATE) {
                    // Over a threshold, pick from elite
                    int parentIndex = (int)(randomFloat(0.0f, elitePredatorBrains.size() - 0.001f));
                    vector<float> brainToMutate = elitePredatorBrains[parentIndex];
                    BrainSystem::mutateVector(brainToMutate, 0 , brainToMutate.size(), dynamicRate, dynamicStrength);
                    BrainSystem::insertBrain(swarm, i, brainToMutate);
                }
            }
        }
    }
}

#ifdef USE_SFML_GUI

void SimulationManager::draw(sf::RenderWindow& window) {
    if (!worlds.empty()) {
        worlds[0]->draw(window);
    }
}

void SimulationManager::resizeTexture(int w, int h) {
    if (!worlds.empty()) worlds[0]->resizeGridTexture(w, h);
}

#endif

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