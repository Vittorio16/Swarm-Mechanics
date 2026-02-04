#pragma once
#include <memory>
#include "Entities/Agent.h"

const float KILL_RANGE_SQ = 9.0f;

class Predator : public Agent{
    private:
    // Stores a pointer to the closest prey, if aplicable

    public:
    Predator(float startX, float startY);

    void updateEnergy(float ax, float ay, float dt) override;
    unique_ptr<Agent> reproduce() override;
};