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
    // Variabili di stato
    bool isAlive;
    float x, y;
    float vx, vy;
    
    // Input sensoriali che guidano decisione
    struct SensoryData {
        float closestPredatorX, closestPredatorY;
    };

    Agent(float startX, float startY);
    virtual ~Agent() = default;

    // Aggiorna decisioni basandosi su dati sensoriali
    void updateSensoryData();
    void think();

    // Aggiorna la posizione usando fisica
    void move(float dt);
};