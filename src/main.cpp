#include "World.h"
#include <SFML/Graphics.hpp>
using namespace std;

int main() {
    sf::RenderWindow window(sf::VideoMode(800, 600), "Sim");
    
    World myWorld;
    bool renderingEnabled = true; // Toggle this to turn off graphics

    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) window.close();
            
            // PRESS 'R' TO TOGGLE RENDERING
            if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::R) {
                renderingEnabled = !renderingEnabled;
                window.setTitle(renderingEnabled ? "Sim (Visible)" : "Sim (Headless - Fast!)");
            }
        }

        // --- 1. ALWAYS UPDATE LOGIC ---
        // Even if we don't draw, the simulation continues.
        // myWorld.update();

        // --- 2. CONDITIONALLY DRAW ---
        if (renderingEnabled) {
            window.clear();
            myWorld.draw(window); // Pass the window here!
            window.display();
        } else {
            // Optional: Sleep a tiny bit to prevent CPU burning if you don't want max speed
            // sf::sleep(sf::milliseconds(10));
        }
    }
}