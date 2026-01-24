#include <random>
#include <cmath>
#include "World.h"
#include "Entities/Predator.h"
#include "Entities/Prey.h"

using namespace std;

// Constructor
World::World() : grid(NUM_CELLE_X, vector<Cell>(NUM_CELLE_Y)) {
    // Setup random number generation
    random_device rd;
    mt19937 gen(rd());
    uniform_real_distribution<float> disX(0.0f, static_cast<float>(NUM_CELLE_X));
    uniform_real_distribution<float> disY(0.0f, static_cast<float>(NUM_CELLE_Y));

    // Puts set number of predators and preys in random positions
    for (int i = 0; i < NUM_PREDATOR; i++){
        float randX = disX(gen);
        float randY = disY(gen);

        agents.push_back(make_unique<Predator>(randX, randY));
    }
    for (int i = 0; i < NUM_PREY; i++){
        float randX = disX(gen);
        float randY = disY(gen);

        agents.push_back(make_unique<Prey>(randX, randY));
    }
}

// Updates the world each tick of the simulation
void World::update(float dt){
    // First it checks the system state and lets agents decide
    for (auto& agent : agents){
        agent->updateSensoryData();
        agent->think();
    }

    // Then updates everything at the same time
    for (auto& agent : agents){
        agent->move(dt);

        // Creates pacman style world
        if (agent->x > NUM_CELLE_X){
            agent->x -= NUM_CELLE_X;
        } else if (agent->x < 0){
            agent->x += NUM_CELLE_X;
        }
        if (agent->y > NUM_CELLE_Y){
            agent->y -= NUM_CELLE_Y;
        } else if (agent->y < 0){
            agent->y += NUM_CELLE_Y;
        }
    }
}

// Draws World with all cells and entities in a window
void World::draw(sf::RenderWindow& window){
    sf::Vector2u windowSize = window.getSize();

    float scaleX = windowSize.x / (float)NUM_CELLE_X;
    float scaleY = windowSize.y / (float)NUM_CELLE_Y;

    // Outline settings
    // Negative thickness draws the border INSIDE the cell so it doesn't overlap neighbors
    sf::RectangleShape cell;
    cell.setOutlineColor(sf::Color(128,128,128));
    //cell.setOutlineThickness(-1.0f); 

    for (int i = 0; i < NUM_CELLE_X; i++){
        for (int j = 0; j < NUM_CELLE_Y; j++){
            
            // // 1. Calculate Exact Pixel Coordinates (The "Integer Step" method)
            // // We calculate where this cell starts (x1) and where the NEXT cell starts (x2).
            // float x1 = i * scaleX;
            // float x2 = (i + 1) * scaleX;
            // float y1 = j * scaleY;
            // float y2 = ((j + 1) * windowSize.y) / (float)NUM_CELLE_Y;

            // // 2. Set Position and Size dynamically
            // // The size is simply the distance between the start and the next start.
            // cell.setPosition(x1, y1);
            // cell.setSize(sf::Vector2f(x2 - x1, y2 - y1));

            float x_pos = i * scaleX;
            float y_pos = j * scaleY;
            cell.setPosition(x_pos, y_pos);
            cell.setSize(sf::Vector2f(scaleX, scaleY));

            // 3. Color Logic
            if (grid[i][j].type == Terrain::Standard) {
                cell.setFillColor(sf::Color::White);
            }
            
            window.draw(cell);
        }
    }

    // Draw the agents
    sf::ConvexShape boidShape;
    boidShape.setPointCount(4);

    float size = scaleX * 0.8f;
    float width = size * 0.4f;  // Tail size

    // 0: point, 1: tail, 2: tail bakcdrop, 3: left side of tail
    boidShape.setPoint(0, sf::Vector2f(size / 2.0f, 0.0f));
    boidShape.setPoint(1, sf::Vector2f(-size / 2.0f, width));
    boidShape.setPoint(2, sf::Vector2f(-size / 2.0f + (size * 0.2f), 0.0f)); 
    boidShape.setPoint(3, sf::Vector2f(-size / 2.0f, -width));
    
    boidShape.setOutlineThickness(1.0f);
    
    for (const auto& agent : agents) {
        if (dynamic_cast<Predator*>(agent.get())) {
            boidShape.setFillColor(sf::Color::Red);
            boidShape.setOutlineColor(sf::Color(139, 0, 0));
        } 
        else if (dynamic_cast<Prey*>(agent.get())) {
            boidShape.setFillColor(sf::Color::Green);
            boidShape.setOutlineColor(sf::Color(0, 100, 0));

        }

        int pixelX = agent->x * scaleX;
        int pixelY = agent->y * scaleY; 

        boidShape.setPosition(pixelX, pixelY);
        
        // Rotation based on velocity
        if (agent->vx != 0 || agent->vy != 0) {
            float angleRadians = std::atan2(agent->vy, agent->vx);
            float angleDegrees = angleRadians * 180.0f / M_PI;
            
            boidShape.setRotation(angleDegrees);
        }
        window.draw(boidShape);
    }
}