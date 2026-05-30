#ifndef AUDIO_CORE_H
#define AUDIO_CORE_H

#include <cstdint>
#include <vector>
#include <string>
#include <functional>
#include <mutex>
#include <atomic>

#include "miniaudio.h"


// 音频设备方向
enum AudioDevDir {
    AUDIO_DEV_OUT = 0,  // 输出设备（扬声器），loopback 采集
    AUDIO_DEV_IN  = 1,  // 输入设备（麦克风），直接采集
};

// 音频设备信息
struct AudioDevInfo {
    std::string id;     // 设备 ID（序列化的 ma_device_id）
    std::string name;   // 设备名称
    AudioDevDir dir;    // 设备方向
};

// 原始PCM数据回调：采样数据指针 + 帧数 + 采样率
using PcmDataCb = std::function<void(const float*, uint32_t, uint32_t)>;
// 错误信息回调
using CaptureErrorCb = std::function<void(const char*)>;

// 音频采集层：设备枚举、miniaudio 管理、原始PCM数据输出
// 不做FFT/频谱/能量计算，只负责采集
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
    // 获取实际采样率（启动后为设备实际值，启动前为请求值）
    uint32_t get_sample_rate() const;
    // 设置请求采样率（仅在启动前有效，启动后由设备决定）
    void set_sample_rate(uint32_t rate);
    // 设置原始PCM数据回调
    void on_pcm_data(PcmDataCb cb);
    // 设置错误回调
    void on_error(CaptureErrorCb cb);

private:
    // miniaudio 数据回调（静态，转发到成员方法）
    static void ma_data_callback(ma_device *pDevice, void *pOutput,
                                  const void *pInput, ma_uint32 frameCount);
    // miniaudio 数据回调处理
    void on_ma_data(const float *samples, ma_uint32 frame_count);

    ma_context m_ma_ctx;            // miniaudio 上下文
    ma_device m_ma_dev;             // miniaudio 设备
    bool m_ma_ctx_init;             // 上下文是否已初始化
    bool m_ma_dev_init;             // 设备是否已初始化

    std::mutex m_cb_mutex;          // 回调互斥锁
    std::atomic<bool> m_running;    // 运行标志
    uint32_t m_sample_rate;         // 实际采样率（启动后由设备决定）
    uint32_t m_req_sample_rate;     // 请求采样率（启动前用户设置）
    uint8_t m_channels;             // 通道数
    ma_format m_sample_fmt;         // 采样格式
    std::string m_device_id;        // 指定设备 ID
    AudioDevDir m_dev_dir;          // 设备方向

    PcmDataCb m_pcm_cb = nullptr;            // 原始PCM数据回调
    CaptureErrorCb m_error_cb = nullptr;      // 错误回调
};

#endif // AUDIO_CORE_H
