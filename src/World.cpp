#include <cmath>
#include <algorithm>
#include <chrono>
#include "World.h"
#include "Core/Physics.h"
using namespace std;

#include <iostream>

// Constructor
World::World() : grid(NUM_CELLE_X, vector<Cell>(NUM_CELLE_Y)), 
                lattice_x_cells((int)ceil((float)NUM_CELLE_X / LATTICE_CELL_WIDTH)), lattice_y_cells((int)ceil((float)NUM_CELLE_Y / LATTICE_CELL_HEIGHT)),
                spatial_lattice(lattice_x_cells * lattice_y_cells),
                food_lattice_x_cells((int)ceil((float)NUM_CELLE_X / FOOD_CELL_WIDTH)), food_lattice_y_cells((int)ceil((float)NUM_CELLE_Y / FOOD_CELL_HEIGHT)),
                food_lattice(food_lattice_x_cells * food_lattice_y_cells),
                gen(random_device{}()), disX(0, NUM_CELLE_X - 1), disY(0, NUM_CELLE_Y - 1) {

    for (int i = 0; i < CONSTANT_FOOD_AMOUNT; i++) {
        addFood(disX(gen), disY(gen), MAX_FOOD); 
    }
    
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

// Helper to return to a prey the food scent (x, y) of the best food cell in its vision range
vector<float> World::getBestFoodScent(const Agent* agent) {
    int cx = (int)(agent->x / FOOD_CELL_WIDTH);
    int cy = (int)(agent->y / FOOD_CELL_HEIGHT);
    
    int max_distance = ceil(agent->grassViewRadius / FOOD_CELL_WIDTH);
    float bestFoodScore = -1.0f;
    int bestChunkIndex = -1;

    // Loop through nearby chunks in the food lattice
    for (int i = -max_distance; i <= max_distance; i++) {
        for (int j = -max_distance; j <= max_distance; j++) {
            int tx = ((cx + i) % food_lattice_x_cells + food_lattice_x_cells) % food_lattice_x_cells;
            int ty = ((cy + j) % food_lattice_y_cells + food_lattice_y_cells) % food_lattice_y_cells;

            int chunk_index = ty * food_lattice_x_cells + tx;   
            FoodChunk& chunk = food_lattice[chunk_index];

            if (chunk.totalFood > 0.001f){
                float centerX = chunk.sumFoodX / chunk.totalFood;
                float centerY = chunk.sumFoodY / chunk.totalFood;

                // Calculate distance from agent to this chunk's center of mass
                float dx = centerX - agent->x;
                float dy = centerY - agent->y;

                // Handle wrapping for distance calculation
                if (dx > NUM_CELLE_X * 0.5f) dx -= NUM_CELLE_X;
                if (dx < -NUM_CELLE_X * 0.5f) dx += NUM_CELLE_X;

                if (dy > NUM_CELLE_Y * 0.5f) dy -= NUM_CELLE_Y;
                if (dy < -NUM_CELLE_Y * 0.5f) dy += NUM_CELLE_Y;

                float distSq = dx*dx + dy*dy;
                if (distSq < 0.1f) distSq = 0.1f;

                if (distSq < agent->grassViewRadius * agent->grassViewRadius){
                    float foodScore = chunk.totalFood*chunk.totalFood / distSq;

                    if (foodScore > bestFoodScore) {
                        bestFoodScore = foodScore;
                        bestChunkIndex = chunk_index;
                    }
                }
            }
        }
    }
    // If no food found, return (0, 0)
    if (bestChunkIndex == -1) return {0.0f, 0.0f};

    // Now loop inside the best chunk to find the best cell, to improve precision when close to food
    float bestCellFoodScore = -1.0f;
    float bestCellX = 0.0f;
    float bestCellY = 0.0f;

    for (const sf::Vector2i& pos : food_lattice[bestChunkIndex].activeCells){
        float food = grid[pos.x][pos.y].foodAmount;

        // Skips if food eaten this frame, or if too small for bite size
        if (food <= 0.001f) continue;

        float dx = pos.x + 0.5f - agent->x;
        float dy = pos.y + 0.5f - agent->y;

        // Handle wrapping for distance calculation
        if (dx > NUM_CELLE_X * 0.5f) dx -= NUM_CELLE_X;
        if (dx < -NUM_CELLE_X * 0.5f) dx += NUM_CELLE_X;

        if (dy > NUM_CELLE_Y * 0.5f) dy -= NUM_CELLE_Y;
        if (dy < -NUM_CELLE_Y * 0.5f) dy += NUM_CELLE_Y;

        float distSq = dx*dx + dy*dy;
        if (distSq < 0.1f) distSq = 0.1f; 

        float foodScore = food*food / distSq;

        if (foodScore > bestCellFoodScore){
            bestCellFoodScore = foodScore;
            bestCellX = dx;
            bestCellY = dy;
        }
    }
    if (bestCellFoodScore < 0) return {0.0f, 0.0f};

    // Rotate to agent's coordinates
    float heading = agent->facingAngle;
    float c = cos(-heading);
    float s = sin(-heading);

    float localX = bestCellX * c - bestCellY * s;
    float localY = bestCellX * s + bestCellY * c;

    return {localX, localY};
}

// Helper to check if prey eats grass
void World::checkPreyFeeding(unique_ptr<Agent>& agent){
    if (agent->remainingDigestion > 0.001f) return;

    float posX = agent->x;
    float posY = agent->y;
    if (isnan(posX) || isinf(posX)) posX = 0.0f;
    if (isnan(posY) || isinf(posY)) posY = 0.0f;
    
    int centerIndexX = (int)posX;
    int centerIndexY = (int)posY;

    // Check the 3x3 grid around the agent
    for (int dx = -1; dx <= 1; dx++) {
        for (int dy = -1; dy <= 1; dy++) {
            
            // Calculate Neighbor Coordinates (with wrapping)
            int tx = centerIndexX + dx;
            int ty = centerIndexY + dy;
            
            tx = ((tx % NUM_CELLE_X) + NUM_CELLE_X) % NUM_CELLE_X;
            ty = ((ty % NUM_CELLE_Y) + NUM_CELLE_Y) % NUM_CELLE_Y;

            // Skip empty cells
            if (grid[tx][ty].foodAmount <= 0.001f) continue;

            // Calculate Distance to the centre of that target cell
            float fracX = agent->x - (int)agent->x; 
            float fracY = agent->y - (int)agent->y;
            
            float vecX = dx - fracX + 0.5f;
            float vecY = dy - fracY + 0.5f;

            float distSq = vecX*vecX + vecY*vecY;

            // Prey begins digesting and gains energy based on the food eaten
            if (distSq < PREY_EAT_RADIUS_SQ) {
                float foodEaten = grid[tx][ty].foodAmount;
                
                agent->remainingDigestion = agent->digestionTime;
                agent->energyGained += foodEaten;
                agent->energy += foodEaten; 
                
                // Caps the energy to prevent extreme values - could be removed in the future
                if (agent->energy > 3 * MAX_ENERGY / 2) {
                    agent->energy = 3 * MAX_ENERGY / 2;
                }

                grid[tx][ty].foodAmount = 0;

                int newX, newY;
                // Ensure we don't spawn it on an already full cell
                do {
                    newX = disX(gen);
                    newY = disY(gen);
                } while (grid[newX][newY].foodAmount > 0.0f);

                addFood(newX, newY, MAX_FOOD);

                // Update the food lattice
                int cx = tx / FOOD_CELL_WIDTH;
                int cy = ty / FOOD_CELL_HEIGHT;
                int chunk_index = cy * food_lattice_x_cells + cx;

                food_lattice[chunk_index].totalFood -= foodEaten; 
                food_lattice[chunk_index].sumFoodX -= tx * foodEaten;
                food_lattice[chunk_index].sumFoodY -= ty * foodEaten;

                if (food_lattice[chunk_index].totalFood <= 0.001f) {
                    food_lattice[chunk_index].totalFood = 0.0f;
                    food_lattice[chunk_index].sumFoodX = 0.0f;
                    food_lattice[chunk_index].sumFoodY = 0.0f;
                }

                return;
            }
        }
    }
}

// Adds food to a cell
void World::addFood(int cx, int cy, float growthAmount){
    float current_food = grid[cx][cy].foodAmount;
    float space_left = MAX_FOOD - current_food;
    
    if (space_left <= 0) return;

    float actual_growth = min(growthAmount, space_left);

    int chunk_x = cx / FOOD_CELL_WIDTH;
    int chunk_y = cy / FOOD_CELL_HEIGHT;
    int chunk_index = chunk_y * food_lattice_x_cells + chunk_x;

    // If it was completely empty, register it in the chunk's active list
    if (current_food <= 0.001f){
        food_lattice[chunk_index].activeCells.push_back(sf::Vector2i(cx, cy));
    }

    // Update main grid and chunk scoreboard
    grid[cx][cy].foodAmount += actual_growth;
    food_lattice[chunk_index].totalFood += actual_growth;
    food_lattice[chunk_index].sumFoodX += cx * actual_growth;
    food_lattice[chunk_index].sumFoodY += cy * actual_growth;
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
        
        bx = (bx % lattice_x_cells + lattice_x_cells) % lattice_x_cells;
        by = (by % lattice_y_cells + lattice_y_cells) % lattice_y_cells;

        spatial_lattice[by * lattice_x_cells + bx].push_back(agent.get());
    }
}


