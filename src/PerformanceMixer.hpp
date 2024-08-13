// PerformanceMixer.hpp

#ifndef PERFORMANCE_MIXER_HPP
#define PERFORMANCE_MIXER_HPP

#define MIXER_CHANNELS 4

struct SharedData {
    // Shared data between the module and expander 
    float shared_l_output[MIXER_CHANNELS] = {0.0f, 0.0f, 0.0f, 0.0f};
    float shared_r_output[MIXER_CHANNELS] = {0.0f, 0.0f, 0.0f, 0.0f};

    float shared_cv_aux[MIXER_CHANNELS] = {0.0f, 0.0f, 0.0f, 0.0f};

    bool shared_gateMuted[MIXER_CHANNELS] = {false, false, false, false};
    // You can add more fields as needed
};

#endif // PERFORMANCE_MIXER_HPP