#include <SFML/Graphics.hpp>
#include <thread>
#include <iostream>
#include <fstream>
#include "Core/Config.h"
#include "SimulationManager.h"

using namespace std;

// Helper used to run profiling on the simulation 
void runProfiling(){
    cout << "Avvio Profiling..." << std::endl;
    
    ofstream csvFile("../profiling/data/profiling_results.csv");
    csvFile << "NumPrey,NumPredators,Buckets,Observation,Think,Move,Cleanup,Total,Check_total\n";

    // Define the different numbers of agents to test
    vector<int> predator_count = {1, 2, 4, 8, 16, 32, 64, 128, 256, 512, 1024, 2048, 4096, 8192, 16384, 32768, 65536, 131072};
    vector<int> prey_count = {1, 2, 4, 8, 16, 32, 64, 128, 256, 512, 1024, 2048, 4096, 8192, 16384, 32768, 65536, 131072};
    // vector<int> predator_count = {1, 2, 4, 8, 16, 32, 64, 128, 256, 512, 1024, 2048, 4096, 8192, 16384, 32768, 65536, 131072};

    // Iterations for averaging the profiling results
    int iterations = 200;

    for (int i = 0; i < prey_count.size(); i++) {
        World world(prey_count[i], predator_count[i]);

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
    if (PROFILING_ENABLED) {
        runProfiling();
        return 0;
    }

    int numCores = std::thread::hardware_concurrency();
    // int numCores = 6;
    if (numCores == 0) numCores = FALLBACK_CORE_NUMBER; // Fallback

    cout << "Running on " << numCores - 1 /* 1 */<< " logical cores." << endl;

    sf::RenderWindow window(sf::VideoMode::getDesktopMode(), "Swarm Evolution - LOADING...");
    
    // Draws the window before starting to setup memory
    window.clear(sf::Color(30, 30, 30));
    
    sf::Font font;
    // if (font.loadFromFile("Roboto-Regular.ttf")) {
    //     sf::Text loadingText("Allocating CUDA Memory for " + std::to_string(numCores - 1) + " Worlds...\n\nThis is a heavy operation and takes 10-20 seconds.\nPlease do not close...", font, 40);
    //     loadingText.setPosition(100.0f, 100.0f);
    //     loadingText.setFillColor(sf::Color::White);
    //     window.draw(loadingText);
    // }
    window.display(); 

    // Memory allocation in CUDA
    cout << "Allocating Unified Memory... Please wait, do not press CTRL+C." << endl;
    SimulationManager simManager(numCores - 1);
    cout << "Allocation Complete! Starting simulation." << endl;

    window.setTitle("Swarm Evolution");
    bool renderingEnabled = true;
    sf::Clock clock;

    window.setFramerateLimit(MAX_FPS);

    // Optional: Start in replay mode with a pre-trained brain
    if (REPLAY_MODE_ENABLED) {
        simManager.loadPreTrainedBrains(
            51, 
            "../elite_logs/weights_prey_gen_51.txt", 
            "../elite_logs/weights_pred_gen_51.txt",
            "../elite_logs/hof_prey_gen_51.txt",      
            "../elite_logs/hof_pred_gen_51.txt",      
            "../elite_logs/elite_prey_gen_51.txt",
            "../elite_logs/elite_pred_gen_51.txt"   
        );
    }

    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) window.close();
            
            if (event.type == sf::Event::Resized) {
                sf::FloatRect visibleArea(0, 0, event.size.width, event.size.height);
                window.setView(sf::View(visibleArea));
                simManager.resizeTexture(event.size.width, event.size.height);
            }
            
            // TOGGLE MODES
            if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::R) {
                renderingEnabled = !renderingEnabled;
                if (renderingEnabled) {
                    window.setTitle("Sim (Real-Time)");
                    window.setFramerateLimit(MAX_FPS); 
                } else {
                    window.setTitle("Sim (Training)");
                    window.setFramerateLimit(0);
                }
            }
        }

        float dt;
        if (renderingEnabled) {
            dt = clock.restart().asSeconds();
            if (dt > MINIMUM_REAL_TIME_STEP) dt = MINIMUM_REAL_TIME_STEP;
        } else {
            if (DEBUGGING_ENABLED) dt = 0;
            else dt = FIXED_TIME_STEP;
            clock.restart(); 
        }

        simManager.update(dt, renderingEnabled);

        if (renderingEnabled) {
            window.clear();
            simManager.draw(window);
            window.display();
        }
    }
}