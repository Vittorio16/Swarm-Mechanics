#pragma once
#include <vector>
#include "Core/Config.h"

using namespace std;

class SimplePerceptron{
    private:
    // Sizes of layers
    int inputNodes;
    int hiddenNodes;
    int outputNodes;

    // Vectors of weights between layers, and biases
    vector<float> w01;
    vector<float> w12;
    vector<float> b0;
    vector<float> b1;

    public:

    // Constructor and copy constructor
    SimplePerceptron();
    SimplePerceptron(const SimplePerceptron& oldObj) = default;

    // Given sensory inputs, returns ax and ay between -1 and 1
    vector<float> feedForward(const vector<float>& inputs);
    
    // Helper function to mutate brain of newborns
    void mutate(float currentMutationRate = STARTING_MUTATION_RATE, float currentMutationStrength = STARTING_MUTATION_STRENGTH);
    // Helpers to set weights at the start of the simulation
    vector<float> getWeights() const;
    void setWeights(const vector<float>& newWeights);
};