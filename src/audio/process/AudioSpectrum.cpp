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

// FFT + 对数分组
std::vector<float> AudioSpectrum::process(const float *samples, uint32_t frame_count, uint32_t sample_rate)
{
    uint32_t fft_n = m_fft_size;
    std::vector<float> real(fft_n, 0.0f);
    std::vector<float> imag(fft_n, 0.0f);

    // 加窗 + 填充
    uint32_t copy_len = frame_count < fft_n ? frame_count : fft_n;
    for (uint32_t i = 0; i < copy_len; i++) {
        float window = 0.5f * (1.0f - cosf(2.0f * static_cast<float>(M_PI) * i / (copy_len - 1)));
        real[i] = samples[i] * window;
    }

    // FFT 变换
    AudioFft::transform(real, imag);

    // 对数分组：基于真实频率映射
    // 每个 bin 对应频率 = bin_index * sample_rate / fft_n
    // 频率范围：20Hz（人耳下限）~ sample_rate/2（奈奎斯特频率）
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
    }

    return spectrum;
}
