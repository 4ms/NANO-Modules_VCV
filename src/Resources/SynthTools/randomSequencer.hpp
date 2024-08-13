#ifndef RANDOM_SEQUENCER_HPP
#define RANDOM_SEQUENCER_HPP

#include "rack.hpp" // Make sure to include VCV Rack's main header
#include "clock.hpp"
#include <array>
#include <random>

class RandomSequencer {
public:
    RandomSequencer() : currentStep(0) {
        // Initialize the step array with random values using VCV Rack's utilities
        for (auto& step : steps) {
            step = random::uniform() > 0.5f; // Generates a random float between 0.0 and 1.0
        }
    }

    void update(float deltaTime) {
        clock.update(deltaTime);

        // Check if the clock has completed a cycle
        if (clock.hasCompletedCycle()) {
            currentStep = (currentStep + 1) % steps.size();
            // Here you can trigger an event if steps[currentStep] is true
        }
    }

    void setClockFrequency(float frequency) {
        clock.setFrequency(frequency);
    }

    bool getCurrentStepValue() const {
        return steps[currentStep];
    }

private:
    Clock clock;
    std::array<bool, 64> steps; // Array of 64 steps
    size_t currentStep; // Index of the current step
};

#endif // RANDOM_SEQUENCER_HPP