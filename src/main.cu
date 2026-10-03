#ifdef USE_SFML_GUI
    #include <SFML/Graphics.hpp>
#endif

#include <thread>
#include <iostream>
#include <fstream>
#include <random>
#include "Core/Config.h"
#include "Core/GpuConfig.h"
#include "SimulationManager.h"

using namespace std;

// Helper used to run profiling on the simulation 
void runProfiling(){
    cout << "Avvio Profiling..." << std::endl;
    
    ofstream csvFile("../profiling/data/profiling_results.csv");
    csvFile << "NumPrey,NumPredators,Buckets,Observation,Think,Move,Cleanup,Total,Check_total\n";

    // Define the different numbers of agents to test
    vector<int> prey_count = {1, 2, 4, 8, 16, 32, 64, 128, 256, 512, 1024, 2048, 4096, 8192, 16384, 32768};
    vector<int> predator_count = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    // vector<int> predator_count = {1, 2, 4, 8, 16, 32, 64, 128, 256, 512, 1024, 2048, 4096, 8192, 16384, 32768, 65536, 131072};

    // Iterations for averaging the profiling results
    int iterations = 200;

    for (int i = 0; i < prey_count.size(); i++) {
        World world(prey_count[i], predator_count[i], 0);

        // These are used to keep the averages
        double sum_buckets = 0, sum_obs = 0, sum_think = 0;
        double sum_move = 0, sum_cleanup = 0, sum_total = 0, sum_check_total = 0;

        // Empty ticks to load in cache
        for(int w = 0; w < 5; w++) {
            world.update(FIXED_TIME_STEP, 0);
        }

        // Loop di profiling reale
        for (int j = 0; j < iterations; j++) {
            ProfilingData data = world.update(FIXED_TIME_STEP, 0);
            
            sum_buckets += data.t_buckets;
            sum_obs     += data.t_obs;
            sum_think   += data.t_think;
            sum_move    += data.t_move;
            sum_cleanup += data.t_cleanup;
            sum_total   += data.t_total;
            sum_check_total += data.check_t_total;
        }

        // Average the results over the number of iterations
        double avg_buckets = sum_buckets / iterations;
        double avg_obs     = sum_obs / iterations;
        double avg_think   = sum_think / iterations;
        double avg_move    = sum_move / iterations;
        double avg_cleanup = sum_cleanup / iterations;
        double avg_total   = sum_total / iterations;
        double avg_check_total = sum_check_total / iterations;

        // Single row in the CSV file
        csvFile << prey_count[i] << ","
                << predator_count[i] << ","
                << avg_buckets << ","
                << avg_obs << ","
                << avg_think << ","
                << avg_move << ","
                << avg_cleanup << ","
                << avg_total << ","
                << avg_check_total << "\n";
                
        std::cout << "Completato prey_count = " << prey_count[i] << ", predator_count = " << predator_count[i] << " (Tempo medio totale: " << avg_total << " us)" << std::endl;
    }

    csvFile.close();
    std::cout << "Profiling completato. Dati salvati in profiling_results.csv" << std::endl;
}

int main() {
    GpuConfig::init();
    uint64_t runSeed = std::random_device{}();
    std::cout << "Run seed: " << runSeed << std::endl;

    if (PROFILING_ENABLED) {
        runProfiling();
        return 0;
    }

    int numCores = std::thread::hardware_concurrency();
    if (numCores == 0) numCores = FALLBACK_CORE_NUMBER;
    cout << "Running on " << numCores - 1 << " logical cores." << endl;

    cout << "Allocating Unified Memory... Please wait." << endl;
    SimulationManager simManager(numCores - 1, runSeed);
    cout << "Allocation Complete! Starting simulation." << endl;

    if (REPLAY_MODE_ENABLED) {
        simManager.loadPreTrainedBrains(51, /* ... your paths ... */);
    }

#ifdef USE_SFML_GUI
    sf::RenderWindow window(sf::VideoMode::getDesktopMode(), "Swarm Evolution");
    bool renderingEnabled = true;
    sf::Clock clock;
    window.setFramerateLimit(MAX_FPS);

    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) window.close();
            if (event.type == sf::Event::Resized) {
                sf::FloatRect visibleArea(0, 0, event.size.width, event.size.height);
                window.setView(sf::View(visibleArea));
                simManager.resizeTexture(event.size.width, event.size.height);
            }
            if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::R) {
                renderingEnabled = !renderingEnabled;
            }
        }

        float dt;
        if (renderingEnabled) {
            dt = clock.restart().asSeconds();
            if (dt > MINIMUM_REAL_TIME_STEP) dt = MINIMUM_REAL_TIME_STEP;
        } else {
            dt = DEBUGGING_ENABLED ? 0 : FIXED_TIME_STEP;
            clock.restart(); 
        }

        simManager.update(dt, renderingEnabled);

        if (renderingEnabled) {
            window.clear(sf::Color(30, 30, 30));
            simManager.draw(window);
            window.display();
        }
    }
#else
    std::cout << "Starting Headless Simulation on Jetson..." << std::endl;
    // Infinitely fast-forward the simulation using the fixed time step
    while (true) {
        simManager.update(FIXED_TIME_STEP, false);
    }
#endif

    return 0;
}