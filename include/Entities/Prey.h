#pragma once
#include <memory>
#include "Entities/Agent.h"
#include "Core/Config.h"


class Prey: public Agent{
    private:
    // Internal helpers
    float castFoodRay(float angle, float dist, const vector<vector<Cell>>& grid);
    // float getFoodAt(int x, int y, const vector<vector<Cell>>& grid);

    public:
    Prey(float x, float y);

    void updateEnergy(float thrust, float turn, float dt) override;
    unique_ptr<Agent> reproduce() override;

    float getFitness() const override;
};