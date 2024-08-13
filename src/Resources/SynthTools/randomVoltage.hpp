//
//  randomVoltage.hpp
//
//  Created by Jorge Gutierrez-Rave Olmos / NANO Modules on 06/12/22.
//

#ifndef randomVoltage_hpp
#define randomVoltage_hpp

#include <stdint.h>
#include <math.h>
#include "../uiTools/trigger.hpp"
#include "timer.hpp"
// #include "digital.hpp"

// /** Rescales `x` from the range `[xMin, xMax]` to `[yMin, yMax]`. */
// inline float rescale(float x, float xMin, float xMax, float yMin, float yMax) {
// 	return yMin + (x - xMin) / (xMax - xMin) * (yMax - yMin);
// }

// /** Linearly interpolates between `a` and `b`, from `p = 0` to `p = 1`. */
// inline float crossfade(float a, float b, float p) {
// 	return a + (b - a) * p;
// }

/** Get a random value between `low` and `high`. */
float getRandomRange(float low, float high) {
    float randomNumber = sinf(rand() * rand());
    return low + (high - low) * fabs(randomNumber);
}

class RandomVoltage {
public:
  RandomVoltage() {}

    void init(void) {
        mExternalSample 	 = false;
        mExternallyTriggered = false;
        mSampleTriggered 	 = false;

        mTrigger        = 0.0f;
        mTriggerLenght  = 0.0008f;
        mVoltageSample  = 0.0f;

        mRate           = 0.5f;
        mProb           = 0.5f;
        mSpread         = 0.5f;
        mShape          = 0.5f;
        mOffset         = 0.0f;

        mLastVoltage    = 0.0f;
        mNextVoltage    = 1.0f;
        mPhase          = 0.01f;
        mSampleRate     = 1.0f / 44100.0f;
        mFrequency      = 10.f;

        mClockPhase     = 0.01f;
        mClockFreq      = 10.0f;
        mDeltaPhase     = 1.0f / 44100.0f;

        mRateLight 		= 0.0f;
    }

    void setSampleOrigin(bool external){
        mExternalSample = external;
    }

    void setRate(float rate){
        mRate = rate;
    }

    void setProbability(float prob){
        mProb = prob;
    }

    void setSpread(float spread){
        mSpread = spread;
    }

    void setShape(float shape){
        mShape = shape;
    }

    void setOffset(float offset){
        mOffset = offset;
    }

    void setTrigger(float trigger){
    	mTrigger = trigger;
    }

    void setTriggerSource(bool externallyTriggered){
        mExternallyTriggered = externallyTriggered;
    }

    void setVoltageSample(float voltageSample){
        mVoltageSample = voltageSample;
    }

    void setSampleRate(float sampleRate){
        mSampleRate = sampleRate;
    }

    void process(void){
        auto triggerSample = [&]() {
            mRateLight = 5.0f;
            // Do the process if the trigger input is active
            // Probabilistic trigger
			if (mProb < 1.f && getRandomRange(0.0f, 1.0f) > mProb){
				mSampleTriggered = false;
				return;
			}

            // Generate next random voltage
			mLastVoltage = mNextVoltage;

            if (mExternalSample) {
            	mSampleTriggered = true;
				mNextVoltage = mVoltageSample;
			}
			else {
				mSampleTriggered = true;
				// Crossfade new random voltage with last
				float v = 10.f * getRandomRange(0.0f, 1.0f) - mOffset;
                mNextVoltage = crossfade(mNextVoltage, v, mSpread);
			}
            mPhase = 0.0f;
            // Generate trigger output
            trigGenerator.trigger(mTriggerLenght);
        };

        if(mExternallyTriggered){
            // Calculate period between triggers
            periodCounter.process(mSampleRate);
            	// If we are triggered...
            if (inTrigger.process(mTrigger, 1.f, 3.f)) {
                mFrequency = 1.f / periodCounter.getTime();
                periodCounter.reset();
                triggerSample();
		    }

            mDeltaPhase = fmin(mFrequency * mSampleRate, 0.5f);

        } else {
            // Advance clock phase by rate
            float rate = mRate;
			mClockFreq = powf(2.f, rate);
			mDeltaPhase = fmin(mClockFreq * mSampleRate, 0.5f);
			mClockPhase += mDeltaPhase;
			// Trigger
			if (mClockPhase >= 1.f) {
				mClockPhase -= 1.f;
				triggerSample();
			}
        }

        // Advance phase
		mPhase += mDeltaPhase;
		mPhase = fmin(1.f, mPhase);
    }

    float getSteppedOut(void){
        float steps = ceilf(powf(mShape, 2) * 15 + 1);
		float v = ceilf(mPhase * steps) / steps;
		v = rescale(v, 0.f, 1.f, mLastVoltage, mNextVoltage);
        return v;
    }

    float getLinearOut(void){
        float slope = 1 / mShape;
		float v;
		if (slope < 1e6f) {
			v = fmin(mPhase * slope, 1.f);
		}
		else {
			v = 1.f;
		}
		v = rescale(v, 0.f, 1.f, mLastVoltage, mNextVoltage);
        return v;
    }

    float getExponentialOut(void){
        float b = powf(mShape, 8);
		float v;
		if (0.999f < b) {
			v = mPhase;
		}
		else if (1e-20f < b) {
			v = (powf(b, mPhase) - 1.f) / (b - 1.f);
		}
		else {
			v = 1.f;
		}
		v = rescale(v, 0.f, 1.f, mLastVoltage, mNextVoltage);
        return v;
    }

    float getSmoothOut(void){
        float p = 1 / mShape;
		float v;
		if (p < 1e6f) {
			v = fmin(mPhase * p, 1.f);
			v = cosf(M_PI * v);
		}
		else {
			v = -1.f;
		}
		v = rescale(v, 1.f, -1.f, mLastVoltage, mNextVoltage);
        return v;
    }

    float getTriggerOut(void){
        mRateLight = 0.0f;
        return trigGenerator.process(mSampleRate) * 5.0f;
    	return 0;
    }

    float getLastVoltage(void){
    	return mLastVoltage;
    }

    float getNextVoltage(void){
    	return mNextVoltage;
    }

    float getRateLight(void){
    	return mRateLight;
    }

    float getProbLight(void){
    	return mSampleTriggered * mRateLight;
    }

private:
    Timer periodCounter;
    TriggerHandler trig;
    rack::dsp::SchmittTrigger inTrigger;
    rack::dsp::PulseGenerator trigGenerator;

    bool mExternalSample;
    bool mExternallyTriggered;
    bool mSampleTriggered;

    float mTrigger;
    float mTriggerLenght;
    float mVoltageSample;

    float mRate;
    float mProb;
    float mSpread;
    float mShape;
    float mOffset;

    float mLastVoltage;
	float mNextVoltage;
	float mPhase;
    float mSampleRate;
    float mFrequency;

    float mClockPhase;
    float mClockFreq;
    float mDeltaPhase;

    float mRateLight;
};

#endif /* randomVoltage_hpp */