#ifndef AUDIO_ENERGY_H
#define AUDIO_ENERGY_H

#include <cstdint>

// 能量计算：RMS 均方根
class AudioEnergy
{
public:
    // 计算PCM数据的RMS能量，返回0~1
    static float process(const float *samples, uint32_t frame_count);
};

#endif // AUDIO_ENERGY_H
