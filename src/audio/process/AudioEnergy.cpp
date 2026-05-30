#include "AudioEnergy.h"
#include <cmath>

// 计算RMS能量
float AudioEnergy::process(const float *samples, uint32_t frame_count)
{
    float energy = 0.0f;
    for (uint32_t i = 0; i < frame_count; i++) {
        energy += samples[i] * samples[i];
    }
    energy = sqrtf(energy / frame_count);
    if (energy > 1.0f) energy = 1.0f;
    return energy;
}
