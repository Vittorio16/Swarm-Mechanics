#include <random>
#include <cmath>
#include <algorithm>
#include "World.h"
#include "Entities/Predator.h"
#include "Entities/Prey.h"
#include "Core/Physics.h"
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
        if (!agent->isAlive) continue;
        vector<Observation> agentsInFOV = getObservation(agent.get());

        agent->updateSensoryData(agentsInFOV);
        agent->think();
    }

    vector<unique_ptr<Agent>> nursery;

    // Then updates everything at the same time
    for (auto& agent : agents){
        if (!agent->isAlive) continue;

        // May want to separate movement from feeding
        agent->move(dt);
        // Handles reproduction 
        if (agent->energy > MAX_ENERGY){
            nursery.push_back(agent->reproduce());
        }    

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
    
    // Add newly born agents
    for (auto& baby : nursery){ 
        agents.push_back(std::move(baby));
    }
    // Erases dead agents
    agents.erase(
        remove_if(agents.begin(), agents.end(), 
            [](const std::unique_ptr<Agent>& a) {
                return !a->isAlive; 
            }),
        agents.end()
    );
}

// Given an observer, returns a vector of pointers to all the agents it can see
vector<Observation> World::getObservation(const Agent* observer){
    vector<Observation> observations;

    float observerHeading = atan2(observer->vy, observer->vx);
    for (const auto& otherUnique : agents){
        Agent* otherAgent = otherUnique.get();

        if (otherAgent == observer || !otherAgent->isAlive) continue;

        vector<float> coords = getThoroidalCoordinates(
            observer->x, observer->y, otherAgent->x, otherAgent->y, NUM_CELLE_X, NUM_CELLE_Y
        );

        float distSq = coords[0];
        float angleToTarget = coords[1];
        float dx = coords[2];
        float dy = coords[3];

        // Senses an area around the agent
        if (distSq < observer->rangeOfVision){
            observations.emplace_back(dx, dy, distSq, otherAgent);
            continue;
        }

        if (distSq > (observer->viewRadius*observer->viewRadius)) continue;

        float angleDiff = angleToTarget - observerHeading;
        // Normalize angle difference to be between -PI and PI
        while (angleDiff <= -M_PI) angleDiff += 2 * M_PI;
        while (angleDiff > M_PI) angleDiff -= 2 * M_PI;
        
        if (abs(angleDiff) < observer->fovAngle * M_PI / 360.0f){
            observations.emplace_back(dx, dy, distSq, otherAgent);
        }
    }
    return observations;
}

// Call this when window starts or resizes
void World::resizeGridTexture(int width, int height) {
    if (width == 0 || height == 0) return;
    gridTexture.create(width, height);
    gridTextureValid = false; 
}

// Redraws the whole background when nececcary
void World::remapBackground(sf::Vector2u windowSize, float scaleX, float scaleY){
        // Ensure texture exists and is correct size
        if (gridTexture.getSize() != windowSize) {
            gridTexture.create(windowSize.x, windowSize.y);
        }

        gridTexture.clear(sf::Color::Black);
        
        sf::RectangleShape cell;
        cell.setOutlineColor(sf::Color(128,128,128));
        // cell.setOutlineThickness(1.0f);

        for (int i = 0; i < NUM_CELLE_X; i++) {
            for (int j = 0; j < NUM_CELLE_Y; j++) {
                cell.setPosition(i * scaleX, j * scaleY);
                cell.setSize(sf::Vector2f(scaleX, scaleY));
                
                if (grid[i][j].type == Terrain::Standard) {
                    cell.setFillColor(sf::Color::White);
                }
                gridTexture.draw(cell);
            }
        }
        
        gridTexture.display();
        gridTextureValid = true;
}

// Creates the boidShape for the agents
sf::ConvexShape createShape(float scaleX, float scaleY){
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

    return boidShape;
}

// Sets the shape of the agent to the right color and direction
void setAgentShapeParameters(sf::ConvexShape& boidShape, const unique_ptr<Agent> &agent, float scaleX, float scaleY){
    if (agent->speciesID == 1) {
            boidShape.setFillColor(sf::Color::Red);
            boidShape.setOutlineColor(sf::Color(139, 0, 0));
        } 
    else if (agent->speciesID == -1) {
        boidShape.setFillColor(sf::Color::Green);
        boidShape.setOutlineColor(sf::Color(0, 100, 0));

    }

    float pixelX = agent->x * scaleX;
    float pixelY = agent->y * scaleY; 

    boidShape.setPosition(pixelX, pixelY);
    
    // Rotation based on velocity
    if (agent->vx != 0 || agent->vy != 0) {
        float angleRadians = std::atan2(agent->vy, agent->vx);
        float angleDegrees = angleRadians * 180.0f / M_PI;
        
        boidShape.setRotation(angleDegrees);
    }
}

