#ifndef trigger_hpp
#define trigger_hpp

#include <algorithm> // For std::max

class TriggerHandler {
public:
    TriggerHandler()
    : inputValue(0.0f), triggerOutput(0.0f), previousInputValue(0.0f), 
      threshold(0.5f), lowOutputValue(0.0f), highOutputValue(1.0f), 
      triggerDuration(0), triggerTimer(0), gateOutput(0.0f), triggerJustOccurred(false) {}

    void init() {
        inputValue = 0.0f;
        triggerOutput = 0.0f;
        previousInputValue = 0.0f;
        threshold = 0.5f;
        lowOutputValue = 0.0f;
        highOutputValue = 1.0f;
        gateOutput = lowOutputValue;
        triggerJustOccurred = false; // Ensure it's reset on initialization
    }

    void process(float inputValue) {
        if (inputValue > threshold && previousInputValue <= threshold) {
            // Rising edge detected
            triggerOutput = highOutputValue;
            gateOutput = highOutputValue; // Gate goes high on rising edge
            triggerTimer = triggerDuration;
            triggerJustOccurred = true; // Trigger has just occurred
        } else {
            if (triggerJustOccurred) {
                // Reset the trigger just occurred flag after the first process call following the trigger
                triggerJustOccurred = false;
            } else if (inputValue < threshold) {
                // Handle falling edge or continuous low state
                gateOutput = lowOutputValue; // Ensures gate and trigger output are consistent with trigger state
            }
        }

        // Trigger timer countdown
        if (triggerTimer > 0) {
            triggerTimer -= deltaTime;
            if (triggerTimer <= 0) {
                triggerOutput = lowOutputValue;
                triggerTimer = 0;
            }
        }

        previousInputValue = inputValue;
    }

    void setSampleRate(float newSampleRate) {
        sampleRate = newSampleRate;
        deltaTime = 1.0f / newSampleRate;
    }

    void setThreshold(float thresholdValue) {
        threshold = thresholdValue;
    }

    void setLowValue(float lowThresholdValue) {
        lowOutputValue = lowThresholdValue;
    }

    void setHighValue(float highThresholdValue) {
        highOutputValue = highThresholdValue;
    }

    float getTriggerOutput() const {
        return triggerOutput;
    }

    float getGateOutput() const {
        return gateOutput;
    }

    // New getter to check if a trigger just occurred
    bool getTriggerJustOccurred() const {
        return triggerJustOccurred;
    }

    void setTriggerDurationMs(float durationMs) {
        triggerDuration = std::max(0.0f, durationMs) / 1000.0f; // Convert ms to seconds
    }

private:
    float sampleRate = 44100.0f, deltaTime = 1.0f / 44100.0f;
    float inputValue;
    float triggerOutput;
    float previousInputValue;
    float threshold;
    float lowOutputValue;
    float highOutputValue;
    float triggerDuration; // Duration of the trigger in seconds
    float triggerTimer; // Timer to track the duration of the trigger
    float gateOutput;
    bool triggerJustOccurred; // Indicates if a trigger event just occurred
};

#endif /* trigger_hpp */
