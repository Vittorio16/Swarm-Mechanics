#pragma once
#include <memory>
#include "Entities/Agent.h"
#include "Core/Config.h"
class Prey: public Agent{
    private:
    // Internal helpers
    float castFoodRay(float angle, float dist, const vector<vector<Cell>>& grid);
    float getFoodAt(float x, float y, const vector<vector<Cell>>& grid);

    public:
    Prey(float x, float y);

    vector<float> senseFood(const vector<vector<Cell>>& grid);

    void updateEnergy(float speed, float ax, float ay, float dt) override;
    unique_ptr<Agent> reproduce() override;

    void updateSensoryData(const vector<Observation>& observations, const vector<float>& scents) override;
};