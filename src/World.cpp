#include <cmath>
#include <algorithm>
#include <chrono>
#include "World.h"
#include "Core/Physics.h"

#include "GPU/functions/DecisionSystem.h"
#include "GPU/functions/EnergySystem.h"
#include "GPU/functions/LatticeSystem.h"
#include "GPU/functions/FoodLatticeSystem.h"
#include "GPU/functions/LifeSystem.h"
#include "GPU/functions/PhysicSystem.h"
#include "GPU/functions/SensorySystem.h"

using namespace std;

#include <iostream>

// Constructor
World::World(int num_prey, int num_predators) : 
    swarm(MAX_CAPACITY),
    spatialLattice(NUM_CELLE_X / LATTICE_CELL_WIDTH, NUM_CELLE_Y / LATTICE_CELL_HEIGHT),
    foodLattice(NUM_CELLE_X / FOOD_CELL_WIDTH, NUM_CELLE_Y / FOOD_CELL_HEIGHT, NUM_CELLE_X, NUM_CELLE_Y) {

    LifeSystem::initSwarm(swarm, num_prey, num_predators);
    FoodLatticeSystem::grow(foodLattice, MAX_FOOD);
}

// Updates the world each tick of the simulation
ProfilingData World::update(float dt, int generationCount){
    // Profiling of needed time
    using namespace std::chrono;
    // Place each agent in the correct bucket in the spatial lattice
    auto start = high_resolution_clock::now();
    auto start_buckets = high_resolution_clock::now();

    // Each iteration it rebuilds the spatial lattice
    LatticeSystem::build(spatialLattice, swarm);
    
    auto end_buckets = high_resolution_clock::now();

    auto start_obs = high_resolution_clock::now();
    auto start_think = high_resolution_clock::now();

    double total_obs_time = 0.0;
    double size_agents = 0.0;

    // First it checks the system state and lets agents think
    auto s_obs = high_resolution_clock::now();

    // Gathers observations and updates the sensory data
    SensorySystem::update(swarm, spatialLattice, foodLattice);

    auto e_obs = high_resolution_clock::now();
    total_obs_time += duration_cast<nanoseconds>(e_obs - s_obs).count();
    
    // Feed forward neural network for decision making
    DecisionSystem::think(swarm);

    auto end_think_obs = high_resolution_clock::now();
    auto start_move = high_resolution_clock::now();

    // Then updates everything at the same time - movement, and handles energy gain and consumption
    PhysicsSystem::update(swarm, dt);
    EnergySystem::update(swarm, foodLattice, dt);

    auto end_move = high_resolution_clock::now();
    auto start_cleanup = high_resolution_clock::now();

    // Handles first deaths, then births to keep the array compact
    LifeSystem::handleDeaths(swarm);
    LifeSystem::handleBirths(swarm);
    
    // Handles grass growth
    FoodLatticeSystem::grow(foodLattice, MAX_FOOD);

    auto end_cleanup = high_resolution_clock::now();
    auto end = high_resolution_clock::now();
    // Profiling output
    double t_buckets = duration_cast<microseconds>(end_buckets - start_buckets).count();
    double t_obs = total_obs_time / 1000.0; 
    double t_think = duration_cast<microseconds>(end_think_obs - start_think).count() - t_obs;
    double t_move = duration_cast<microseconds>(end_move - start_move).count();
    double t_cleanup = duration_cast<microseconds>(end_cleanup - start_cleanup).count();
    double t_total = duration_cast<microseconds>(end - start).count();
    double check_t_total = t_buckets + t_obs + t_think + t_move + t_cleanup;

    return {t_buckets, t_obs, t_think, t_move, t_cleanup, t_total, check_t_total};
}

// Optionally draw chunk boundaries for debugging
void World::drawFoodLattice(float scaleX, float scaleY){
    sf::Font font;
    // Attempt to load the font. If it fails, we just skip drawing text so the game doesn't crash.
    bool hasFont = font.loadFromFile("Roboto-Regular.ttf"); 
    
    sf::Text coordText;
    if (hasFont) {
        coordText.setFont(font);
        coordText.setCharacterSize(10);
        coordText.setFillColor(sf::Color(0, 150, 0, 150)); // Faint green text
    }

    sf::RectangleShape chunkBox;
    chunkBox.setFillColor(sf::Color::Transparent);
    chunkBox.setOutlineColor(sf::Color(0, 100, 0));
    chunkBox.setOutlineThickness(1.0f); 

    // Calculate how many pixels wide/tall a single chunk is
    float chunkPixelWidth = FOOD_CELL_WIDTH * scaleX;
    float chunkPixelHeight = FOOD_CELL_HEIGHT * scaleY;

    for (int i = 0; i < foodLattice.num_cells_x; i++) {
        for (int j = 0; j < foodLattice.num_cells_y; j++) {
            float posX = i * chunkPixelWidth;
            float posY = j * chunkPixelHeight;

            chunkBox.setPosition(posX, posY);
            chunkBox.setSize(sf::Vector2f(chunkPixelWidth, chunkPixelHeight));
            
            gridTexture.draw(chunkBox);

            if (hasFont) {
                coordText.setString(std::to_string(i) + "," + std::to_string(j));
                // Offset by 2 pixels so it doesn't overlap the border
                coordText.setPosition(posX + 2, posY + 2); 
                gridTexture.draw(coordText);
            }
        }
    }
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
        
        if (SHOW_GRID) {
            sf::RectangleShape cell;
            cell.setFillColor(sf::Color::White);
            cell.setOutlineColor(sf::Color(128, 128, 128)); // Colore grigio scuro per la griglia
            cell.setOutlineThickness(1.0f);
            cell.setSize(sf::Vector2f(scaleX, scaleY));

            for (int i = 0; i < NUM_CELLE_X; i++) {
                for (int j = 0; j < NUM_CELLE_Y; j++) {
                    cell.setPosition(i * scaleX, j * scaleY);
                    gridTexture.draw(cell);
                }
            }
        }
        
        // Draw food lattice for debugging purposes
        if (SHOW_FOOD_LATTICE) drawFoodLattice(scaleX, scaleY);

        gridTexture.display();
        gridTextureValid = true;
}

