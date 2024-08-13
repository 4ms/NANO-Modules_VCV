// envelope.hpp
//
// Combined Attack-Decay (AD) and Attack-Decay-Sustain-Release (ADSR) Envelope Generators
//
// Provides functionality for both AD and ADSR envelope generators,
// essential components in synthesizers for dynamic sound shaping.

#ifndef ENVELOPE_HPP
#define ENVELOPE_HPP

#include <algorithm> // For std::max
#include <cmath> // For pow and other mathematical operations

#include "../uiTools/trigger.hpp" // For trig class
#include "shaper.hpp" // For trig class

#define CURVE_FACTOR 5.0f
#define STEEPNESS_FACTOR 1.0f

class ADEnvelope {
public:
    // Default constructor
    ADEnvelope() : mSampleRate(44100.0f), mCurrentPhase(0.0f), mState(State::Idle),
                mAttackShape(0.5f), mDecayShape(0.5f),
                mOutputLevel(1.0f), mOutputOffset(0.0f), mResetOnTrigger(false) {}

    // Enums to represent the state of the envelope
    enum class State {
        Idle,
        Attack,
        Decay
    };

    // Initializes the envelope with a specific sample rate
    void init(float sampleRate) {
        mSampleRate = sampleRate;
        inTrig.init();
        inTrig.setTriggerDurationMs(0.1f);
    }

    // Sets the attack time
    void setAttack(float attack) {
        mAttack = attack;
    }

    // Sets the decay time
    void setDecay(float decay) {
        mDecay = decay;
    }

    // Sets the shape of the attack stage
    void setAttackShape(float shape) {
        mAttackShape = clamp(shape, 0.0f, 1.0f);
    }

    // Sets the shape of the decay stage
    void setDecayShape(float shape) {
        mDecayShape = clamp(shape, 0.0f, 1.0f);
    }

    // Sets the looping functionality
    void setLooping(bool looping) {
        // Check if looping is being enabled and the envelope is not currently cycling
        if (looping && !mLooping && mState == State::Idle) {
            trigger(); // Automatically trigger the envelope to start cycling
        }
        mLooping = looping;
    }

    // Sets the output level
    void setOutputLevel(float level) {
        mOutputLevel = level;
    }

    // Sets the output offset
    void setOutputOffset(float offset) {
        mOutputOffset = offset;
    }

    // Method to set re-trigger behavior
    void setResetOnTrigger(bool reset) {
        mResetOnTrigger = reset;
    }

    // Trigger the envelope
    void trigger() {
        if (mResetOnTrigger) {
            mCurrentPhase = 0.0f; // Reset phase to the start for a new attack
        } else if (mState == State::Decay){
            // Recalculate starting phase for a smooth transition into the attack phase
            mCurrentPhase = Shaper::inverseShapePhase(getCurveNormalized(), mAttackShape, STEEPNESS_FACTOR); // Adjust steepness as needed
        }
        mState = State::Attack;
    }

    // Process the envelope for the current sample
    void process(float triggerValue) {
        inTrig.process(triggerValue);

        if (inTrig.getTriggerOutput()) {
            trigger();
        }

        switch (mState) {
            case State::Attack:
                advancePhase(mCurrentPhase, mAttack, mAttackShape, true);
                if (mCurrentPhase >= 1.0f) {
                    mCurrentPhase = 1.0f;
                    mState = State::Decay;
                }
                break;
            case State::Decay:
                advancePhase(mCurrentPhase, mDecay, mDecayShape, false);
                if (mCurrentPhase <= 0.0f) {
                    if (mLooping) {
                        trigger(); // Restart the envelope if looping is enabled
                    } else {
                        mCurrentPhase = 0.0f;
                        mState = State::Idle;
                    }
                }
                break;
            case State::Idle:
                // No further action required
                break;
        }
    }

    // Get the current output level of the envelope
    float getPhaseOutput() const {
        return (mCurrentPhase * mOutputLevel) + mOutputOffset;
    }

    // Get the current curve output level of the envelope
    float getCurveOutput() const {
        return (mCurrentCurve * mOutputLevel) + mOutputOffset;
    }

