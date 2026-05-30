#ifndef AUDIO_SPECTRUM_H
#define AUDIO_SPECTRUM_H

#include <vector>
#include <cstdint>

// 频谱分析：FFT + 对数分组，输出频谱柱状图数据
class AudioSpectrum
{
public:
    // 构造函数
    AudioSpectrum();

    // 获取频谱柱子数量
    uint32_t get_bar_count() const;
    // 设置频谱柱子数量
    void set_bar_count(uint32_t count);
    // 获取FFT大小
    uint32_t get_fft_size() const;
    // 设置FFT大小
    void set_fft_size(uint32_t size);

    // 对原始PCM数据做FFT + 对数分组，返回频谱数据
    // sample_rate: 采样率，用于将FFT bin映射到真实频率
    std::vector<float> process(const float *samples, uint32_t frame_count, uint32_t sample_rate);

private:
    uint32_t m_bar_count;   // 频谱柱子数量
    uint32_t m_fft_size;    // FFT 大小
};

#endif // AUDIO_SPECTRUM_H
