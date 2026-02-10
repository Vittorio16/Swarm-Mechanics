#include "World.h"
#include <SFML/Graphics.hpp>
using namespace std;

#include <iostream>

int main() {
    // Standard fixed step (1/200th of a second)
    const float FIXED_TIME_STEP = 1.0f / 200.0f;
    constexpr int MAX_FPS = 170;
    
    sf::RenderWindow window(sf::VideoMode::getDesktopMode(), "Sim");
    bool renderingEnabled = true; // Toggle this to run faster evolution

    sf::Clock clock;
    World myWorld;
    window.setFramerateLimit(MAX_FPS);

    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) window.close();
            
            if (event.type == sf::Event::Resized) {
                // Update the view to the new window size
                sf::FloatRect visibleArea(0, 0, event.size.width, event.size.height);
                window.setView(sf::View(visibleArea));

                myWorld.resizeGridTexture(event.size.width, event.size.height);
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

        // --- 1. ALWAYS UPDATE LOGIC ---
        // Even if we don't draw, the simulation continues.
        float dt;

        if (renderingEnabled) {
            // MODE A: Real-Time
            dt = clock.restart().asSeconds();
        } else {
            // MODE B: Training
            dt = FIXED_TIME_STEP;
            // dt = 0;
            clock.restart(); 
        }

        myWorld.update(dt);

        // --- 2. CONDITIONALLY DRAW ---
        if (renderingEnabled) {
            window.clear();
            myWorld.draw(window);
            window.display();
        } else {
            // Optional: Sleep a tiny bit to prevent CPU burning if you don't want max speed
            // sf::sleep(sf::milliseconds(10));
        }
    }
}