    // Get the current curve output level from 0.0f to 1.0f of the envelope
    float getCurveNormalized() const {
        return mCurrentCurve;
    }

    bool getEndOfCycle() const {
        return mCurrentCurve < 0.01f;
    }

private:
    float mSampleRate;
    float mAttack;
    float mDecay;
    float mLooping;
    float mCurrentPhase;
    float mCurrentCurve;
    float mAttackShape;
    float mDecayShape;
    float mOutputLevel;
    float mOutputOffset;
    bool mResetOnTrigger;
    TriggerHandler inTrig;
    State mState;

    void advancePhase(float& currentPhase, float duration, float shape, bool increasing) {
        float deltaTime = 1.0f / mSampleRate; // Time passed per sample
        float phaseProgress = deltaTime / duration; // Fraction of the phase completed per sample

        if (increasing) {
            currentPhase += phaseProgress; // Increase linearly for now
        } else {
            currentPhase -= phaseProgress; // Decrease linearly for now
        }

        // Apply shape-based adjustment after calculating the linear progress.
        currentPhase = clamp(currentPhase, 0.0f, 1.0f); // Ensure currentPhase stays within bounds
        mCurrentCurve = Shaper::shapeCurve(currentPhase, 0.0f, 1.0f, shape, STEEPNESS_FACTOR);
    }
};

class ADSREnvelope {
public:
    // Default constructor
    ADSREnvelope() : mSampleRate(44100.0f), mCurrentPhase(0.0f), mState(State::Idle),
                     mAttackShape(0.5f), mDecayReleaseShape(0.5f), mSustainLevel(0.5f),
                     mOutputLevel(1.0f), mOutputOffset(0.0f), mResetOnTrigger(false) {}

    // Enums to represent the state of the envelope
    enum class State {
        Idle,
        Attack,
        Decay,
        Sustain,
        Release
    };

// Initializes the envelope with a specific sample rate
    void init(float sampleRate) {
        mSampleRate = sampleRate;
        inGate.init();
        inGate.setTriggerDurationMs(0.1f);
        inRetrig.init();
        inRetrig.setTriggerDurationMs(0.1f);        
    }

    // Sets the attack time
    void setAttack(float attack) {
        mAttack = attack;
    }

    // Sets the decay time
    void setDecay(float decay) {
        mDecay = decay;
    }

    // Set the sustain level
    void setSustain(float level) {
        mSustainLevel = clamp(level, 0.0f, 0.999f);
    }

    // Set the release time
    void setRelease(float release) {
        mRelease = release;
    }

    // Sets the shape of the attack stage
    void setAttackShape(float shape) {
        mAttackShape = clamp(shape, 0.0f, 1.0f);
    }

    // Sets the shape of the decay stage
    void setDecayReleaseShape(float shape) {
        mDecayReleaseShape = clamp(shape, 0.0f, 1.0f);
    }

    // Sets the output level
    void setOutputLevel(float level) {
        mOutputLevel = level;
    }

    // Sets the output offset
    void setOutputOffset(float offset) {
        mOutputOffset = offset;
    }

    // Method to set re-trigger behavior
    void setResetOnTrigger(bool reset) {
        mResetOnTrigger = reset;
    }

    // Trigger the envelope
    void trigger() {
        if (mResetOnTrigger) {
            mCurrentPhase = 0.0f; // Reset phase to the start for a new attack
        } else if (mState == State::Decay || mState == State::Sustain || mState == State::Release){
            // Recalculate starting phase for a smooth transition into the attack phase
            mCurrentPhase = Shaper::inverseShapePhase(getCurveNormalized(), mAttackShape, STEEPNESS_FACTOR); // Adjust steepness as needed
        }
        mState = State::Attack;
    }

    // Trigger the envelope
    void retrigger(float trig) {
        inRetrig.process(trig);
        if (inRetrig.getTriggerJustOccurred()){
            trigger();
        }
    }

