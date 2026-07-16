#pragma once
#include <vector>

using namespace std;

struct GraveyardData {
    int max_capacity;
    int current_count;

    vector<int> speciesID;
    vector<float> fitness;

    vector<float> w01;
    vector<float> w12;
    vector<float> b0;
    vector<float> b1;

    GraveyardData(int capacity) : 
        max_capacity(capacity), 
        current_count(0),
        speciesID(capacity, 0),
        fitness(capacity, 0.0f),
        w01(capacity * W01_SIZE, 0.0f),
        w12(capacity * W12_SIZE, 0.0f),
        b0(capacity * B0_SIZE, 0.0f),
        b1(capacity * B1_SIZE, 0.0f) {}

    void clear() {
        current_count = 0;
    }
};