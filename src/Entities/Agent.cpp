#include "Entities/Agent.h"

// Constructor
Agent::Agent(float startX, float startY) : 
        x(startX), y(startY), vx(0), vy(0),
        ax(0), ay(0), friction(10.0f), isAlive(true) {}