    // Extend the process method to include gate signal handling for Sustain
    void process(float gateSignal) {
        inGate.process(gateSignal);

        if (inGate.getGateOutput() && (mState == State::Idle || mState == State::Release)) {
            trigger();
        } else if (!inGate.getGateOutput() && (mState == State::Attack || mState == State::Decay || mState == State::Sustain)) {
            release();
        }

        switch (mState) {
            case State::Attack:
                advancePhase(mCurrentPhase, mAttack, mAttackShape, true);
                if (mCurrentPhase >= 1.0f) {
                    mState = State::Decay;
                }
                break;
            case State::Decay:
                advancePhase(mCurrentPhase, mDecay, mDecayReleaseShape, false); // Ensure decayReleaseShape is applied correctly
                if (mCurrentPhase <= mSustainLevel) {
                    mCurrentPhase = mSustainLevel; // Explicitly set to sustain level to avoid variations
                    mState = State::Sustain;
                }
                break;
            case State::Sustain:
                // During Sustain, mCurrentPhase should simply equal mSustainLevel, without further adjustments
                mCurrentPhase = mSustainLevel;
                break;
            case State::Release:
                advancePhase(mCurrentPhase, mRelease, mDecayReleaseShape, false); // Apply the shape on release too
                if (mCurrentPhase <= 0.0f) {
                    mCurrentPhase = 0.0f;
                    mState = State::Idle;
                }
                break;
            case State::Idle:
                break;
        }
    }


    // Implement a method to start the release phase
    void release() {
        if (mState == State::Sustain || mState == State::Attack || mState == State::Decay) {
            // Calculate the release phase starting point based on the current output level and the decay/release shape
            float currentOutputLevel = getCurveNormalized(); // Or getPhaseOutput(), depending on how you're calculating this
            mCurrentPhase = Shaper::inverseShapePhase(currentOutputLevel, mDecayReleaseShape, STEEPNESS_FACTOR);
            mState = State::Release;
        }
    }

    // Get the current output level of the envelope
    float getPhaseOutput() const {
        return (mCurrentPhase * mOutputLevel) + mOutputOffset;
    }

    // Get the current curve output level of the envelope
    float getCurveOutput() const {
        return (mCurrentCurve * mOutputLevel) + mOutputOffset;
    }

    // Get the current curve output level from 0.0f to 1.0f of the envelope
    float getCurveNormalized() const {
        return mCurrentCurve;
    }

    bool getEndOfCycle() const {
        return mCurrentCurve < 0.01f;
    }

private:
    float mSampleRate;
    float mAttack, mDecay, mRelease;
    float mCurrentPhase, mCurrentCurve;
    float mAttackShape, mDecayReleaseShape, mSustainLevel;
    float mOutputLevel, mOutputOffset;
    bool mResetOnTrigger;
    TriggerHandler inGate, inRetrig;
    State mState;

    void advancePhase(float& currentPhase, float duration, float shape, bool increasing) {
        float deltaTime = 1.0f / mSampleRate; // Time passed per sample
        float phaseProgress = deltaTime / duration; // Fraction of the phase completed per sample

        if (increasing) {
            currentPhase += phaseProgress; // Increase linearly for now
        } else {
            currentPhase -= phaseProgress; // Decrease linearly for now
        }

        // Apply shape-based adjustment after calculating the linear progress.
        currentPhase = clamp(currentPhase, 0.0f, 1.0f); // Ensure currentPhase stays within bounds
        mCurrentCurve = Shaper::shapeCurve(currentPhase, 0.0f, 1.0f, shape, STEEPNESS_FACTOR);

        if(mState == State::Decay){
            // Normalize the phase for shaping purposes: Map [sustainLevel, 1.0f] to [0.0f, 1.0f]
            float normalizedPhase = (currentPhase - mSustainLevel) / (1.0f - mSustainLevel);

            // Apply the shaping function to the normalizedPhase
            mCurrentCurve = Shaper::shapeCurve(normalizedPhase, mSustainLevel, 1.0f, shape, STEEPNESS_FACTOR);
        }
    }
};

#endif // ENVELOPE_HPP
