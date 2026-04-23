#include <SFML/Graphics.hpp>
#include <thread>
#include <iostream>
#include "Core/Config.h"
#include "SimulationManager.h"

using namespace std;


int main() {
    int numCores = std::thread::hardware_concurrency();
    if (numCores == 0) numCores = FALLBACK_CORE_NUMBER; // Fallback

    cout << "Running on " << numCores - 1 << " logical cores." << endl;
    
    sf::RenderWindow window(sf::VideoMode::getDesktopMode(), "Swarm Evolution");
    bool renderingEnabled = true; // Toggle this to run faster evolution

    sf::Clock clock;

    SimulationManager simManager(numCores - 1);

    window.setFramerateLimit(MAX_FPS);

    // Optional: Start in replay mode with a pre-trained brain
    if (REPLAY_MODE_ENABLED) {
        // Load the sickest generation with FULL genetic diversity
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
                // Update the view to the new window size
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

        // --- ALWAYS UPDATE LOGIC ---
        // Even if we don't draw, the simulation continues.
        float dt;

        if (renderingEnabled) {
            // MODE A: Real-Time
            dt = clock.restart().asSeconds();
            if (dt > MINIMUM_REAL_TIME_STEP) dt = MINIMUM_REAL_TIME_STEP; // Cap to prevent huge jumps
        } else {
            // MODE B: Training
            if (DEBUGGING_ENABLED) {
                dt = 0;
            } else {
                dt = FIXED_TIME_STEP;
            }
            clock.restart(); 
        }

        simManager.update(dt, renderingEnabled);

        // --- 2. CONDITIONALLY DRAW ---
        if (renderingEnabled) {
            window.clear();
            simManager.draw(window);
            window.display();
        }
    }
}