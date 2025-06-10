// slew_limiter.hpp
//
// Slew Limiter Class with Adjustable Timings
//
// Provides functionality for a slew limiter that smoothens an input signal
// with adjustable Rise and Fall timings.

#ifndef SLEW_LIMITER_HPP
#define SLEW_LIMITER_HPP

#include <algorithm>  // For std::max, std::min
#include <cmath>

#define STEEPNESS_FACTOR 1.5f

namespace NANO_Resources {

class SlewLimiter {
   public:
    // Default constructor
    SlewLimiter()
        : mSampleRate(44100.0f),
          mRise(0.01f),
          mFall(0.01f),
          mSlewOutput(0.0f),
          mCurveOutput(0.0f),
          mInitialOutput(0.0f),
          mNormalizedOutput(0.0f),
          mPreviousInput(0.0f),
          mTotalDistance(0.0f),
          mPhase(0.0f),
          mRising(true),
          mFalling(false) {}

    // Initializes the slew limiter with a specific sample rate
    void init(float sampleRate) {
        mSampleRate = sampleRate;
    }

    // Sets the Rise time
    void setRise(float rise) {
        mRise = rise;
    }

    // Sets the Fall time
    void setFall(float fall) {
        mFall = fall;
    }

    // Sets the shape of the Rise stage
    void setRiseShape(float shape) {
        mRiseShape = clamp(shape, 0.0f, 1.0f);
    }

    // Sets the shape of the Fall stage
    void setFallShape(float shape) {
        mFallShape = clamp(shape, 0.0f, 1.0f);
    }

    // Get the slew phase
    float getPhase(void) const {
        return mPhase;
    }

    // Get the rising state
    bool getRising(void) const {
        return mRising;
    }

    // Get the falling state
    bool getFalling(void) const {
        return mFalling;
    }

    // Process the input signal through the slew limiter
    float process(float inputSignal) {
        float deltaTime = 1.0f / mSampleRate;
        float maxRiseIncrement = deltaTime / mRise;
        float maxFallIncrement = deltaTime / mFall;

        // Detect changes in the input signal
        if (inputSignal != mPreviousInput) {
            mPreviousInput = inputSignal;                                 // Update target
            mInitialOutput = mSlewOutput;                                 // Capture starting point for new input
            mTotalDistance = std::fabs(mPreviousInput - mInitialOutput);  // Total travel distance
            mRising = (inputSignal > mSlewOutput);                        // Determine direction of change
            mFalling = (inputSignal < mSlewOutput);                       // Determine direction of change

            float minVal = std::min(mInitialOutput, mPreviousInput);
            float maxVal = std::max(mInitialOutput, mPreviousInput);
            // Avoid division by zero
            float range = (maxVal - minVal);
            float normalizedLocal = (range != 0.0f) ? (mCurveOutput - minVal) / range : 0.0f;
            normalizedLocal = clamp(normalizedLocal, 0.0f, 1.0f);

            float shapeFactor = mRising ? mRiseShape : mFallShape;
            // Use external shaping function (make sure Shaper::inverseShapePhase exists)
            mPhase = Shaper::inverseShapePhase(normalizedLocal, shapeFactor, STEEPNESS_FACTOR);
        }

        // Update slew output based on the rising or falling state
        if (mRising) {
            mSlewOutput += std::min(maxRiseIncrement, mPreviousInput - mSlewOutput);
        } else {
            mSlewOutput += std::max(-maxFallIncrement, mPreviousInput - mSlewOutput);
        }

        // Compute the curve output using an external shaping function
        {
            float minVal = std::min(mInitialOutput, mPreviousInput);
            float maxVal = std::max(mInitialOutput, mPreviousInput);
            float norm = (maxVal - minVal != 0.0f) ? (mSlewOutput - minVal) / (maxVal - minVal) : 0.0f;
            mCurveOutput = Shaper::shapeCurve(norm, minVal, maxVal, mRising ? mRiseShape : mFallShape, STEEPNESS_FACTOR);
        }

        // Calculate progress phase
        if (mTotalDistance != 0.0f) {
            if (mRising) {
                mPhase = (mSlewOutput - mInitialOutput) / mTotalDistance;
            } else {
                mPhase = 1.0f - (mSlewOutput - mInitialOutput) / mTotalDistance;
            }
        } else {
            mPhase = 1.0f;
        }
        mPhase = clamp(mPhase, 0.0f, 1.0f);

        // Reset rising/falling if target is reached
        if (inputSignal == mSlewOutput) {
            mRising = false;
            mFalling = false;
        }

        return mCurveOutput;
    }

   private:
    float mSampleRate;
    float mRise;
    float mFall;
    float mRiseShape;
    float mFallShape;
    float mSlewOutput;
    float mCurveOutput;
    float mInitialOutput;
    float mNormalizedOutput;
    float mPreviousInput;
    float mTotalDistance;
    float mPhase;
    bool mRising;
    bool mFalling;

    // Utility clamping function
    float clamp(float value, float min, float max) const {
        return std::max(min, std::min(value, max));
    }
};

}  // namespace NANO_Resources

#endif  // SLEW_LIMITER_HPP