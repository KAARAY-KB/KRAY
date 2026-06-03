#ifndef AUDIO_SPECTRUM_H
#define AUDIO_SPECTRUM_H

#include <vector>
#include <cstdint>
/*
frequency band          frequency range(Hz)                         Descriptions 
    Bass                    20 - 250                        包含低音基频和泛音，是音乐的基础和厚度感来源 
    Low-Mid Tom             250 - 500                       人声和多数乐器的基频集中区，影响声音的"温暖感" 
    Mid Tom                 500 - 2k                        人耳最敏感的区域，人声核心频段，影响清晰度 
    Hi-Mid Tom              2k - 4k                         人耳感知最强烈的区域，影响声音的"存在感"和"穿透力" 
    Treble                  4k - 20k                        包含泛音和细节，影响声音的亮度、空气感和空间感  
*/

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
