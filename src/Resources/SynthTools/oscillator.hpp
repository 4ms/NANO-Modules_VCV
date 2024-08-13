//
//  oscillator.hpp
//
//  Created by Jorge Gutierrez-Rave Olmos / NANO Modules on 16/08/22.
//

#ifndef Oscillator_hpp
#define Oscillator_hpp

#include <math.h>
#include <stdint.h>

// Oscillator class defines the core functionality for generating various waveforms.
// Supports sine, triangle, sawtooth (with BLEP for anti-aliasing), and pulse waves.
// Capable of handling frequency modulation for rise/fall times and supports one-shot or looped playback.
class Oscillator {
public:
  // Default constructor
  Oscillator() {}

  // Configuration flags for oscillator playback mode
  bool ONE_SHOT = false;
  bool LOOP_OSC = true;

  // Initializes the oscillator with default values and sets the loop mode.
  // Parameters:
  // - loop: Determines if the oscillator should loop continuously or play once (one-shot).
  void init(bool loop) {
    // Set initial frequencies for rise and fall to middle A (440 Hz halved)
    mFrequency = 220.0f;
    mFreqRise = 220.0f;
    mFreqFall = 220.0f;

    // Initial phase of the waveform, starts at zero
    mPhase = 0.0f;

    // Time increment per sample, assuming a sample rate of 44.1 kHz
    mDelta = 1.0f / 44100.0f;

    // Calculate the phase increment based on frequency and sample rate
    mPhaseIncrement = mFrequency * mDelta;

    // Maximum value of phase before it wraps around
    mMaxPhase = 1.0f;

    // Amplitude limits for the output waveform
    mMaxWave = 5.0f;
    mMinWave = -5.0f;

    // Normalized phase for use in waveform generation
    mTau = mPhase / mMaxPhase;

    // Width of the pulse waveform, default set to 50%
    mPulseWidth = 0.5f;

    // Placeholder for output values, not directly used
    mOutput = 0.0f;
    mOutputRamp = 0.0f;

    // Curve factors for shaping the rise and fall of the envelope
    mRiseCurveFactor = 3.0f;
    mFallCurveFactor = 4.0f;

    // Initialize playback modes
    mOneShot = false;
    mLoop = loop;
    mRising = false; // Used to indicate if the waveform is in the rising phase
  }

  // Triggers a one-shot playback of the oscillator wave.
  void triggerOneShot() {
    mOneShot = true; // Enable one-shot mode
    mPhase = 0.0f; // Reset phase to start
  }

  // Advances the oscillator's phase, handling rise/fall frequencies and loop/one-shot behavior.
  void phaseTick() {
    if (mLoop || mOneShot) { // Check if we're in a mode that requires phase advancement
      // Adjust frequency based on current phase to create rise/fall effect
      if(mPhase <= mMaxPhase / 2.0f) {
        mFrequency = mFreqRise;
      } else {
        mFrequency = mFreqFall;
      }

      // Recalculate phase increment based on possibly new frequency
      mPhaseIncrement = mFrequency * mDelta;

      // Advance phase by the increment
      mPhase += mPhaseIncrement;

      // Prevent phase from exceeding maximum, allowing for waveform repetition
      mPhase = clamp(mPhase, 0.0f, mMaxPhase);

      // If phase completes a full cycle
      if (mPhase >= mMaxPhase) {
        mPhase -= mMaxPhase; // Wrap phase around for continuous oscillation
        mOneShot = false; // Disable one-shot if it was active, as we've completed a cycle
      }

      // Update the normalized phase value
      mTau = mPhase / mMaxPhase;
    }
  }

  // Shapes the output ramp signal, calculating rising and falling edges based on the phase.
  void rampShaper() {
    // Calculate rising edge of the waveform
    if (mPhase <= mMaxPhase / 2.0f) {
      mOutputRamp = powf(mPhase * 2.0f * powf(1.0f, mRiseCurveFactor - 1.0f), 1 / mRiseCurveFactor);
      mRising = true; // Indicate that we're in the rising phase
    } else { // Calculate falling edge of the waveform
      mOutputRamp = powf(((1.0f - mPhase) * 2), mFallCurveFactor) / powf(1.0f, mFallCurveFactor - 1.0f);
      mRising = false; // Indicate that we're in the falling phase
    }
  }

  // Sets the oscillator's base frequency for both rise and fall.
  // Parameters:
  // - frequency: The frequency in Hz to set for the oscillator.
  void setFrequency(float frequency) {
    mFrequency = frequency;
    mFreqRise = frequency;
    mFreqFall = frequency;
  }

