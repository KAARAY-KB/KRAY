#include "AudioSpectrum.h"
#include "AudioFft.h"
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// 构造函数
AudioSpectrum::AudioSpectrum()
    : m_bar_count(16)
    , m_fft_size(1024)
{
    update_default_weights();
    m_dirs.resize(m_bar_count, 1.0f);
}

// 获取频谱柱子数量
uint32_t AudioSpectrum::get_bar_count() const
{
    return m_bar_count;
}

// 设置频谱柱子数量
void AudioSpectrum::set_bar_count(uint32_t count)
{
    m_bar_count = count;
    update_default_weights();
    m_dirs.resize(count, 1.0f);
}

// 获取FFT大小
uint32_t AudioSpectrum::get_fft_size() const
{
    return m_fft_size;
}

// 设置FFT大小
void AudioSpectrum::set_fft_size(uint32_t size)
{
    m_fft_size = size;
}

// 设置加权曲线
void AudioSpectrum::set_weights(const std::vector<float> &weights)
{
    if (weights.empty()) {
        update_default_weights();
    } else {
        m_weights = weights;
        m_weights.resize(m_bar_count, 1.0f);
    }
}

// 设置方向控制
void AudioSpectrum::set_dirs(const std::vector<float> &dirs)
{
    if (dirs.empty()) {
        m_dirs.assign(m_bar_count, 1.0f);
    } else {
        m_dirs = dirs;
        m_dirs.resize(m_bar_count, 1.0f);
    }
}

// 生成默认加权曲线（低频弱、高频强）
// 参考 examples 的分段加权思路，用连续曲线替代硬分段
void AudioSpectrum::update_default_weights()
{
    m_weights.resize(m_bar_count);
    for (uint32_t i = 0; i < m_bar_count; i++) {
        // 从 0.5 线性增长到 1.5，低频弱、高频强
        float t = static_cast<float>(i) / (m_bar_count - 1);
        // m_weights[i] = 0.5f + t * 1.0f;
        m_weights[i] = 1.0f;
    }
}

// FFT + 对数分组 + 加权 + 方向
std::vector<float> AudioSpectrum::process(const float *samples, uint32_t frame_count, uint32_t sample_rate)
{
    uint32_t fft_n = m_fft_size;
    // fft_n = sample_rate / 2;
    std::vector<float> real(fft_n, 0.0f);
    std::vector<float> imag(fft_n, 0.0f);

    // Hann 窗 + 填充
    uint32_t copy_len = frame_count < fft_n ? frame_count : fft_n;
    for (uint32_t i = 0; i < copy_len; i++) {
        float window = 0.5f * (1.0f - cosf(2.0f * static_cast<float>(M_PI) * i / (copy_len - 1)));
        real[i] = samples[i] * window;
    }

    // FFT 变换
    AudioFft::transform(real, imag);

    // 对数分组：基于真实频率映射
    uint32_t bar_count = m_bar_count;
    std::vector<float> spectrum(bar_count);
    uint32_t half_n = fft_n / 2;

    float freq_min = 20.0f;
    float freq_max = static_cast<float>(sample_rate) / 2.0f;
    float log_min = log2f(freq_min);
    float log_max = log2f(freq_max);

    uint32_t prev_bin_hi = 0;
    for (uint32_t i = 0; i < bar_count; i++) {
        // 对数频率划分
        float lo_freq = powf(2.0f, log_min + (log_max - log_min) * i / bar_count);
        float hi_freq = powf(2.0f, log_min + (log_max - log_min) * (i + 1) / bar_count);

        // 频率 → bin 索引
        uint32_t bin_lo = static_cast<uint32_t>(lo_freq * fft_n / sample_rate);
        uint32_t bin_hi = static_cast<uint32_t>(hi_freq * fft_n / sample_rate);

        // 确保单调递增
        if (bin_lo < prev_bin_hi) bin_lo = prev_bin_hi;
        if (bin_hi <= bin_lo) bin_hi = bin_lo + 1;
        if (bin_hi > half_n) bin_hi = half_n;
        prev_bin_hi = bin_hi;

        // 计算该频段的平均幅度
        int count = bin_hi - bin_lo;
        if (count <= 0) count = 1;
        float mag_sum = 0.0f;
        for (uint32_t j = bin_lo; j < bin_hi; j++) {
            float mag = sqrtf(real[j] * real[j] + imag[j] * imag[j]);
            mag_sum += mag;
        }
        spectrum[i] = mag_sum / count / (fft_n / 2);

        // 应用加权曲线
        spectrum[i] *= m_weights[i];

        // 应用方向控制
        spectrum[i] *= m_dirs[i];
    }

    return spectrum;
}
