#include "AudioWaveform.h"

// 构造函数
AudioWaveform::AudioWaveform()
    : m_points(512)
{
}

// 获取波形数据点数
uint32_t AudioWaveform::get_points() const
{
    return m_points;
}

// 设置波形数据点数
void AudioWaveform::set_points(uint32_t points)
{
    m_points = points;
}

// 对原始PCM数据降采样
std::vector<float> AudioWaveform::process(const float *samples, uint32_t frame_count)
{
    uint32_t wave_points = m_points;
    std::vector<float> waveform(wave_points);
    for (uint32_t i = 0; i < wave_points; i++) {
        uint32_t idx = static_cast<uint32_t>(static_cast<float>(i) / wave_points * frame_count);
        if (idx >= frame_count) idx = frame_count - 1;
        waveform[i] = samples[idx];
    }
    return waveform;
}
