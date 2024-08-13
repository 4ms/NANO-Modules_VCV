#ifndef ENCODER_HPP
#define ENCODER_HPP

#include "rack.hpp"

using namespace rack;

class EncoderHandler {
public:
    EncoderHandler() = default;

    // Process encoder input and update the parameter accordingly
    float process(float currentEncoderValue, float& paramValue, float scalingFactor, float minValue, float maxValue) {
        float encoderDelta = currentEncoderValue - lastEncoderValue;
        lastEncoderValue = currentEncoderValue; // Update the last value for the next frame
        return onEncoderChange(encoderDelta, paramValue, scalingFactor, minValue, maxValue);
    }

private:
    float lastEncoderValue = 0.0f; // Stores the last encoder value to calculate deltas

    // Updates the parameter value based on the encoder's change, applies scaling, and clamps the value
    static float onEncoderChange(float delta, float& paramValue, float scalingFactor, float minValue, float maxValue) {
        paramValue += delta * scalingFactor;
        paramValue = clamp(paramValue, minValue, maxValue);
        return paramValue;
    }
};

#endif // ENCODER_HPP
