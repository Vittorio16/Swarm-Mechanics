#pragma once
#include <cstdint>
#include <string>

// Everything settable without a rebuild.
struct RunConfig {
    int numPrey = -1;
    int numPredators = -1;
    int numWorlds = -1;
    int gridMultiplier = 8;

    float grassRadius = -1.0f;
    float predViewRadius = -1.0f;
    float genDuration = -1.0f;

    uint64_t seed = 0;
    int maxGenerations = 0;
    uint64_t maxTicks = 0; 

    bool emitJson = false;  // machine-readable stdout for the harness
    std::string tag;
};