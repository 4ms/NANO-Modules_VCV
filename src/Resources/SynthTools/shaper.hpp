#ifndef SHAPER_HPP
#define SHAPER_HPP

#include <cmath> // For powf and log10

class Shaper {
public:
    // Helper function to linearly interpolate between two values
    static float interpolate(float a, float b, float t) {
        return a * (1 - t) + b * t;
    }

    // Helper function to find the interpolation factor given two values and a target
    static float findInterpolationFactor(float a, float b, float target) {
        // Avoid division by zero
        if (a == b) return 0.0f;
        return (target - a) / (b - a);
    }

    // Helper function for adjusting input range
    static float adjustRange(float input, float minValue, float maxValue) {
        return (input * (maxValue - minValue) + minValue);
    }

    // Function to map a value from [inputMin, inputMax] to [minValue, maxValue] with middleValue at (inputMin + inputMax) / 2
    static float mapToRange(float input, float inputMin, float inputMax, float minValue, float middleValue, float maxValue) {
        // Normalize the input to a [-1, 1] range
        float normalizedInput = 2.0f * (input - inputMin) / (inputMax - inputMin) - 1.0f;

        // Ensure minValue is not zero to avoid division by zero
        if (minValue == 0.0f) {
            minValue = 0.0001f; // Set a small value to avoid division by zero
        }

        // Use exponential scaling to map normalized input range [-1, 1] to [minValue, maxValue]
        float output = minValue * powf((maxValue / minValue), (normalizedInput + 1.0f) / 2.0f);

        // Adjust the output so that normalized input 0 maps exactly to middleValue
        float middleFactor = middleValue / (minValue * powf((maxValue / minValue), 0.5f));
        return output * middleFactor;
    }


    // Shaping function with normalization and interpolation
    static float shapeCurve(float currentPhase, float minValue, float maxValue, float shape, float steepnessAdjustment) {
        float k = 1 + steepnessAdjustment * 9; // Example adjustment

        // Calculate exponential and logarithmic outputs
        float expMaxOutput = exp(k) - 1;
        float expOutput = (exp(k * currentPhase) - 1) / expMaxOutput;

        float logMaxOutput = log(k + 1);
        float logOutput = log(k * currentPhase + 1) / logMaxOutput;

        // Interpolate between the curves based on the shape parameter
        if (shape < 0.5f) {
            // Interpolate between exponential and linear
            float t = shape * 2; // Map shape range from [0, 0.5] to [0, 1]
            return adjustRange(interpolate(expOutput, currentPhase, t), minValue, maxValue);
        } else if (shape > 0.5f) {
            // Interpolate between linear and logarithmic
            float t = (shape - 0.5f) * 2; // Map shape range from [0.5, 1] to [0, 1]
            return adjustRange(interpolate(currentPhase, logOutput, t), minValue, maxValue);
        } else {
            // Linear case, no interpolation needed
            return adjustRange(currentPhase, minValue, maxValue);
        }
    }

    // Inverse shaping function with normalization and interpolation
    static float inverseShapePhase(float outputLevel, float shape, float steepnessAdjustment) {
        float k = 1 + steepnessAdjustment * 9; // Adjust as in shapePhase

        // Calculate the max outputs for exponential and logarithmic shapes
        float expMaxOutput = exp(k) - 1;
        float logMaxOutput = log(k + 1);

        if (shape < 0.5f) {
            // The shape is closer to exponential, interpolate between exponential and linear
            float expOutput = log(outputLevel * expMaxOutput + 1) / k;
            float linOutput = outputLevel;
            float t = shape * 2; // Map shape range from [0, 0.5] to [0, 1]
            return interpolate(expOutput, linOutput, t);
        } else if (shape > 0.5f) {
            // The shape is closer to logarithmic, interpolate between linear and logarithmic
            float linOutput = outputLevel;
            float logOutput = (exp(outputLevel * logMaxOutput) - 1) / k;
            float t = (shape - 0.5f) * 2; // Map shape range from [0.5, 1] to [0, 1]
            return interpolate(linOutput, logOutput, t);
        } else {
            // Linear case, no interpolation needed
            return outputLevel;
        }
    }

    // Soft clipping function with adjustable threshold (e.g., ±5V)
    static float softClip(float input, float threshold) {
        // Scale the input based on the threshold and apply tanh for soft clipping
        return threshold * std::tanh(input / threshold);
    }

    // Hard clipping function with adjustable threshold
    static float hardClip(float input, float threshold) {
        if (input > threshold) {
            return threshold; // Clip to positive threshold
        } else if (input < -threshold) {
            return -threshold; // Clip to negative threshold
        }
        return input; // No clipping needed
    }

    // Crossfade function that can handle from 2 to 8 inputs, with a crossfadeValue in range [0, 1]
    static float multiInputCrossfade(const float* inputs, int numInputs, float crossfadeValue) {
        // Ensure the number of inputs is between 2 and 8
        if (numInputs < 2 || numInputs > 8) {
            return 0.0f; // Invalid input count, return 0
        }

        // Scale the crossfade value to fit the number of inputs
        float scaledValue = crossfadeValue * (numInputs - 1); // Range [0, numInputs-1]
        int lowerIndex = static_cast<int>(scaledValue);       // Determine which input to fade from
        int upperIndex = lowerIndex + 1;                      // Fade to the next input

        // Ensure upper index is within bounds
        if (upperIndex >= numInputs) {
            upperIndex = numInputs - 1;
        }

        // Calculate the local fade amount (how much to blend between lowerIndex and upperIndex)
        float localFade = scaledValue - lowerIndex;

        // Crossfade between the lower and upper inputs
        return interpolate(inputs[lowerIndex], inputs[upperIndex], localFade);
    }
};

#endif // SHAPER_HPP
