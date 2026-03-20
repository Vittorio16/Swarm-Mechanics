#include <random>
#include <cmath>
#include <fstream>
#include <iostream>
#include "Core/GlobalHelpers.h"

float randomFloat(float min, float max) {
    thread_local static random_device rd;
    thread_local static mt19937 gen(rd());
    
    uniform_real_distribution<float> dis(min, max);
    return dis(gen);
}

void saveWeightsToFile(const string& filename, const vector<float>& weights) {
    ofstream outFile(filename);
    if (!outFile.is_open()) {
        cerr << "Error opening file for writing: " << filename << endl;
        return;
    }
    for (float w : weights) {
        outFile << w << " ";
    }
    outFile.close();
}

vector<float> loadWeightsFromFile(const string& filename) {
    ifstream inFile(filename);
    vector<float> weights;
 
    if (!inFile.is_open()) {
        cerr << "Error opening file for reading: " << filename << endl;
        return weights;
    }
 
    float w;
    while (inFile >> w) {
        weights.push_back(w);
    }
 
    inFile.close();
    return weights;
}