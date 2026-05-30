#define MINIAUDIO_IMPLEMENTATION
#include "AudioCore.h"
#include <cstring>

// 构造函数
AudioCore::AudioCore()
    : m_ma_ctx_init(false)
    , m_ma_dev_init(false)
    , m_running(false)
    , m_channels(1)
    , m_sample_fmt(ma_format_f32)
    , m_sample_rate(0)
    , m_req_sample_rate(ma_standard_sample_rate_48000)
    , m_dev_dir(AUDIO_DEV_OUT)
{
    std::memset(&m_ma_ctx, 0, sizeof(m_ma_ctx));
    std::memset(&m_ma_dev, 0, sizeof(m_ma_dev));
}

// 析构函数
AudioCore::~AudioCore()
{
    stop();
}

// 获取采样率
uint32_t AudioCore::get_sample_rate() const
{
    return m_sample_rate;
}

// 设置请求采样率（仅在启动前有效）
void AudioCore::set_sample_rate(uint32_t rate)
{
    m_req_sample_rate = rate;
}

// 枚举指定方向的音频设备
std::vector<AudioDevInfo> AudioCore::enum_devices(AudioDevDir dir)
{
    std::vector<AudioDevInfo> devices;

    ma_context ctx;
    if (ma_context_init(nullptr, 0, nullptr, &ctx) != MA_SUCCESS) {
        return devices;
    }

    ma_device_info *p_infos = nullptr;
    ma_uint32 count = 0;

    ma_result result;
    if (dir == AUDIO_DEV_OUT) {
        // 输出设备（loopback 模式需要枚举 playback 设备）
        result = ma_context_get_devices(&ctx, &p_infos, &count, nullptr, nullptr);
    } else {
        // 输入设备
        result = ma_context_get_devices(&ctx, nullptr, nullptr, &p_infos, &count);
    }

    if (result == MA_SUCCESS && p_infos != nullptr) {
        for (ma_uint32 i = 0; i < count; i++) {
            AudioDevInfo info;
            // 将 ma_device_id 序列化为字符串作为设备 ID
            // sizeof(ma_device_id) 在不同平台不同，统一用十六进制编码
            const uint8_t *raw = reinterpret_cast<const uint8_t *>(&p_infos[i].id);
            char buf[sizeof(ma_device_id) * 2 + 1] = {0};
            for (size_t b = 0; b < sizeof(ma_device_id); b++) {
                sprintf(buf + b * 2, "%02x", raw[b]);
            }
            info.id = buf;
            info.name = p_infos[i].name;
            info.dir = dir;
            devices.push_back(info);
        }
    }

    ma_context_uninit(&ctx);
    return devices;
}

// 设置采集设备 ID 和方向
void AudioCore::set_device(const std::string &device_id, AudioDevDir dir)
{
    m_device_id = device_id;
    m_dev_dir = dir;
}

// 启动采集
bool AudioCore::start()
{
    if (m_running) return true;

    // 初始化 miniaudio 上下文
    if (!m_ma_ctx_init) {
        if (ma_context_init(nullptr, 0, nullptr, &m_ma_ctx) != MA_SUCCESS) {
            if (m_error_cb) m_error_cb("miniaudio context init failed");
            return false;
        }
        m_ma_ctx_init = true;
    }

    // 配置设备
    ma_device_type dev_type;
    if (m_dev_dir == AUDIO_DEV_OUT) {
        // 输出设备：用 loopback 模式录制扬声器
        dev_type = ma_device_type_loopback;
    } else {
        // 输入设备：用 capture 模式录制麦克风
        dev_type = ma_device_type_capture;
    }

    ma_device_config config = ma_device_config_init(dev_type);
    config.capture.format = m_sample_fmt;
    config.capture.channels = m_channels;
    config.sampleRate = m_req_sample_rate;
    config.dataCallback = ma_data_callback;
    config.pUserData = this;

    // 设置指定设备 ID
    if (!m_device_id.empty()) {
        // 从序列化字符串反序列化 ma_device_id
        ma_device_info *p_infos = nullptr;
        ma_uint32 count = 0;
        ma_result result;

        if (m_dev_dir == AUDIO_DEV_OUT) {
            result = ma_context_get_devices(&m_ma_ctx, &p_infos, &count, nullptr, nullptr);
        } else {
            result = ma_context_get_devices(&m_ma_ctx, nullptr, nullptr, &p_infos, &count);
        }

        if (result == MA_SUCCESS && p_infos != nullptr) {
            bool found = false;
            for (ma_uint32 i = 0; i < count; i++) {
                const uint8_t *raw = reinterpret_cast<const uint8_t *>(&p_infos[i].id);
                char buf[sizeof(ma_device_id) * 2 + 1] = {0};
                for (size_t b = 0; b < sizeof(ma_device_id); b++) {
                    sprintf(buf + b * 2, "%02x", raw[b]);
                }
                if (m_device_id == buf) {
                    config.capture.pDeviceID = &p_infos[i].id;
                    found = true;
                    break;
                }
            }
            if (!found) {
                // 未找到指定设备，使用默认设备
                if (m_error_cb) m_error_cb("Device not found, using default");
            }
        }
    }

    // 初始化设备
    if (ma_device_init(&m_ma_ctx, &config, &m_ma_dev) != MA_SUCCESS) {
        if (m_error_cb) m_error_cb("miniaudio device init failed");
        return false;
    }
    m_ma_dev_init = true;

    // 启动设备
    if (ma_device_start(&m_ma_dev) != MA_SUCCESS) {
        if (m_error_cb) m_error_cb("miniaudio device start failed");
        ma_device_uninit(&m_ma_dev);
        m_ma_dev_init = false;
        return false;
    }

    m_running = true;

    // 读取设备实际采样率（可能与请求值不同）
    m_sample_rate = m_ma_dev.sampleRate;

    return true;
}

// 停止采集
void AudioCore::stop()
{
    m_running = false;

    if (m_ma_dev_init) {
        ma_device_stop(&m_ma_dev);
        ma_device_uninit(&m_ma_dev);
        m_ma_dev_init = false;
        std::memset(&m_ma_dev, 0, sizeof(m_ma_dev));
    }

    if (m_ma_ctx_init) {
        ma_context_uninit(&m_ma_ctx);
        m_ma_ctx_init = false;
        std::memset(&m_ma_ctx, 0, sizeof(m_ma_ctx));
    }
}

// 是否正在采集
bool AudioCore::is_running() const
{
    return m_running;
}

// 设置原始PCM数据回调
void AudioCore::on_pcm_data(PcmDataCb cb)
{
    std::lock_guard<std::mutex> lock(m_cb_mutex);
    m_pcm_cb = cb;
}

// 设置错误回调
void AudioCore::on_error(CaptureErrorCb cb)
{
    std::lock_guard<std::mutex> lock(m_cb_mutex);
    m_error_cb = cb;
}

// miniaudio 数据回调（静态，转发到成员方法）
void AudioCore::ma_data_callback(ma_device *pDevice, void *pOutput,
                                          const void *pInput, ma_uint32 frameCount)
{
    (void)pOutput;
    auto *self = static_cast<AudioCore *>(pDevice->pUserData);
    if (!self->m_running) return;

    const float *samples = static_cast<const float *>(pInput);
    if (samples == nullptr) return;

    self->on_ma_data(samples, frameCount);
}

// miniaudio 数据回调处理
void AudioCore::on_ma_data(const float *samples, ma_uint32 frame_count)
{
    std::lock_guard<std::mutex> lock(m_cb_mutex);
    if (m_pcm_cb) m_pcm_cb(samples, frame_count, m_sample_rate);
}
