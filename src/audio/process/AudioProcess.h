#ifndef AUDIO_PROCESS_H
#define AUDIO_PROCESS_H

#include <vector>
#include <functional>
#include <mutex>

#include "AudioWaveform.h"
#include "AudioSpectrum.h"
#include "AudioEnergy.h"

// 频谱数据回调：数据指针 + 长度
using SpectrumCb = std::function<void(const float*, int)>;
// 波形数据回调：数据指针 + 长度
using WaveformCb = std::function<void(const float*, int)>;
// 能量数据回调
using EnergyCb = std::function<void(float)>;

// 音频处理组合层：组合波形、频谱、能量三个处理模块
// 接收原始PCM数据，分发到各模块处理，通过回调输出结果
class AudioProcess
{
public:
    // 构造函数
    AudioProcess();
    // 析构函数
    ~AudioProcess();

    // 输入原始PCM数据（由采集层调用）
    void feed(const float *samples, uint32_t frame_count, uint32_t sample_rate);

    // 获取波形数据点数
    uint32_t get_waveform_points() const;
    // 设置波形数据点数
    void set_waveform_points(uint32_t points);
    // 获取频谱柱子数量
    uint32_t get_bar_count() const;
    // 设置频谱柱子数量
    void set_bar_count(uint32_t count);
    // 获取FFT大小
    uint32_t get_fft_size() const;
    // 设置FFT大小
    void set_fft_size(uint32_t size);

    // 设置频谱回调
    void on_spectrum(SpectrumCb cb);
    // 设置波形回调
    void on_waveform(WaveformCb cb);
    // 设置能量回调
    void on_energy(EnergyCb cb);

private:
    std::mutex m_cb_mutex;          // 回调互斥锁

    AudioWaveform m_waveform;       // 波形处理模块
    AudioSpectrum m_spectrum;       // 频谱处理模块
    AudioEnergy   m_energy;         // 能量处理模块（静态方法，无需实例）

    SpectrumCb m_spectrum_cb = nullptr;       // 频谱回调
    WaveformCb m_waveform_cb = nullptr;       // 波形回调
    EnergyCb   m_energy_cb = nullptr;         // 能量回调
};

#endif // AUDIO_PROCESS_H