// Draws the layer of grass
sf::VertexArray drawGrass(const FoodLatticeData& foodLattice, float scaleX, float scaleY){
    sf::VertexArray grassLayer(sf::Quads);

    for (int j = 0; j < foodLattice.num_cells_y; j++) {
        for (int i = 0; i < foodLattice.num_cells_x; i++) {
            int idx = j * foodLattice.num_cells_x + i;
            float foodAmount = foodLattice.foodGrid[idx];

            if (foodAmount > 0.001f) {
                float x = scaleX * i;
                float y = scaleY * j;

                sf::Uint8 alpha = static_cast<sf::Uint8>((foodAmount / MAX_FOOD) * 255);
                sf::Color grassColor(0, 200, 0, alpha); 

                grassLayer.append(sf::Vertex(sf::Vector2f(x, y), grassColor));
                grassLayer.append(sf::Vertex(sf::Vector2f(x + scaleX, y), grassColor));
                grassLayer.append(sf::Vertex(sf::Vector2f(x + scaleX, y + scaleY), grassColor));
                grassLayer.append(sf::Vertex(sf::Vector2f(x, y + scaleY), grassColor));
            }
        }
    }
    return grassLayer;
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

    boidShape.setOrigin(0.0f, 0.0f);
    return boidShape;
}


// Sets the shape of the agent to the right color and direction
void setAgentShapeParameters(sf::ConvexShape& boidShape, const SwarmData &swarm, int i, float scaleX, float scaleY){
    if (swarm.agentIdentifications.speciesID[i] == PREDATOR_ID) {
            boidShape.setFillColor(sf::Color::Red);
            boidShape.setOutlineColor(sf::Color(139, 0, 0));
        } 
    else if (swarm.agentIdentifications.speciesID[i] == PREY_ID) {
        boidShape.setFillColor(sf::Color::Green);
        boidShape.setOutlineColor(sf::Color(0, 100, 0));

    }

    float pixelX = swarm.physics.x[i] * scaleX;
    float pixelY = swarm.physics.y[i] * scaleY; 

    boidShape.setPosition(pixelX, pixelY);
    
    // Rotation based on facing angle
    float angleDegrees = swarm.physics.facingAngle[i] * 180.0f / M_PI;
        boidShape.setRotation(angleDegrees);
}

// Draws the cone of vision for each agent
void drawFOV(sf::RenderWindow& window, const sf::Vector2u& windowSize, const SwarmData& swarm, int i, float scaleX, float scaleY){
    float fovRadius = swarm.perceptions.viewRadius[i] * scaleX;
    float fovAngle = swarm.perceptions.fovAngle[i] * (M_PI / 180.0f);
    int triangleCount = 20; // Resolution of the arc

    float heading = swarm.physics.facingAngle[i];

    // Size = Center + (Points on arc)
    sf::VertexArray fovShape(sf::TriangleFan, triangleCount + 1);

    // Center Vertex (Agent Position)
    float px = swarm.physics.x[i] * scaleX;
    float py = swarm.physics.y[i] * scaleY;
    fovShape[0].position = sf::Vector2f(px, py);

    // Set Color based on species (with transparency)
    sf::Color fovColor;
    if (swarm.agentIdentifications.speciesID[i] == PREDATOR_ID) fovColor = sf::Color(255, 0, 0, 30); // Faint Red
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
    float proximityRadius = std::sqrt(swarm.perceptions.sensingRange[i]) * scaleX;
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

    // Draws the food for prey
    sf::VertexArray grassLayer = drawGrass(foodLattice, scaleX, scaleY);
    window.draw(grassLayer);

    // Draw the agents
    sf::ConvexShape boidShape = createShape(scaleX, scaleY);
    
    for (int i = 0; i < swarm.current_count; i++){
        setAgentShapeParameters(boidShape, swarm, i, scaleX, scaleY);

        if (SHOW_FOV) drawFOV(window, windowSize, swarm, i, scaleX, scaleY);
        
        window.draw(boidShape);
    }
}