#pragma once

#include <random>

class Random {
private:
    static std::mt19937 generator;

public:
    static void Init() {
        generator.seed(std::random_device()());
    }

    static float Range(float min, float max) {
        std::uniform_real_distribution<float> distribution(min, max);
        return distribution(generator);
    }
};