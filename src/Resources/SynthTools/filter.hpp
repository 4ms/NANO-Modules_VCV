#ifndef Filter_hpp
#define Filter_hpp

#include "math.hpp"  // Include for clamp and tanh
#include "shaper.hpp"


class Filter {
public:
    // Constructor - Initializes the filter object
    Filter() {}

    // Initializes internal filter parameters (sets them to zero)
    void init(void) {
        mInput = 0.0f;        // Input signal
        mCutoff = 0.0f;       // Cutoff frequency control value (0 to 1 range)
        mResonance = 0.0f;    // Resonance control value
        mFirstStage = 0.0f;   // First stage of the filter
        mSecondStage = 0.0f;  // Second stage of the filter
        mLowPassOut = 0.0f;   // Low-pass filter output
        mBandPassOut = 0.0f;  // Band-pass filter output
        mModeWild = 0;        // Resonance mode (Mild / Wild)
    }

    // Core filter processing function - calculates the filter output stages
    void process(void) {
        // First stage: Applies cutoff and resonance to the input
        // 'tanh' smooths the nonlinearities, making the filter behave in a more analog way
        if(!mModeWild){
            mFirstStage += mCutoff * (mInput - mFirstStage + Shaper::softClip((mResonance * (tanh((mFirstStage - mSecondStage) / 10.0f))), 40.0f));
        } else {
            mFirstStage += mCutoff * (mInput - mFirstStage + Shaper::softClip((mResonance * (tanh((mFirstStage - mSecondStage) / 10.0f))), 6.0f));
        }

        // Second stage: Processes the first stage to further smooth the signal
        // The second stage is the main output for the low-pass filter
        mSecondStage += mCutoff * (mFirstStage - mSecondStage + 0.1f);

        // If resonance is too high (>= 20), the filter introduces additional feedback to enhance the resonance effect
        if (mResonance >= 20) {
            mFirstStage += 0.001f;  // Resonance enhancement feedback
        }
    }

    // Sets the input signal for the filter
    void setInput(float input) {
        mInput = input;
    }

    // Sets the cutoff frequency with clamping to avoid extremes
    void setCutoff(float cutoff) {
        mCutoff = clamp(cutoff, 0.001f, 0.999f);
    }

    // Sets the resonance with clamping and scaling
    void setResonance(float resonance) {
        if(!mModeWild){
            mResonance = clamp(resonance, 0.001f, 0.999f) * 25.0f;
        } else {
            mResonance = 18.0f + clamp(resonance, 0.001f, 0.999f) * 25.0f;
        }   
    }

    // Sets the resonance mode between Mild / Wild
    void setResonanceMode(bool wildActive){
        if (wildActive) mModeWild = true;
        else mModeWild = false;
    }

    // Gets the low-pass output (smoothed second stage)
    float getLowPass() const {
        return Shaper::hardClip(mSecondStage, 10.0f);
    }

    // Gets the band-pass output (difference between stages)
    float getBandPass() const {
        return Shaper::hardClip(mFirstStage - mSecondStage, 10.0f);
    }

    // Gets the high-pass output (input minus low-pass)
    float getHighPass() const {
        return Shaper::hardClip(mInput - mSecondStage, 10.0f);
    }

private:
    // Private member variables to hold the filter's internal state
    float mInput;         // Current input signal
    float mCutoff;        // Cutoff frequency (normalized)
    float mResonance;     // Resonance (scaled by setResonance)
    float mFirstStage;    // First stage of the filter (state variable)
    float mSecondStage;   // Second stage of the filter (state variable)
    float mLowPassOut;    // Cached low-pass output
    float mBandPassOut;   // Cached band-pass output
    bool mModeWild;       // Resonance mode (Mild / Wild)
};

#endif /* Filter_hpp */
