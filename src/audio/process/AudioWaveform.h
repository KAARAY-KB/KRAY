#ifndef AUDIO_WAVEFORM_H
#define AUDIO_WAVEFORM_H

#include <vector>
#include <cstdint>

// 波形降采样：将原始PCM数据降采样到指定点数
class AudioWaveform
{
public:
    // 构造函数
    AudioWaveform();

    // 获取波形数据点数
    uint32_t get_points() const;
    // 设置波形数据点数
    void set_points(uint32_t points);

    // 对原始PCM数据降采样，返回波形数据
    std::vector<float> process(const float *samples, uint32_t frame_count);

private:
    uint32_t m_points;  // 波形数据点数
};

#endif // AUDIO_WAVEFORM_H