// Draws the cone of vision for each agent
void drawFOV(sf::RenderWindow& window, const sf::Vector2u& windowSize, const unique_ptr<Agent>& agent, float scaleX, float scaleY){
    float fovRadius = agent->viewRadius * scaleX;
    float fovAngle = agent->fovAngle * (M_PI / 180.0f);
    int triangleCount = 20; // Resolution of the arc

    float heading = 0.0f;
    if (agent->vx != 0 || agent->vy != 0) {
        heading = std::atan2(agent->vy, agent->vx);
    }

    // Size = Center + (Points on arc)
    sf::VertexArray fovShape(sf::TriangleFan, triangleCount + 1);

    // Center Vertex (Agent Position)
    float px = agent->x * scaleX;
    float py = agent->y * scaleY;
    fovShape[0].position = sf::Vector2f(px, py);

    // Set Color based on species (with transparency)
    sf::Color fovColor;
    if (agent->speciesID == 1) fovColor = sf::Color(255, 0, 0, 30); // Faint Red
    else fovColor = sf::Color(0, 255, 0, 30); // Faint Green

    fovShape[0].color = fovColor;

    // 4. Calculate Arc Vertices
    float startAngle = heading - (fovAngle / 2.0f);
    float angleStep = fovAngle / (float)(triangleCount - 1);

    for (int i = 0; i < triangleCount; ++i) {
        float currentAngle = startAngle + (angleStep * i);
        
        // Polar coordinates to Cartesian: x = r * cos(theta), y = r * sin(theta)
        float vx = px + cos(currentAngle) * fovRadius;
        float vy = py + sin(currentAngle) * fovRadius;

        fovShape[i + 1].position = sf::Vector2f(vx, vy);
        fovShape[i + 1].color = fovColor;
    }

    // We use sqrt because rangeOfVision is considered as squared distance
    float proximityRadius = std::sqrt(agent->rangeOfVision) * scaleX;
    sf::CircleShape proximityShape(proximityRadius);
    
    // Important: Set Origin to center so it draws around the agent, not from top-left
    proximityShape.setOrigin(proximityRadius, proximityRadius);
    proximityShape.setPosition(px, py);
    proximityShape.setFillColor(fovColor);

    // DRAWING WITH PAC-MAN LOGIC ---

    // Helper lists for offsets
    std::vector<float> xOffsets = {0.0f};
    std::vector<float> yOffsets = {0.0f};

    float winW = (float)windowSize.x;
    float winH = (float)windowSize.y;
    float fovPixelRadius = max(fovRadius, proximityRadius);

    // Check X-Axis Wrapping
    if (px < fovPixelRadius) {
        xOffsets.push_back(winW);
    }
    else if (px > winW - fovPixelRadius) {
        xOffsets.push_back(-winW);
    }

    // Check Y-Axis Wrapping
    // If I am close to the TOP edge, draw a ghost on the BOTTOM
    if (py < fovPixelRadius) {
        yOffsets.push_back(winH);
    }
    // If I am close to the BOTTOM edge, draw a ghost on the TOP
    else if (py > winH - fovPixelRadius) {
        yOffsets.push_back(-winH);
    }

    // Draw the shape for every required offset (Original + Ghosts)
    for (float ox : xOffsets) {
        for (float oy : yOffsets) {
            
            // Use RenderStates to apply the offset efficiently
            sf::RenderStates states = sf::RenderStates::Default;
            states.transform.translate(ox, oy);
            
            window.draw(fovShape, states);
            window.draw(proximityShape, states);
        }
    }
}

// Draws World with all cells and entities in a window
void World::draw(sf::RenderWindow& window){
    sf::Vector2u windowSize = window.getSize();

    float scaleX = windowSize.x / (float)NUM_CELLE_X;
    float scaleY = windowSize.y / (float)NUM_CELLE_Y;

    if (!gridTextureValid || gridTexture.getSize() != windowSize) {
        remapBackground(windowSize, scaleX, scaleY);
    }
    // Draws the background
    sf::Sprite backgroundSprite(gridTexture.getTexture());
    window.draw(backgroundSprite);

    // Draw the agents
    sf::ConvexShape boidShape = createShape(scaleX, scaleY);
    
    for (const auto& agent : agents) {
        setAgentShapeParameters(boidShape, agent, scaleX, scaleY);

        drawFOV(window, windowSize, agent, scaleX, scaleY);
        window.draw(boidShape);
    }
}