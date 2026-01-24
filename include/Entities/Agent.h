#pragma once

// Forward declaration of the world class
class World;

class Agent{
    private:

    protected:
    // Variabili fisiche
    float ax, ay;
    float force;
    float max_speed;
    float friction;

    public:
    bool isAlive;
    float x, y;
    float vx, vy;
    
    Agent(float startX, float startY);
    virtual ~Agent() = default;
};