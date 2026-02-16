#include <cmath>
#include <algorithm>
#include "World.h"
#include "Entities/Predator.h"
#include "Entities/Prey.h"
#include "Core/Physics.h"
using namespace std;

#include <iostream>

// Constructor
World::World() : grid(NUM_CELLE_X, vector<Cell>(NUM_CELLE_Y)), 
                lattice_width((int)ceil((float)NUM_CELLE_X / LATTICE_CELL_WIDTH)), lattice_height((int)ceil((float)NUM_CELLE_Y / LATTICE_CELL_HEIGHT)),
                spatial_lattice(lattice_width * lattice_height),
                gen(random_device{}()), disX(0, NUM_CELLE_X - 1), disY(0, NUM_CELLE_Y - 1) {

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

// Helper to check if prey eats grass
void World::checkPreyFeeding(unique_ptr<Agent>& agent){
    // Define Eat Range 
    float eatRadiusSq = 1.0f * 1.0f; 

    int centerIndexX = (int)agent->x;
    int centerIndexY = (int)agent->y;

    if (isnan(centerIndexX) || isinf(centerIndexX)) centerIndexX = 0.0f;
    if (isnan(centerIndexY) || isinf(centerIndexY)) centerIndexY = 0.0f;

    // Check the 3x3 grid around the agent
    // This ensures we can eat from a cell even if we drifted into its neighbor
    for (int dx = -1; dx <= 1; dx++) {
        for (int dy = -1; dy <= 1; dy++) {
            
            // ACalculate Neighbor Coordinates (with wrapping)
            int tx = centerIndexX + dx;
            int ty = centerIndexY + dy;
            
            tx = fmod(tx, NUM_CELLE_X);
            if (tx < 0) tx += NUM_CELLE_X;

            ty = fmod(ty, NUM_CELLE_Y);
            if (ty < 0) ty += NUM_CELLE_Y;

            // Skip empty cells
            if (grid[tx][ty].foodAmount <= 0) continue;

            // Calculate Distance to the CENTER of that target cell
            // Note: We use the relative (dx, dy) to calculate distance 
            // This handles the edge-of-world cases automatically.
            
            // Agent relative position in its current cell
            float fracX = agent->x - (int)agent->x; 
            float fracY = agent->y - (int)agent->y;
            
            // Vector from Agent to Target Cell Center
            // (Target Cell Offset) - (Agent Fractional Pos) + (Center Bias)
            float vecX = dx - fracX + 0.5f;
            float vecY = dy - fracY + 0.5f;

            float distSq = vecX*vecX + vecY*vecY;

            if (distSq < eatRadiusSq) {
                agent->energyGained += grid[tx][ty].foodAmount * 5.0f;
                
                agent->energy += grid[tx][ty].foodAmount * 5.0f; 
                grid[tx][ty].foodAmount = 0;
                
                
                return;
            }
        }
    }
}

// Grows the grass
void World::growGrass(float dt){
    float spawnChance = 0.005f;
    int growthAttempts = 40;
    float growthAmount = 1000.0f * dt;

    static std::uniform_real_distribution<float> chance(0.0f, 1.0f);

    for (int i = 0; i < growthAttempts; i++){
        if (chance(gen) < spawnChance){
            int cx = disX(gen);
            int cy = disY(gen);
    
            grid[cx][cy].foodAmount = grid[cx][cy].foodAmount + growthAmount > MAX_FOOD ?
                                        MAX_FOOD : grid[cx][cy].foodAmount + growthAmount;
        }
    }
}


// Resets all the buckets of the spatial lattice and updates their contents
void World::update_buckets(){
    for (auto& bucket : spatial_lattice){
        bucket.clear();
    }

    for (auto& agent : agents){
        if (!agent->isAlive) continue;

        int bx = (int)(agent->x / LATTICE_CELL_WIDTH);
        int by = (int)(agent->y / LATTICE_CELL_HEIGHT);
        
        bx = (bx % lattice_width + lattice_width) % lattice_width;
        by = (by % lattice_height + lattice_height) % lattice_height;

        spatial_lattice[by * lattice_width + bx].push_back(agent.get());
    }
}


// Updates the world each tick of the simulation
void World::update(float dt){
    // Place each agent in the correct bucket in the spatial lattice
    update_buckets();

    // First it checks the system state and lets agents decide
    for (auto& agent : agents){
        if (!agent->isAlive) continue;
        vector<Observation> agentsInFOV = getObservation(agent.get());

        // Update scents if the agent is a prey
        vector<float> scents = {0.0f, 0.0f, 0.0f};
        if (agent->speciesID == -1){
            Prey* p = static_cast<Prey*>(agent.get());
            scents = p->senseFood(this->grid);
        }

        agent->updateSensoryData(agentsInFOV, scents);
        agent->think();
    }

    vector<unique_ptr<Agent>> nursery;

    // Then updates everything at the same time
    for (auto& agent : agents){
        if (!agent->isAlive) continue;
        // May want to separate movement from feeding
        agent->move(dt);

        // Creates pacman style world
        if (isnan(agent->x) || isinf(agent->x)) agent->x = 0.0f;
        agent->x = fmod(agent->x, NUM_CELLE_X);
        if(agent->x < 0) agent->x += NUM_CELLE_X;

        if (isnan(agent->y) || isinf(agent->y)) agent->y = 0.0f;
        agent->y = fmod(agent->y, NUM_CELLE_Y);
        if(agent->y < 0) agent->y += NUM_CELLE_Y;

        // Handles eating grass for prey
        if (agent->speciesID == -1){
            checkPreyFeeding(agent);
        }
        
        // Handles reproduction 
        if (agent->energy > MAX_ENERGY){
            nursery.push_back(agent->reproduce());
        }
    }
    
    // Add newly born agents
    for (auto& baby : nursery){ 
        agents.push_back(std::move(baby));
    }
    // Erases dead agents - first moving them to graveyard for scoring purposes
    auto firstDead = std::partition(agents.begin(), agents.end(), 
        [](const std::unique_ptr<Agent>& a) {
            return a->isAlive; 
        });

    // Move the dead agents into the graveyard
    for (auto it = firstDead; it != agents.end(); ++it) {
        // Only save them if they actually did something useful
        if ((*it)->getFitness() > 5.0f) { 
            graveyard.push_back(std::move(*it));
        }
    }
    agents.erase(firstDead, agents.end());

    // Grass growth
    growGrass(dt);
}

// Given an observer, returns a vector of pointers to all the agents it can see
vector<Observation> World::getObservation(const Agent* observer){
    vector<Observation> observations;

    float observerHeading = observer->facingAngle;
    
    int bx = (int)(observer->x / LATTICE_CELL_WIDTH);
    int by = (int)(observer->y / LATTICE_CELL_HEIGHT);
    
    bx = (bx % lattice_width + lattice_width) % lattice_width;
    by = (by % lattice_height + lattice_height) % lattice_height;
    
    int max_x_distance = (int)ceil((float)observer->viewRadius / LATTICE_CELL_WIDTH);
    int max_y_distance = (int)ceil((float)observer->viewRadius / LATTICE_CELL_HEIGHT);
    
    for (int i = - max_x_distance; i <= max_x_distance; i++){
        for (int j = -max_y_distance; j <= max_y_distance; j++){
            int temp_bx = ((bx + i) % lattice_width + lattice_width) % lattice_width;
            int temp_by = ((by + j) % lattice_height + lattice_height) % lattice_height;

            for (Agent* otherAgent : spatial_lattice[temp_by * lattice_width + temp_bx]){
        
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
                if (isnan(angleDiff) || isinf(angleDiff)) angleDiff = 0.0f;
        
                angleDiff = fmod(angleDiff, 2*M_PI);
                if (angleDiff <= -M_PI) angleDiff += 2 * M_PI;
                if (angleDiff > M_PI) angleDiff -= 2 * M_PI;
        
                if (abs(angleDiff) < observer->fovAngle * M_PI / 360.0f){
                    observations.emplace_back(dx, dy, distSq, otherAgent);
                }
            }
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

// Draws the layer of grass
sf::VertexArray drawGrass(const vector<vector<Cell>>& grid, float scaleX, float scaleY){
    sf::VertexArray grassLayer(sf::Quads);

    for (int i = 0; i < NUM_CELLE_X; i++){
        for (int j = 0; j < NUM_CELLE_Y; j++){
            if (grid[i][j].foodAmount > 0){
                float x = scaleX * i;
                float y = scaleY * j;

                // Create a color based on how grown the grass is
                sf::Uint8 alpha = static_cast<sf::Uint8>((grid[i][j].foodAmount / MAX_FOOD) * 255);
                sf::Color grassColor(0, 200, 0, alpha); 

                // Define the 4 corners of the grass cell
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
    
    // Rotation based on facing angle
    float angleDegrees = agent->facingAngle * 180.0f / M_PI;
        boidShape.setRotation(angleDegrees);
}
// Draws the cone of vision for each agent
void drawFOV(sf::RenderWindow& window, const sf::Vector2u& windowSize, const unique_ptr<Agent>& agent, float scaleX, float scaleY){
    float fovRadius = agent->viewRadius * scaleX;
    float fovAngle = agent->fovAngle * (M_PI / 180.0f);
    int triangleCount = 20; // Resolution of the arc

    float heading = agent->facingAngle;

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

    // Draws the food for prey
    sf::VertexArray grassLayer = drawGrass(grid, scaleX, scaleY);
    window.draw(grassLayer);

    // Draw the agents
    sf::ConvexShape boidShape = createShape(scaleX, scaleY);
    
    for (const auto& agent : agents) {
        setAgentShapeParameters(boidShape, agent, scaleX, scaleY);

        //drawFOV(window, windowSize, agent, scaleX, scaleY);
        window.draw(boidShape);
    }
}