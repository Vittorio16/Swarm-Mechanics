#pragma once
#include "Entities/Agent.h"

const float KILL_RANGE_SQ = 25.0f;

class Predator : public Agent{
    private:
    // Stores a pointer to the closest prey, if aplicable

    public:
    Predator(float startX, float startY);

    void updateEnergy(float speed, float ax, float ay, float dt) override;
};