// Updates the world each tick of the simulation
void World::update(float dt, int generationCount){
    // Profiling of needed time
    using namespace std::chrono;
    // Place each agent in the correct bucket in the spatial lattice
    auto start = high_resolution_clock::now();
    auto start_buckets = high_resolution_clock::now();

    update_buckets();
    
    auto end_buckets = high_resolution_clock::now();

    auto start_obs = high_resolution_clock::now();
    auto start_think = high_resolution_clock::now();

    double total_obs_time = 0.0;
    double size_agents = 0.0;

    // First it checks the system state and lets agents think
    for (auto& agent : agents){
        if (!agent->isAlive) continue;
        size_agents++;

        auto s_obs = high_resolution_clock::now();

        vector<Observation> agentsInFOV = getObservation(agent.get());

        vector<float> scents = {0.0f, 0.0f, 0.0f};

        Agent* a = static_cast<Agent*>(agent.get());
        scents = getBestFoodScent(a);

        agent->updateSensoryData(agentsInFOV, scents);

        auto e_obs = high_resolution_clock::now();
        total_obs_time += duration_cast<nanoseconds>(e_obs - s_obs).count();
        
        agent->think();
    }
    auto end_think_obs = high_resolution_clock::now();
    auto start_move = high_resolution_clock::now();
    
    vector<unique_ptr<Agent>> nursery;

    // Then updates everything at the same time
    for (auto& agent : agents){
        if (!agent->isAlive) continue;
        // May want to separate movement from feeding
        agent->move(dt);

        // pacman style world
        if (isnan(agent->x) || isinf(agent->x)) agent->x = 0.0f;
        agent->x = fmod(agent->x, NUM_CELLE_X);
        if(agent->x < 0) agent->x += NUM_CELLE_X;

        if (isnan(agent->y) || isinf(agent->y)) agent->y = 0.0f;
        agent->y = fmod(agent->y, NUM_CELLE_Y);
        if(agent->y < 0) agent->y += NUM_CELLE_Y;

        // Handles eating grass for prey
        if (agent->speciesID == PREY_ID){
            checkPreyFeeding(agent);
        }
        
        // Handles reproduction 
        if (agent->energy > MAX_ENERGY){
            nursery.push_back(agent->reproduce());
        }
    }
    
    auto end_move = high_resolution_clock::now();
    auto start_cleanup = high_resolution_clock::now();

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
        if ((*it)->getFitness() > MINIMUM_FITNESS_TO_BE_SAVED) { 
            graveyard.push_back(std::move(*it));
        }
    }
    agents.erase(firstDead, agents.end());

    
    // Remove element from activeGrass if its food got eaten
    for (auto& chunk : food_lattice){ 
        chunk.activeCells.erase(
            remove_if(chunk.activeCells.begin(), chunk.activeCells.end(), 
            [&](const sf::Vector2i& pos) {
                return grid[pos.x][pos.y].foodAmount <= 0.001f;}),
                chunk.activeCells.end()
            );
    }
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
}

