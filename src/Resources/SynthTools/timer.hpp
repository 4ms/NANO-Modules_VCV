//
//  timer.hpp
//
//  Created by Jorge Gutierrez-Rave Olmos / NANO Modules on 06/12/22.
//

#ifndef timer_hpp
#define timer_hpp

#include <stdint.h>

class Timer {
public:
  Timer() {}

    void init(void) {
        mTime = 0.0f;
    }

    float process(float deltaTime){
        mTime += deltaTime;
        return mTime;
    }

    float getTime(void){
        return mTime;
    }

    void reset(void){
        mTime = 0.0f;
    }

private:
    float mTime;
};

#endif /* timer_hpp */