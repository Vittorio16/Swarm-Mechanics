#pragma once
#include "Entities/Agent.h"

class Prey: public Agent{
    private:

    public:
    Prey(float x, float y);

    void updateEnergy(float speed, float ax, float ay, float dt) override;
};