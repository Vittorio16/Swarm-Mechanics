#include <SFML/Graphics.hpp>
#include <thread>
#include <iostream>
#include "SimulationManager.h"

using namespace std;


int main() {
    int numCores = std::thread::hardware_concurrency();
    if (numCores == 0) numCores = 4; // Fallback

    cout << "Running on " << numCores - 1 << " logical cores." << endl;

    // Standard fixed step (1/200th of a second)
    const float FIXED_TIME_STEP = 1.0f / 200.0f;
    constexpr int MAX_FPS = 170;
    
    sf::RenderWindow window(sf::VideoMode::getDesktopMode(), "Swarm Evolution");
    bool renderingEnabled = true; // Toggle this to run faster evolution

    sf::Clock clock;

    SimulationManager simManager(numCores - 1);

    window.setFramerateLimit(MAX_FPS);

    // Optional: Start in replay mode with a pre-trained brain
    bool replayMode = false; 
    if (replayMode) {
        // Load the sickest generation with FULL genetic diversity
        simManager.loadPreTrainedBrains(
            24, 
            "../logs/weights_prey_gen_24.txt", 
            "../logs/weights_pred_gen_24.txt",
            "../logs/hof_prey_gen_24.txt",      // Prey HoF
            "../logs/hof_pred_gen_24.txt",      // Predator HoF
            "../logs/elite_prey_gen_24.txt",    // Prey Elite Pool
            "../logs/elite_pred_gen_24.txt"     // Predator Elite Pool
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
            if (dt > 0.05f) dt = 0.05f;
        } else {
            // MODE B: Training
            // dt = 0; // Debugging mode
            dt = FIXED_TIME_STEP;
            clock.restart(); 
        }

        simManager.update(dt, renderingEnabled);

        // --- 2. CONDITIONALLY DRAW ---
        if (renderingEnabled) {
            window.clear();
            simManager.draw(window);
            window.display();
        } else {
            // Optional: Sleep a tiny bit to prevent CPU burning if you don't want max speed
            // sf::sleep(sf::milliseconds(10));
        }
    }
}