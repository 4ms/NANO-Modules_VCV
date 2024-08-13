//
//  Filter.hpp
//
//  Created by Jorge Gutierrez-Rave Olmos / NANO Modules on 16/08/22.
//

#ifndef Filter_hpp
#define Filter_hpp

#include "math.hpp"

class Filter {
public:
  Filter() {}

void init(void){
    mInput = 0.0f;
    mCutoff = 0.0f;
    mResonance = 0.0f;
    mFirstStage = 0.0f;
    mSecondStage = 0.0f;
    mLowPassOut = 0.0f;
    mBandPassOut = 0.0f;
}

void process(void){
    mFirstStage += mCutoff * (mInput - mFirstStage + (mResonance * (tanh((mFirstStage - mSecondStage) / 10.0f))));

    mSecondStage += mCutoff * (mFirstStage - mSecondStage + 0.1f);
}

void setInput(float input){
    mInput = input;
}

void setCutoff(float cutoff){
    mCutoff = clamp(cutoff, 0.001f, 0.999f);
}

void setResonance(float resonance){
    mResonance = clamp(resonance, 0.001f, 0.999f) * 28.0f;

    //Resonance feeding
    if (resonance >= 20){
        mFirstStage += 0.001f;
    }
}

float getLowPass(void){
    mLowPassOut = tanh(mSecondStage / 10.0f) * 11.0f;
    return mLowPassOut;
}

float getBandPass(void){
    mBandPassOut = tanh((mFirstStage - mSecondStage) / 10.0f) * 11.0f;
    return mBandPassOut;
}


private:
    float mInput;
    float mCutoff;
    float mResonance;
    float mFirstStage;
    float mSecondStage;
    float mLowPassOut;
    float mBandPassOut;
};

#endif /* Filter_hpp */