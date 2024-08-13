#ifndef CLOCK_HPP
#define CLOCK_HPP

#include "../uiTools/trigger.hpp"
#include <iostream>

class ClockMultiplier {

public:
    ClockMultiplier(int factor) 
        : multiplicationFactor(factor), // Set to the provided factor
          multPhase(0.0f), // Initialize phase accumulator to 0
          pulseCount(0), // Initialize pulse count, starting with 0 pulses generated
          lastPulseState(false), // The last state of the incoming pulse is false (no pulse)
          sampleCountSinceLastPulse(0), // No samples counted since the last pulse yet
          incomingPeriod(0.0f), // No period calculated yet
          pulseInterval(0.0f), // No pulse interval determined yet
          sampleRate(44100.0f), // Default sample rate, likely to be overridden
          deltaTime(1.0f / 44100.0f) // Calculated from the default sample rate
    {}

    void init() {
        inTrig.init();
        multTrig.init();
        inTrig.setTriggerDurationMs(0.1f);
        multTrig.setTriggerDurationMs(0.1f);
    }

    void update(float incomingPulse) {
        inTrig.process(incomingPulse);

        // Handle both multiplication and division based on the multiplication factor
        if (multiplicationFactor >= 1.0) {
            // Multiplication logic (as before)
            if (inTrig.getTriggerJustOccurred()) {
                resetForNewPulse();
            } else {
                sampleCountSinceLastPulse++;
            }

            if (pulseCount < multiplicationFactor) {
                generateMultiples();
            }
        } else {
            // Division logic
            if (inTrig.getTriggerJustOccurred()) {
                // Increment division counter for each incoming pulse
                divisionCounter++;
                // Calculate the division factor as the inverse of multiplicationFactor
                float divisionFactor = 1.0 / multiplicationFactor;

                if (divisionCounter >= divisionFactor) {
                    // Generate a pulse after enough incoming pulses have been received
                    multTrig.process(true);
                    divisionCounter = 0; // Reset counter
                }
            } else {
                sampleCountSinceLastPulse++;
                multTrig.process(false);
            }
        }
    }

    void setSampleRate(float newSampleRate){
        sampleRate = newSampleRate;
        deltaTime = 1.0f / newSampleRate;
    }

    void setFactor(float factor) {
        multiplicationFactor = factor;
    }

    float getMultOut(void) const {
        return multTrig.getTriggerOutput();
    }

    bool getMultInstantOut(void) const {
        return multTrig.getTriggerJustOccurred();
    }

    float getPhaseOut(void) const {
        return multPhase;
    }

    private:
    float multiplicationFactor; // Factor by which to multiply the incoming clock
    int divisionCounter; // Tracks the number of incoming pulses for division
    float multPhase; // Accumulator for phase to determine when to emit a pulse
    int pulseCount; // Counter for the number of pulses generated in the current cycle
    bool lastPulseState; // Tracks the state of the last pulse to detect rising edges
    int sampleCountSinceLastPulse; // Counts the samples since the last incoming pulse
    float incomingPeriod; // The calculated period of the incoming clock signal
    float incomingPulse; // The pulse input signal
    float pulseInterval; // The target interval between pulses for the multiplied clock
    float sampleRate = 44100.0f, deltaTime = 1.0f / 44100.0f;
    TriggerHandler inTrig, multTrig;

    void resetForNewPulse() {
        // Calculate the time since the last pulse
        incomingPeriod = static_cast<float>(sampleCountSinceLastPulse * deltaTime);
        pulseInterval = incomingPeriod / multiplicationFactor;
        // Reset counters
        sampleCountSinceLastPulse = 0;
        multPhase = 0.0f;
        pulseCount = 0;
        multTrig.process(true);
    }

    void generateMultiples() {
        multPhase += deltaTime;

        if (multPhase >= pulseInterval) {
            multTrig.process(true);
            multPhase -= pulseInterval;
            pulseCount++;
        } else {
            multTrig.process(false);
        }
    }
};

class ClockGenerator {
public:
    // Constructor
    ClockGenerator(int multFactor = 1) 
        : frequency(1.0f), // Default frequency set to 1Hz
          running(false), // Initially not running
          phase(0.0f), // Start with phase 0
          sampleRate(44100.0f), // Default sample rate, likely to be overridden
          deltaTime(1.0f / 44100.0f), // Calculated from the default sample rate
          clockIn(false), // Default clock input state
          clockSource(false), // Default clock source state (internal/external)
          clockMult(multFactor) // Initialize ClockMultiplier with provided factor
    {}

    void init(void) {
        clockTrig.init();
        clockTrig.setTriggerDurationMs(0.1f);
        clockMult.init();
        clockMult.setSampleRate(sampleRate);
        clockMult.setFactor(1);
    }

    void setClockSource(bool newClockSource) {
        clockSource = newClockSource;
    }

    void setClockInput(float newClockIn) {
        clockIn = newClockIn;
    }

    void setClockFactor(float newMultFactor) {
        clockMult.setFactor(newMultFactor);
        clockFactor = newMultFactor;
    }

    void setFrequency(float newFrequency) {
        frequency = newFrequency;
    }

    void setState(bool newRunning) {
        running = newRunning;
    }

    void setSampleRate(float newSampleRate){
        sampleRate = newSampleRate;
        deltaTime = 1.0f / newSampleRate;
    }

    void update(void) {
        if (!running) { return; }
        if (clockSource) {

            phase += frequency * deltaTime * clockFactor;

            if(phase < 0.1f){
                phasePulse = true;
            } else {
                phasePulse = false;
            }

            clockTrig.process(phasePulse); // Consider making this sample rate configurable

            if (phase >= 1.0f) {
                phase -= 1.0f;
                // Trigger events or actions on clock pulse (e.g., advancing sequencer step)
            }
        } else {
            clockMult.update(clockIn); // Consider making this sample rate configurable
        }
    }

    float getTrigger() const {
        if(clockSource) return clockTrig.getTriggerOutput(); // Fixed to return the trigger output
        else return clockMult.getMultOut();
    }

    bool getInstantTrigger() const {
        if(clockSource) return clockTrig.getTriggerJustOccurred(); // Fixed to return the trigger output
        else return clockMult.getMultInstantOut();
    }

    float getPhase() const {
        if(clockSource) return phase;
        else return clockMult.getPhaseOut(); 
    }

    float getFrequency() const {
        return frequency;
    }

    bool getState() const {
        return running;
    }

private:
    float frequency = 1.0f; // Frequency of the clock
    bool running = false; // State of the clock (running or stopped)
    float phase = 0.0f; // Current phase of the clock
    float sampleRate = 44100.0f, deltaTime = 1.0f / 44100.0f;
    float clockIn = 0.0f, clockFactor = 1.0f;
    bool clockSource = false;
    bool phasePulse = false;
    TriggerHandler clockTrig;
    ClockMultiplier clockMult;
};

#endif // CLOCK_HPP
