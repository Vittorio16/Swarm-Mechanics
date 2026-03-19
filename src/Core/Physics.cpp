#include "Core/Physics.h"
#include <cmath>


#include "Core/Physics.h"
#include <cmath>
#include <vector> // Required for std::vector

using namespace std;

// Returns {distanceSquared, angle}
ThoroidalData getThoroidalCoordinates(float obsX, float obsY, float targetX, float targetY, int worldWidth, int worldHeight){

    float dx = targetX - obsX;
    float dy = targetY - obsY;

    // Thoroidal Wrapping (Pac-Man Logic)
    if (dx > worldWidth * 0.5f) {
        dx -= worldWidth;
    } 
    else if (dx < -worldWidth * 0.5f) {
        dx += worldWidth;
    }
    if (dy > worldHeight * 0.5f) {
        dy -= worldHeight;
    } 
    else if (dy < -worldHeight * 0.5f) {
        dy += worldHeight;
    }

    float distSq = dx*dx + dy*dy;
    float angleToTarget = atan2(dy, dx);
    
    return {distSq, angleToTarget, dx, dy};
}