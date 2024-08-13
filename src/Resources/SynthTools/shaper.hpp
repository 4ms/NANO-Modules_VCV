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
};

#endif // SHAPER_HPP
