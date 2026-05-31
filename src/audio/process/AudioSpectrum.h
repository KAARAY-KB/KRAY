#ifndef AUDIO_SPECTRUM_H
#define AUDIO_SPECTRUM_H

#include <vector>
#include <cstdint>

// 频谱分析：FFT + 对数分组 + 加权曲线 + 方向控制
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

    // 设置加权曲线：weights[i] 对应第 i 个频谱柱的权重
    // 低频压、高频提，让律动更有节奏感
    // 传入空向量则使用默认加权
    void set_weights(const std::vector<float> &weights);

    // 设置方向控制：dirs[i] = 1.0 正向, -1.0 反向
    // 用于镜像等效果
    // 传入空向量则全部正向
    void set_dirs(const std::vector<float> &dirs);

    // 对原始PCM数据做FFT + 对数分组 + 加权 + 方向，返回频谱数据
    // sample_rate: 采样率，用于将FFT bin映射到真实频率
    std::vector<float> process(const float *samples, uint32_t frame_count, uint32_t sample_rate);

private:
    // 生成默认加权曲线（低频弱、高频强）
    void update_default_weights();

    uint32_t m_bar_count;               // 频谱柱子数量
    uint32_t m_fft_size;                // FFT 大小
    std::vector<float> m_weights;       // 加权曲线
    std::vector<float> m_dirs;          // 方向控制
};

#endif // AUDIO_SPECTRUM_H
