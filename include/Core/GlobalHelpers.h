#pragma once
#include <random>
#include <vector>
#include <string>

using namespace std;

float randomFloat(float min = -1.0f, float max = 1.0f);

void saveWeightsToFile(const string& filename, const vector<float>& weights);
vector<float> loadWeightsFromFile(const string& filename);

void saveHallOfFameToFile(const string& filename, const vector<vector<float>>& hof);
vector<vector<float>> loadHallOfFameFromFile(const string& filename);