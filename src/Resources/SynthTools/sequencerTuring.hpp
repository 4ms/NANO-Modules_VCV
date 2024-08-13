#ifndef RANDOM_SEQUENCER_HPP
#define RANDOM_SEQUENCER_HPP

#include "rack.hpp" // Make sure to include VCV Rack's main header
#include "clock.hpp"
#include <array>
#include <random>
#include "../uiTools/trigger.hpp"
#include "quantizer.hpp"

class RandomSequencer {
public:
    RandomSequencer() : currentStep(0) {
        // Initialize the step array with random values using VCV Rack's utilities
        for (int i = 0; i < 64; i++) {
            triggerSequence[i] = random::uniform() > 0.5f; // Generates a random float between 0.0 and 1.0
            voltageSequence[i] = random::uniform();
        }
    }

    void init() {
        seqClock.init();
        seqClock.setFrequency(1.0f);
        seqClock.setState(true);
        seqClock.setSampleRate(sampleRate);
        pulseTrigger.init();
        pulseTrigger.setSampleRate(sampleRate);
        pulseTrigger.setTriggerDurationMs(1.0f);
        mainQuantizer.setScale(Scale::CHROMATIC);
        slewQuantizer.setScale(Scale::CHROMATIC);
    }

    void update(void) {
        seqClock.update();

        if (seqClock.getInstantTrigger()) {
            currentStep = (currentStep + 1) % sequenceLength;

            float randomnessStep = random::uniform();

            // If randomAmount is not 0, potentially modify the current step values
            if (randomAmount > 0.0f) {
                if (randomnessStep < randomAmount) {
                    // With a probability equal to randomAmount, change the trigger
                    triggerSequence[currentStep] = randomnessStep < probability;
                }
                if (randomnessStep < randomAmount) {
                    // With a probability equal to randomAmount, change the voltage and adapt it to the thresholds
                    float scaledStep = minVoltage + (randomnessStep * (maxVoltage - minVoltage));
                    // Quantize the voltage step and store it
                    voltageSequence[currentStep] = scaledStep;
                }
            }
            // If randomAmount is 0, the sequence stays the same
        }

        updateSlewLimiter(deltaTime);

        if(seqClock.getInstantTrigger()){
           pulseTrigger.process(triggerSequence[currentStep]);
        } else {
           pulseTrigger.process(0.0f);
        }
    }

    void setSampleRate(float newSampleRate){
        sampleRate = newSampleRate;
        deltaTime = 1.0f / newSampleRate;
    }

    void setState(bool isActive) {
        seqClock.setState(isActive);
    }

    void setSequenceLength(int newSequenceLength){
        sequenceLength = newSequenceLength;
    }

    void setSequenceProbability(float newProbability) {
        probability = newProbability;
    }

    void setRandomAmount(float newrandomAmount) {
        randomAmount = newrandomAmount;
    }

    void setClockSource(bool newClockSource){
        seqClock.setClockSource(newClockSource);
    }

    void setClockInput(float newClockIn) {
        seqClock.setClockInput(newClockIn);
    }

    void setClockFrequency(float frequency) {
        seqClock.setFrequency(frequency);
    }

    void setClockFactor(float factor) {
        seqClock.setClockFactor(factor);
    }

    void setTriggerRange(float newMinTrigger, float newMaxTrigger) {
        minTrigger = newMinTrigger;
        maxTrigger = newMaxTrigger;
    }

    void setVoltageRange(float newMinVoltage, float newMaxVoltage) {
        minVoltage = newMinVoltage;
        maxVoltage = newMaxVoltage;
    }

    void setSlewRate(float newSlewRate) {
        slewRate = 1.0f - newSlewRate;
    }

    void setQuantizerScale(int newScale){
        mainQuantizer.setScale(newScale);
        slewQuantizer.setScale(newScale);
    }

    void setSlewQuantizerMode(bool newPreQuantizer){
        slewPreQuantizer = newPreQuantizer;
    }

    float getTriggerStepValue() const {
        float scaledTrigger = minTrigger + (pulseTrigger.getTriggerOutput() * (maxTrigger - minTrigger));
        return scaledTrigger;
    }

    bool getGateStepValue() const {
        return triggerSequence[currentStep];
    }

    float getVoltageStepValue() const {
        return mainQuantizer.quantizeVoltage(voltageSequence[currentStep]);
    }

    float getVoltageSlewValue() const {
        if(slewPreQuantizer){
            return mainQuantizer.quantizeVoltage(currentVoltage);
        } else {
            return currentVoltage;
        }
    }

    float getClockValue() const {
        float scaledTrigger = minTrigger + (seqClock.getTrigger() * (maxTrigger - minTrigger));
        return scaledTrigger;
    }

    int getPulseSequence() const {
        int decimalNumber = 0;
        for (int i = 0; i < 64; ++i) {
            if (triggerSequence[i]) {
                decimalNumber |= (1 << i);
            }
        }
        return decimalNumber;
    }

private:
    int sequenceLength = 8;
    int currentStep = 0;
    bool slewPreQuantizer = false;
    bool triggerSequence[64];
    float voltageSequence[64];
    float quantizedStep = 0.0f;
    float randomAmount = 1.0f;
    float probability = 1.0f;
    float minVoltage = 0.0f; // Minimum voltage
    float maxVoltage = 5.0f; // Maximum voltage
    float minTrigger = 0.0f; // Minimum voltage
    float maxTrigger = 5.0f; // Maximum voltage
    float slewRate = 1.0f;
    float currentVoltage = 0.0f;
    float sampleRate = 44100.0f, deltaTime = 1.0f / 44100.0f; 
    ClockGenerator seqClock;
    TriggerHandler pulseTrigger;
    Quantizer mainQuantizer, slewQuantizer;

void updateSlewLimiter(float deltaTime) {
    float targetVoltage = slewQuantizer.quantizeVoltage(voltageSequence[currentStep]);
    float voltageDifference = targetVoltage - currentVoltage;
    float cycleDuration = 1.0f / seqClock.getFrequency();

    // Maximum change per clock cycle for the slowest slew rate (slewRate = 0.0f)
    float maxChangePerCycle = std::abs(voltageDifference) / cycleDuration;

    // Calculate the allowed change per update cycle
    // For slewRate = 1.0f, allowed change is the full voltage difference (instant change)
    // For slewRate = 0.0f, allowed change is part of the maxChangePerCycle
    float expSlew = powf(slewRate, 10);
    float allowedChange = (expSlew * std::abs(voltageDifference)) + 
                           ((1.0f - expSlew) * maxChangePerCycle * deltaTime);

    // Ensure the allowed change does not exceed the actual voltage difference
    allowedChange = std::min(allowedChange, std::abs(voltageDifference));

    if (allowedChange > 0) {
        currentVoltage += std::copysign(allowedChange, voltageDifference);
    }
}


};

#endif // RANDOM_SEQUENCER_HPP