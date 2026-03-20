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

// Saves a vector of weights to a file, space-separated
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

// Loads weights from a file into a vector
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

// Saves the Hall of Fame (vector of weight vectors) to a file, one agent per line
void saveHallOfFameToFile(const string& filename, const vector<vector<float>>& hof) {
    ofstream file(filename);
    if (!file.is_open()) return;
    
    // Each agent gets its own line
    for (const auto& weights : hof) {
        for (float w : weights) {
            file << w << " ";
        }
        file << "\n"; 
    }
    file.close();
}

#include <sstream>

// Loads the Hall of Fame from a file, reconstructing the vector of weight vectors
vector<vector<float>> loadHallOfFameFromFile(const string& filename){
    vector<vector<float>> hof;
    ifstream file(filename);
    if (!file.is_open()) return hof;

    string line;
    // Read the file line by line
    while (getline(file, line)) {
        vector<float> weights;
        stringstream ss(line);
        float w;
        
        // Extract floats from the current line
        while (ss >> w) {
            weights.push_back(w);
        }
        
        // If we successfully read weights on this line, add it to the HoF
        if (!weights.empty()) {
            hof.push_back(weights);
        }
    }
    file.close();
    return hof;
}