  // Sets the frequency for the rising edge of the waveform.
  // Parameters:
  // - riseFrequency: The rise frequency in Hz.
  void setRiseFrequency(float riseFrequency) {
    mFreqRise = riseFrequency;
  }

  // Sets the frequency for the falling edge of the waveform.
  // Parameters:
  // - fallFrequency: The fall frequency in Hz.
  void setFallFrequency(float fallFrequency) {
    mFreqFall = fallFrequency;
  }

  // Sets the sample frequency, affecting phase increment calculations.
  // Parameters:
  // - sampleFreq: The sample frequency in Hz.
  void setSampleFreq(float sampleFreq) {
    mDelta = 1.0f / sampleFreq; // Recalculate delta based on new sample frequency
  }

  // Returns the current phase of the oscillator.
  float getPhase() {
    return mPhase; // Simply return the current phase value
  }

  // Generates a sine wave output based on the current phase.
  float getSineWave() {
    // Calculate sine wave based on current phase, scaling by max amplitude
    return mMaxWave * std::cos(mPhase * M_PI * 2.0f);
  }

  // Generates a triangle wave output based on the current phase.
  float getTriangleWave() {
    // Determine the triangle wave value based on current phase position
    if(mPhase <= mMaxPhase / 2.0f) {
      return mMinWave + ((mMaxWave * mPhase * 4.0f));
    } else {
      return mMaxWave * 3.0f + ((mMinWave * mPhase * 4.0f));
    }
  }

  // Generates a sawtooth wave output using BLEP for anti-aliasing.
  float getSawWave() {
    // Basic sawtooth wave calculation with phase, modified by BLEP for smoothing
    float saw = 2.0f * mPhase;
    saw -= PolyBLEP(mTau, mPhaseIncrement);
    return (saw * mMaxWave) + mMinWave;
  }

  // Generates a pulse wave output using BLEP for anti-aliasing.
  float getPulseWave() {
    // Basic pulse wave calculation with conditional amplitude based on phase
    float pul = 0.0f;
    if (mPhase < mPulseWidth) {
      pul = mMaxWave;
    } else {
      pul = mMinWave;
    }
    // Apply BLEP to both the leading and trailing edges of the pulse for smoothing
    pul += PolyBLEP(mTau, mPhaseIncrement);
    pul -= PolyBLEP(fmod(mTau + (1.0f - mPulseWidth), 1.0), mPhaseIncrement);
    return pul;
  }

  // Returns the current envelope ramp output, useful for modulating other parameters.
  float getEnvelopeRamp() {
    return mOutputRamp; // Return the calculated ramp output
  }

private:
  // Private member variables to hold oscillator state and configuration.
  float mFrequency; // Base frequency of the oscillator
  float mFreqRise; // Frequency during the rise phase
  float mFreqFall; // Frequency during the fall phase
  float mPhase; // Current phase of the oscillator
  float mPhaseIncrement; // Amount to increment the phase each tick
  float mDelta; // Delta time per sample
  float mMaxPhase; // Maximum value of the phase before wrapping
  float mMaxWave; // Maximum output amplitude
  float mMinWave; // Minimum output amplitude
  float mTau; // Normalized phase used in BLEP calculations
  float mPulseWidth; // Width of the pulse wave
  float mOutput; // Current output amplitude (not used in current implementation)
  float mOutputRamp; // Current output of the ramp shaper
  float mRiseCurveFactor; // Curve factor for the rising edge
  float mFallCurveFactor; // Curve factor for the falling edge
  bool mOneShot; // Flag to indicate one-shot playback
  bool mLoop; // Flag to indicate looped playback
  bool mRising; // Flag to indicate if the phase is in the rising portion
  // PolyBLEP function used to reduce aliasing in sawtooth and pulse waveforms.
  // Parameters:
  // - t: Normalized time parameter for the current phase
  // - mPhaseIncrement: The current phase increment
  float PolyBLEP(float t, float mPhaseIncrement) {
    // Calculate BLEP for smooth transitions in sawtooth and pulse waveforms
    float dt = mPhaseIncrement / M_PI_2;
    if (t < dt) {
      t /= dt;
      return t + t - t * t - 1.0;
    } else if (t > 1.0 - dt) {
      t = (t - 1.0) / dt;
      return t * t + t + t + 1.0;
    } else {
      return 0.0;
    }
  }
};

#endif /* Oscillator_hpp */
