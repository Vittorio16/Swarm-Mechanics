#pragma once
#include <vector>

using namespace std;

const int INPUT_LAYER_SIZE = 4;
const int HIDDEN_LAYER_SIZE = 4;
const int OUTPUT_LAYER_SIZE = 2;

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

    SimplePerceptron();
    // Given sensory inputs, returns ax and ay between -1 and 1
    vector<float> feedForward(const vector<float>& inputs);
};