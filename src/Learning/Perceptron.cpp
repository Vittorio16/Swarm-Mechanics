#include <cmath>
#include <random>
#include "Learning/Perceptron.h"
#include "Core/GlobalHelpers.h"

// Passes the value given in an activation function
float activation(float a){
    return tanh(a);
}

// Constructor
SimplePerceptron::SimplePerceptron() : 
    inputNodes(INPUT_LAYER_SIZE), hiddenNodes(HIDDEN_LAYER_SIZE), outputNodes(OUTPUT_LAYER_SIZE) {
        w01.resize(INPUT_LAYER_SIZE * HIDDEN_LAYER_SIZE);
        for (float& w : w01) w = randomFloat();

        w12.resize(HIDDEN_LAYER_SIZE * OUTPUT_LAYER_SIZE);
        for (float& w : w12) w = randomFloat();

        b0.resize(HIDDEN_LAYER_SIZE);
        for (float& b : b0) b = randomFloat();
        b1.resize(OUTPUT_LAYER_SIZE);
        for (float& b : b1) b = randomFloat();
    }

// Calculates the ouputs of the network based on the given sensory inputs
vector<float> SimplePerceptron::feedForward(const vector<float>& inputs){
    vector<float> hiddenValues(hiddenNodes);

    // Updates the neurons in the hidden node
    for (int i = 0; i < hiddenNodes; i++){
        float sum = 0.0f;

        for (int j = 0; j < inputNodes; j++){
            sum += inputs[j] * w01[j * hiddenNodes + i];
        }
        sum += b0[i];
        hiddenValues[i] = activation(sum);
    }
    
    // Updates the neurons of the output layer
    vector<float> outputValues(outputNodes);
    
    for (int i = 0; i < outputNodes; i++){
        float sum = 0.0f;
        
        for (int j = 0; j < hiddenNodes; j++){
            sum += hiddenValues[j] * w12[j * outputNodes + i];
        }
        sum += b1[i];
        outputValues[i] = activation(sum);
    }
    return outputValues;
}

// Mutates randomly some weights of the network
void SimplePerceptron::mutate(){
    // Helper lambda to mutate the weights
   auto mutateVector = [&](vector<float>& weights){
        for (float& w : weights){
            if (randomFloat() < MUTATION_RATE){
                float change = randomFloat() * MUTATION_STRENGTH * 2 - MUTATION_STRENGTH;

                w += change;
                if (w > 1.0f) w = 1.0f;
                if (w < -1.0f) w = -1.0f;
            }
        }
    };

    mutateVector(w01);
    mutateVector(w12);
    mutateVector(b0);
    mutateVector(b1);
}