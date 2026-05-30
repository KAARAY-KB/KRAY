#include "AudioProcess.h"

// 构造函数
AudioProcess::AudioProcess()
{
}

// 析构函数
AudioProcess::~AudioProcess()
{
}

// 输入原始PCM数据
void AudioProcess::feed(const float *samples, uint32_t frame_count, uint32_t sample_rate)
{
    // 波形处理
    if (m_waveform_cb) {
        std::vector<float> wave = m_waveform.process(samples, frame_count);
        std::lock_guard<std::mutex> lock(m_cb_mutex);
        m_waveform_cb(wave.data(), static_cast<int>(wave.size()));
    }

    // 频谱处理
    if (m_spectrum_cb) {
        std::vector<float> spec = m_spectrum.process(samples, frame_count, sample_rate);
        std::lock_guard<std::mutex> lock(m_cb_mutex);
        m_spectrum_cb(spec.data(), static_cast<int>(spec.size()));
    }

    // 能量处理
    if (m_energy_cb) {
        float energy = AudioEnergy::process(samples, frame_count);
        std::lock_guard<std::mutex> lock(m_cb_mutex);
        m_energy_cb(energy);
    }
}

// 获取波形数据点数
uint32_t AudioProcess::get_waveform_points() const
{
    return m_waveform.get_points();
}

// 设置波形数据点数
void AudioProcess::set_waveform_points(uint32_t points)
{
    m_waveform.set_points(points);
}

// 获取频谱柱子数量
uint32_t AudioProcess::get_bar_count() const
{
    return m_spectrum.get_bar_count();
}

// 设置频谱柱子数量
void AudioProcess::set_bar_count(uint32_t count)
{
    m_spectrum.set_bar_count(count);
}

// 获取FFT大小
uint32_t AudioProcess::get_fft_size() const
{
    return m_spectrum.get_fft_size();
}

// 设置FFT大小
void AudioProcess::set_fft_size(uint32_t size)
{
    m_spectrum.set_fft_size(size);
}

// 设置频谱回调
void AudioProcess::on_spectrum(SpectrumCb cb)
{
    std::lock_guard<std::mutex> lock(m_cb_mutex);
    m_spectrum_cb = cb;
}

// 设置波形回调
void AudioProcess::on_waveform(WaveformCb cb)
{
    std::lock_guard<std::mutex> lock(m_cb_mutex);
    m_waveform_cb = cb;
}

// 设置能量回调
void AudioProcess::on_energy(EnergyCb cb)
{
    std::lock_guard<std::mutex> lock(m_cb_mutex);
    m_energy_cb = cb;
}