// Given an observer, returns a vector of pointers to all the agents it can see
vector<Observation> World::getObservation(const Agent* observer){
    vector<Observation> observations;

    float observerHeading = observer->facingAngle;
    
    int bx = (int)(observer->x / LATTICE_CELL_WIDTH);
    int by = (int)(observer->y / LATTICE_CELL_HEIGHT);
    
    bx = (bx % lattice_x_cells + lattice_x_cells) % lattice_x_cells;
    by = (by % lattice_y_cells + lattice_y_cells) % lattice_y_cells;
    
    int max_x_distance = (int)ceil((float)observer->viewRadius / LATTICE_CELL_WIDTH);
    int max_y_distance = (int)ceil((float)observer->viewRadius / LATTICE_CELL_HEIGHT);
    
    for (int i = - max_x_distance; i <= max_x_distance; i++){
        for (int j = -max_y_distance; j <= max_y_distance; j++){
            int temp_bx = ((bx + i) % lattice_x_cells + lattice_x_cells) % lattice_x_cells;
            int temp_by = ((by + j) % lattice_y_cells + lattice_y_cells) % lattice_y_cells;

            for (Agent* otherAgent : spatial_lattice[temp_by * lattice_x_cells + temp_bx]){
        
                if (otherAgent == observer || !otherAgent->isAlive) continue;
        
                ThoroidalData coords = getThoroidalCoordinates(
                    observer->x, observer->y, otherAgent->x, otherAgent->y, NUM_CELLE_X, NUM_CELLE_Y
                );
        
                float dist = coords.dist;
                float distSq = coords.distSq;
                float angleToTarget = coords.angleToTarget;
                float dx = coords.dx;
                float dy = coords.dy;
        
                // Senses an area around the agent
                if (distSq < observer->sensingRange){
                    observations.emplace_back(dx, dy, dist, distSq, otherAgent);
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
                    observations.emplace_back(dx, dy, dist, distSq, otherAgent);
                }
            }
        }
    }
    return observations;
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

    for (int i = 0; i < food_lattice_x_cells; i++) {
        for (int j = 0; j < food_lattice_y_cells; j++) {
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
        
        sf::RectangleShape cell;
        cell.setOutlineColor(sf::Color(128,128,128));
        
        if (SHOW_GRID) cell.setOutlineThickness(1.0f);

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
        
        // Draw food lattice for debugging purposes
        if (SHOW_FOOD_LATTICE) drawFoodLattice(scaleX, scaleY);

        gridTexture.display();
        gridTextureValid = true;
}

// Draws the layer of grass
sf::VertexArray drawGrass(const vector<vector<Cell>>& grid, const vector<FoodChunk>& food_lattice, float scaleX, float scaleY){
    sf::VertexArray grassLayer(sf::Quads);

    for (int chunkIndex = 0; chunkIndex < food_lattice.size(); chunkIndex++){
        const FoodChunk& chunk = food_lattice[chunkIndex];
        if (chunk.totalFood <= 0) continue;

        for (const auto& pos : chunk.activeCells){
            int i = pos.x;
            int j = pos.y;

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
    if (agent->speciesID == PREDATOR_ID) {
            boidShape.setFillColor(sf::Color::Red);
            boidShape.setOutlineColor(sf::Color(139, 0, 0));
        } 
    else if (agent->speciesID == PREY_ID) {
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
    if (agent->speciesID == PREDATOR_ID) fovColor = sf::Color(255, 0, 0, 30); // Faint Red
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
    float proximityRadius = std::sqrt(agent->sensingRange) * scaleX;
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
    sf::VertexArray grassLayer = drawGrass(grid, food_lattice, scaleX, scaleY);
    window.draw(grassLayer);

    // Draw the agents
    sf::ConvexShape boidShape = createShape(scaleX, scaleY);
    
    for (const auto& agent : agents) {
        setAgentShapeParameters(boidShape, agent, scaleX, scaleY);

        if (SHOW_FOV) drawFOV(window, windowSize, agent, scaleX, scaleY);
        
        window.draw(boidShape);
    }
}