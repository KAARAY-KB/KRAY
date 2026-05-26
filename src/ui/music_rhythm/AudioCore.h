#ifndef AUDIO_CORE_H
#define AUDIO_CORE_H

#include <vector>
#include <string>
#include <functional>
#include <thread>
#include <atomic>

#ifdef _WIN32
#include <windows.h>
#include <mmdeviceapi.h>
#include <audioclient.h>
#elif defined(__linux__)
#include <pulse/pulseaudio.h>
#elif defined(__APPLE__)
#include <CoreAudio/CoreAudio.h>
#include <AudioUnit/AudioUnit.h>
#endif

// 音频设备方向
enum AudioDevDir {
    AUDIO_DEV_OUT = 0,  // 输出设备（扬声器），loopback 采集
    AUDIO_DEV_IN  = 1,  // 输入设备（麦克风），直接采集
};

// 音频设备信息
struct AudioDevInfo {
    std::string id;     // 设备 ID
    std::string name;   // 设备名称
    AudioDevDir dir;    // 设备方向
};

// 频谱数据回调：数据指针 + 长度
using SpectrumCb = std::function<void(const float*, int)>;
// 波形数据回调：数据指针 + 长度
using WaveformCb = std::function<void(const float*, int)>;
// 能量数据回调
using EnergyCb = std::function<void(float)>;
// 错误信息回调
using ErrorCb = std::function<void(const char*)>;

// 纯 C++ 音频采集核心，不依赖 Qt
// Windows: WASAPI loopback/capture
// Linux:   PulseAudio 监听/采集
// macOS:   CoreAudio AUHAL 采集
// 通过回调函数输出频谱、波形、能量数据
class AudioCore
{
public:
    // 构造函数
    AudioCore();
    // 析构函数
    ~AudioCore();

    // 枚举指定方向的音频设备
    static std::vector<AudioDevInfo> enum_devices(AudioDevDir dir);
    // 设置采集设备 ID 和方向
    void set_device(const std::string &device_id, AudioDevDir dir);
    // 启动采集
    bool start();
    // 停止采集
    void stop();
    // 是否正在采集
    bool is_running() const;

    // 设置频谱回调
    void on_spectrum(SpectrumCb cb);
    // 设置波形回调
    void on_waveform(WaveformCb cb);
    // 设置能量回调
    void on_energy(EnergyCb cb);
    // 设置错误回调
    void on_error(ErrorCb cb);
    // 获取波形数据点数
    uint32_t get_waveform_points() const;
    // 设置波形数据点数
    void set_waveform_points(uint32_t points);
    // 获取频谱柱子数量
    uint32_t get_bar_count() const;
    // 设置频谱柱子数量
    void set_bar_count(uint32_t count);

private:
    // 采集线程主循环
    void capture_loop();
    // 处理音频数据：FFT + 波形 + 能量
    void process_audio(const float *samples, uint32_t frame_count);
    // 基2 Cooley-Tukey FFT
    void fft(std::vector<float> &real, std::vector<float> &imag);
    // 将采集数据转为 float 格式
    void convert_to_float(uint8_t *data, uint32_t frames,
                          uint32_t channels, uint32_t bits,
                          std::vector<float> &out);

    std::thread m_thread;           // 采集线程
    std::atomic<bool> m_running;    // 运行标志
    uint32_t m_waveform_points;          // 波形数据点数
    uint32_t m_bar_count;                // 频谱柱子数量
    uint32_t m_fft_size;                 // FFT 大小
    uint32_t m_sample_rate;              // 采样率
    std::string m_device_id;        // 指定设备 ID
    AudioDevDir m_dev_dir;          // 设备方向

    SpectrumCb m_spectrum_cb;       // 频谱回调
    WaveformCb m_waveform_cb;       // 波形回调
    EnergyCb m_energy_cb;           // 能量回调
    ErrorCb m_error_cb;             // 错误回调

#ifdef __linux__
    // PulseAudio 上下文和流
    pa_mainloop *m_pa_loop;         // PulseAudio 主循环
    pa_context *m_pa_ctx;           // PulseAudio 上下文
    pa_stream *m_pa_stream;         // PulseAudio 流
    std::vector<float> m_pa_buf;    // PulseAudio 采集缓冲
#endif

#ifdef __APPLE__
    // macOS AudioUnit
    AudioUnit m_au;                 // 音频单元
    std::vector<float> m_au_buf;    // AudioUnit 采集缓冲
    static OSStatus au_callback(void *inRefCon,
                                AudioUnitRenderActionFlags *ioActionFlags,
                                const AudioTimeStamp *inTimeStamp,
                                UInt32 inBusNumber,
                                UInt32 inNumberFrames,
                                AudioBufferList *ioData);
#endif
};

#endif // AUDIO_CORE_H
