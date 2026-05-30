#define MINIAUDIO_IMPLEMENTATION
#include "AudioCore.h"
#include <cmath>
#include <algorithm>
#include <cstring>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// ==================== 构造/析构 ====================

// 构造函数
AudioCore::AudioCore()
    : m_ma_ctx_init(false)
    , m_ma_dev_init(false)
    , m_running(false)
    , m_waveform_points(512)
    , m_bar_count(16)
    , m_fft_size(1024)
    , m_sample_rate(44100)
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

// ==================== 属性 getter/setter ====================

// 获取波形数据点数
uint32_t AudioCore::get_waveform_points() const
{
    return m_waveform_points;
}
// 设置波形数据点数
void AudioCore::set_waveform_points(uint32_t points)
{
    m_waveform_points = points;
}

// 获取频谱柱子数量
uint32_t AudioCore::get_bar_count() const
{
    return m_bar_count;
}
// 设置频谱柱子数量
void AudioCore::set_bar_count(uint32_t count)
{
    m_bar_count = count;
}

// ==================== 设备枚举 ====================

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

// ==================== 设备设置 ====================

// 设置采集设备 ID 和方向
void AudioCore::set_device(const std::string &device_id, AudioDevDir dir)
{
    m_device_id = device_id;
    m_dev_dir = dir;
}

// ==================== 启动/停止 ====================

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
    config.capture.format = ma_format_f32;
    config.capture.channels = 1;
    config.sampleRate = m_sample_rate;
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

// ==================== 回调设置 ====================

// 设置频谱回调
void AudioCore::on_spectrum(SpectrumCb cb)
{
    std::lock_guard<std::mutex> lock(m_cb_mutex);
    m_spectrum_cb = cb;
}

// 设置波形回调
void AudioCore::on_waveform(WaveformCb cb)
{
    std::lock_guard<std::mutex> lock(m_cb_mutex);
    m_waveform_cb = cb;
}

// 设置能量回调
void AudioCore::on_energy(EnergyCb cb)
{
    std::lock_guard<std::mutex> lock(m_cb_mutex);
    m_energy_cb = cb;
}

// 设置错误回调
void AudioCore::on_error(ErrorCb cb)
{
    std::lock_guard<std::mutex> lock(m_cb_mutex);
    m_error_cb = cb;
}

// ==================== miniaudio 回调 ====================

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
    process_audio(samples, frame_count);
}

// ==================== 音频处理 ====================

// 处理音频数据：FFT + 波形 + 能量
void AudioCore::process_audio(const float *samples, uint32_t frame_count)
{
    // ---- 波形数据 ----
    uint32_t wave_points = m_waveform_points;
    std::vector<float> waveform(wave_points);
    for (uint32_t i = 0; i < wave_points; i++) {
        uint32_t idx = static_cast<uint32_t>(static_cast<float>(i) / wave_points * frame_count);
        if (idx >= frame_count) idx = frame_count - 1;
        waveform[i] = samples[idx];
    }

    {
        std::lock_guard<std::mutex> lock(m_cb_mutex);
        if (m_waveform_cb) m_waveform_cb(waveform.data(), wave_points);
    }

    // ---- FFT 频谱数据 ----
    uint32_t fft_n = m_fft_size;
    std::vector<float> real(fft_n, 0.0f);
    std::vector<float> imag(fft_n, 0.0f);

    uint32_t copy_len = frame_count < fft_n ? frame_count : fft_n;
    for (uint32_t i = 0; i < copy_len; i++) {
        float window = 0.5f * (1.0f - cosf(2.0f * static_cast<float>(M_PI) * i / (copy_len - 1)));
        real[i] = samples[i] * window;
    }

    fft(real, imag);

    uint32_t bar_count = m_bar_count;
    std::vector<float> spectrum(bar_count);
    uint32_t half_n = fft_n / 2;

    float log_min = log2f(2.0f);
    float log_max = log2f(static_cast<float>(half_n));
    uint32_t prev_hi = 2;
    for (uint32_t i = 0; i < bar_count; i++) {
        float lo = powf(2.0f, log_min + (log_max - log_min) * i / bar_count);
        float hi = powf(2.0f, log_min + (log_max - log_min) * (i + 1) / bar_count);
        uint32_t bin_lo = static_cast<uint32_t>(lo);
        uint32_t bin_hi = static_cast<uint32_t>(hi);
        if (bin_lo < prev_hi) bin_lo = prev_hi;
        if (bin_hi <= bin_lo) bin_hi = bin_lo + 1;
        if (bin_hi > half_n) bin_hi = half_n;
        prev_hi = bin_hi;
        int count = bin_hi - bin_lo;
        if (count <= 0) count = 1;
        float mag_sum = 0.0f;
        for (uint32_t j = bin_lo; j < bin_hi; j++) {
            float mag = sqrtf(real[j] * real[j] + imag[j] * imag[j]);
            mag_sum += mag;
        }
        spectrum[i] = mag_sum / count / (fft_n / 2);
    }

    {
        std::lock_guard<std::mutex> lock(m_cb_mutex);
        if (m_spectrum_cb) m_spectrum_cb(spectrum.data(), bar_count);
    }

    // ---- 总能量 ----
    float energy = 0.0f;
    for (uint32_t i = 0; i < frame_count; i++) {
        energy += samples[i] * samples[i];
    }
    energy = sqrtf(energy / frame_count);
    if (energy > 1.0f) energy = 1.0f;

    {
        std::lock_guard<std::mutex> lock(m_cb_mutex);
        if (m_energy_cb) m_energy_cb(energy);
    }
}

// ==================== FFT ====================

// 基2 Cooley-Tukey FFT
void AudioCore::fft(std::vector<float> &real, std::vector<float> &imag)
{
    uint32_t n = static_cast<uint32_t>(real.size());

    for (int i = 1, j = 0; i < n; i++) {
        int bit = n >> 1;
        while (j & bit) {
            j ^= bit;
            bit >>= 1;
        }
        j ^= bit;
        if (i < j) {
            std::swap(real[i], real[j]);
            std::swap(imag[i], imag[j]);
        }
    }

    for (uint32_t len = 2; len <= n; len <<= 1) {
        float angle = -2.0f * static_cast<float>(M_PI) / len;
        float w_real = cosf(angle);
        float w_imag = sinf(angle);

        for (uint32_t i = 0; i < n; i += len) {
            float cur_real = 1.0f;
            float cur_imag = 0.0f;

            for (uint32_t j = 0; j < len / 2; j++) {
                uint32_t even = i + j;
                uint32_t odd = i + j + len / 2;

                float t_real = cur_real * real[odd] - cur_imag * imag[odd];
                float t_imag = cur_real * imag[odd] + cur_imag * real[odd];

                real[odd] = real[even] - t_real;
                imag[odd] = imag[even] - t_imag;
                real[even] += t_real;
                imag[even] += t_imag;

                float new_real = cur_real * w_real - cur_imag * w_imag;
                float new_imag = cur_real * w_imag + cur_imag * w_real;
                cur_real = new_real;
                cur_imag = new_imag;
            }
        }
    }
}
