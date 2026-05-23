#include "AudioCore.h"
#include <cmath>
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// MinGW 可能缺少 PKEY_Device_FriendlyName 定义
#ifndef _PKEY_Device_FriendlyName
static const PROPERTYKEY _PKEY_Device_FriendlyName = {
    {0xa45c254e, 0xdf1c, 0x4efd, {0x80, 0x20, 0x67, 0xd1, 0x46, 0xa8, 0x50, 0xe0}},
    14
};
#define PKEY_Device_FriendlyName _PKEY_Device_FriendlyName
#endif

// 构造函数
AudioCore::AudioCore()
    : m_running(false)
    , m_waveform_points(512)
    , m_bar_count(16)
    , m_fft_size(1024)
    , m_sample_rate(44100)
    , m_dev_dir(AUDIO_DEV_OUT)
{
}

// 析构函数
AudioCore::~AudioCore()
{
    stop();
}

// 获取波形数据点数
int AudioCore::get_waveform_points() const
{
    return m_waveform_points;
}
// 设置波形数据点数
void AudioCore::set_waveform_points(int points)
{
    if (points > 0) m_waveform_points = points;
}

// 获取频谱柱子数量
int AudioCore::get_bar_count() const
{
    return m_bar_count;
}
// 设置频谱柱子数量
void AudioCore::set_bar_count(int count)
{
    if (count > 0) m_bar_count = count;
}

// 枚举指定方向的音频设备
std::vector<AudioDevInfo> AudioCore::enum_devices(AudioDevDir dir)
{
    std::vector<AudioDevInfo> devices;

#ifdef _WIN32
    HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    bool need_uninit = (hr == S_OK);

    IMMDeviceEnumerator *p_enumerator = nullptr;
    hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr,
                          CLSCTX_ALL, __uuidof(IMMDeviceEnumerator),
                          (void **)&p_enumerator);
    if (FAILED(hr)) {
        if (need_uninit) CoUninitialize();
        return devices;
    }

    // 根据方向选择 eRender 或 eCapture
    EDataFlow data_flow = (dir == AUDIO_DEV_OUT) ? eRender : eCapture;

    IMMDeviceCollection *p_collection = nullptr;
    hr = p_enumerator->EnumAudioEndpoints(data_flow, DEVICE_STATE_ACTIVE, &p_collection);
    p_enumerator->Release();
    if (FAILED(hr)) {
        if (need_uninit) CoUninitialize();
        return devices;
    }

    UINT count = 0;
    p_collection->GetCount(&count);

    for (UINT i = 0; i < count; i++) {
        IMMDevice *p_device = nullptr;
        hr = p_collection->Item(i, &p_device);
        if (FAILED(hr)) continue;

        // 获取设备 ID
        LPWSTR p_id = nullptr;
        hr = p_device->GetId(&p_id);
        if (FAILED(hr)) {
            p_device->Release();
            continue;
        }

        AudioDevInfo info;
        info.dir = dir;
        // 宽字符转 UTF-8
        int len = WideCharToMultiByte(CP_UTF8, 0, p_id, -1, nullptr, 0, nullptr, nullptr);
        if (len > 0) {
            info.id.resize(len - 1);
            WideCharToMultiByte(CP_UTF8, 0, p_id, -1, &info.id[0], len, nullptr, nullptr);
        }
        CoTaskMemFree(p_id);

        // 获取设备友好名称
        IPropertyStore *p_props = nullptr;
        hr = p_device->OpenPropertyStore(STGM_READ, &p_props);
        if (SUCCEEDED(hr)) {
            PROPVARIANT prop;
            PropVariantInit(&prop);
            hr = p_props->GetValue(PKEY_Device_FriendlyName, &prop);
            if (SUCCEEDED(hr) && prop.pwszVal != nullptr) {
                int name_len = WideCharToMultiByte(CP_UTF8, 0, prop.pwszVal, -1, nullptr, 0, nullptr, nullptr);
                if (name_len > 0) {
                    info.name.resize(name_len - 1);
                    WideCharToMultiByte(CP_UTF8, 0, prop.pwszVal, -1, &info.name[0], name_len, nullptr, nullptr);
                }
            }
            PropVariantClear(&prop);
            p_props->Release();
        }

        p_device->Release();
        devices.push_back(info);
    }

    p_collection->Release();
    if (need_uninit) CoUninitialize();
#endif

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

    m_running = true;
    m_thread = std::thread(&AudioCore::capture_loop, this);
    return true;
}

// 停止采集
void AudioCore::stop()
{
    m_running = false;
    if (m_thread.joinable()) {
        m_thread.join();
    }
}

// 是否正在采集
bool AudioCore::is_running() const
{
    return m_running;
}

// 设置频谱回调
void AudioCore::on_spectrum(SpectrumCb cb)
{
    m_spectrum_cb = cb;
}

// 设置波形回调
void AudioCore::on_waveform(WaveformCb cb)
{
    m_waveform_cb = cb;
}

// 设置能量回调
void AudioCore::on_energy(EnergyCb cb)
{
    m_energy_cb = cb;
}

// 设置错误回调
void AudioCore::on_error(ErrorCb cb)
{
    m_error_cb = cb;
}

// 采集线程主循环
void AudioCore::capture_loop()
{
#ifdef _WIN32
    HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    if (FAILED(hr)) {
        if (m_error_cb) m_error_cb("COM init failed");
        m_running = false;
        return;
    }

    // 获取设备枚举器
    IMMDeviceEnumerator *p_enumerator = nullptr;
    hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr,
                          CLSCTX_ALL, __uuidof(IMMDeviceEnumerator),
                          (void **)&p_enumerator);
    if (FAILED(hr)) {
        if (m_error_cb) m_error_cb("Cannot create device enumerator");
        CoUninitialize();
        m_running = false;
        return;
    }

    // 根据方向选择数据流和角色
    EDataFlow data_flow = (m_dev_dir == AUDIO_DEV_OUT) ? eRender : eCapture;
    ERole role = eConsole;

    // 按设备 ID 或默认获取设备
    IMMDevice *p_device = nullptr;
    if (!m_device_id.empty()) {
        int wlen = MultiByteToWideChar(CP_UTF8, 0, m_device_id.c_str(), -1, nullptr, 0);
        if (wlen > 0) {
            std::vector<wchar_t> wbuf(wlen);
            MultiByteToWideChar(CP_UTF8, 0, m_device_id.c_str(), -1, wbuf.data(), wlen);
            hr = p_enumerator->GetDevice(wbuf.data(), &p_device);
        }
        if (FAILED(hr) || p_device == nullptr) {
            p_enumerator->GetDefaultAudioEndpoint(data_flow, role, &p_device);
        }
    }
    if (p_device == nullptr) {
        hr = p_enumerator->GetDefaultAudioEndpoint(data_flow, role, &p_device);
    }
    p_enumerator->Release();
    if (FAILED(hr) || p_device == nullptr) {
        if (m_error_cb) m_error_cb("Cannot get audio device");
        CoUninitialize();
        m_running = false;
        return;
    }

    // 激活音频客户端
    IAudioClient *p_audio_client = nullptr;
    hr = p_device->Activate(__uuidof(IAudioClient), CLSCTX_ALL,
                            nullptr, (void **)&p_audio_client);
    p_device->Release();
    if (FAILED(hr)) {
        if (m_error_cb) m_error_cb("Cannot activate audio client");
        CoUninitialize();
        m_running = false;
        return;
    }

    // 获取设备混合格式
    WAVEFORMATEX *p_format = nullptr;
    hr = p_audio_client->GetMixFormat(&p_format);
    if (FAILED(hr)) {
        p_audio_client->Release();
        if (m_error_cb) m_error_cb("Cannot get audio format");
        CoUninitialize();
        m_running = false;
        return;
    }

    m_sample_rate = p_format->nSamplesPerSec;
    UINT32 channels = p_format->nChannels;
    UINT32 bits_per_sample = p_format->wBitsPerSample;

    // 初始化音频客户端
    // 输出设备用 loopback，输入设备用普通共享模式
    DWORD stream_flags = AUDCLNT_SHAREMODE_SHARED;
    if (m_dev_dir == AUDIO_DEV_OUT) {
        stream_flags |= AUDCLNT_STREAMFLAGS_LOOPBACK;
    }

    REFERENCE_TIME hns_duration = 10000000;
    hr = p_audio_client->Initialize(
        AUDCLNT_SHAREMODE_SHARED,
        stream_flags,
        hns_duration, 0, p_format, nullptr);
    if (FAILED(hr)) {
        CoTaskMemFree(p_format);
        p_audio_client->Release();
        if (m_error_cb) m_error_cb("Audio client init failed");
        CoUninitialize();
        m_running = false;
        return;
    }

    // 获取采集客户端
    IAudioCaptureClient *p_capture = nullptr;
    hr = p_audio_client->GetService(__uuidof(IAudioCaptureClient),
                                    (void **)&p_capture);
    if (FAILED(hr)) {
        CoTaskMemFree(p_format);
        p_audio_client->Release();
        if (m_error_cb) m_error_cb("Cannot get capture client");
        CoUninitialize();
        m_running = false;
        return;
    }

    // 开始采集
    hr = p_audio_client->Start();
    if (FAILED(hr)) {
        p_capture->Release();
        CoTaskMemFree(p_format);
        p_audio_client->Release();
        if (m_error_cb) m_error_cb("Cannot start capture");
        CoUninitialize();
        m_running = false;
        return;
    }

    // 采集循环
    UINT32 packet_len = 0;
    UINT32 buffer_frames = 0;
    BYTE *p_data = nullptr;
    DWORD flags = 0;

    while (m_running) {
        hr = p_capture->GetNextPacketSize(&packet_len);
        if (FAILED(hr)) break;

        if (packet_len == 0) {
            Sleep(1);
            continue;
        }

        hr = p_capture->GetBuffer(&p_data, &buffer_frames, &flags, nullptr, nullptr);
        if (FAILED(hr)) break;

        if (!(flags & AUDCLNT_BUFFERFLAGS_SILENT) && p_data != nullptr) {
            std::vector<float> samples;
            convert_to_float(p_data, buffer_frames, channels, bits_per_sample, samples);
            if (!samples.empty()) {
                process_audio(samples.data(), buffer_frames);
            }
        }

        p_capture->ReleaseBuffer(buffer_frames);
    }

    // 停止并释放
    p_audio_client->Stop();
    p_capture->Release();
    CoTaskMemFree(p_format);
    p_audio_client->Release();
    CoUninitialize();
#endif

    m_running = false;
}

// 将采集数据转为 float 格式（取左声道）
void AudioCore::convert_to_float(uint8_t *data, uint32_t frames,
                                  uint32_t channels, uint32_t bits,
                                  std::vector<float> &out)
{
    out.resize(frames);

    if (bits == 16) {
        int16_t *p = reinterpret_cast<int16_t *>(data);
        for (uint32_t i = 0; i < frames; i++) {
            out[i] = static_cast<float>(p[i * channels]) / 32768.0f;
        }
    } else if (bits == 32) {
        float *p = reinterpret_cast<float *>(data);
        for (uint32_t i = 0; i < frames; i++) {
            out[i] = p[i * channels];
        }
    } else if (bits == 24) {
        for (uint32_t i = 0; i < frames; i++) {
            uint32_t offset = i * channels * 3;
            int32_t val = static_cast<int32_t>(data[offset])
                        | (static_cast<int32_t>(data[offset + 1]) << 8)
                        | (static_cast<int32_t>(static_cast<int8_t>(data[offset + 2])) << 16);
            out[i] = static_cast<float>(val) / 8388608.0f;
        }
    } else {
        std::fill(out.begin(), out.end(), 0.0f);
    }
}

// 处理音频数据：FFT + 波形 + 能量
void AudioCore::process_audio(const float *samples, uint32_t frame_count)
{
    // ---- 波形数据 ----
    int wave_points = m_waveform_points;
    std::vector<float> waveform(wave_points);
    for (int i = 0; i < wave_points; i++) {
        int idx = static_cast<int>(static_cast<float>(i) / wave_points * frame_count);
        if (idx >= static_cast<int>(frame_count)) idx = frame_count - 1;
        waveform[i] = samples[idx];
    }
    if (m_waveform_cb) m_waveform_cb(waveform.data(), wave_points);

    // ---- FFT 频谱数据 ----
    int fft_n = m_fft_size;
    std::vector<float> real(fft_n, 0.0f);
    std::vector<float> imag(fft_n, 0.0f);

    int copy_len = static_cast<int>(frame_count) < fft_n ? static_cast<int>(frame_count) : fft_n;
    for (int i = 0; i < copy_len; i++) {
        float window = 0.5f * (1.0f - cosf(2.0f * static_cast<float>(M_PI) * i / (copy_len - 1)));
        real[i] = samples[i] * window;
    }

    fft(real, imag);

    int bar_count = m_bar_count;
    std::vector<float> spectrum(bar_count);
    int half_n = fft_n / 2;

    // 对数分组：低频柱子覆盖窄频段，高频柱子覆盖宽频段
    // 从 bin 2 开始（约 94Hz @48kHz），避免 DC 和极低频
    float log_min = log2f(2.0f);
    float log_max = log2f(static_cast<float>(half_n));
    int prev_hi = 2;
    for (int i = 0; i < bar_count; i++) {
        float lo = powf(2.0f, log_min + (log_max - log_min) * i / bar_count);
        float hi = powf(2.0f, log_min + (log_max - log_min) * (i + 1) / bar_count);
        int bin_lo = static_cast<int>(lo);
        int bin_hi = static_cast<int>(hi);
        // 确保不与前一柱重叠，每柱覆盖唯一 bin
        if (bin_lo < prev_hi) bin_lo = prev_hi;
        if (bin_hi <= bin_lo) bin_hi = bin_lo + 1;
        if (bin_hi > half_n) bin_hi = half_n;
        prev_hi = bin_hi;
        int count = bin_hi - bin_lo;
        if (count <= 0) count = 1;
        float mag_sum = 0.0f;
        for (int j = bin_lo; j < bin_hi; j++) {
            float mag = sqrtf(real[j] * real[j] + imag[j] * imag[j]);
            mag_sum += mag;
        }
        spectrum[i] = mag_sum / count / (fft_n / 2);
    }
    if (m_spectrum_cb) m_spectrum_cb(spectrum.data(), bar_count);

    // ---- 总能量 ----
    float energy = 0.0f;
    for (uint32_t i = 0; i < frame_count; i++) {
        energy += samples[i] * samples[i];
    }
    energy = sqrtf(energy / frame_count);
    if (energy > 1.0f) energy = 1.0f;
    if (m_energy_cb) m_energy_cb(energy);
}

// 基2 Cooley-Tukey FFT
void AudioCore::fft(std::vector<float> &real, std::vector<float> &imag)
{
    int n = static_cast<int>(real.size());

    // 位反转排列
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

    // 蝶形运算
    for (int len = 2; len <= n; len <<= 1) {
        float angle = -2.0f * static_cast<float>(M_PI) / len;
        float w_real = cosf(angle);
        float w_imag = sinf(angle);

        for (int i = 0; i < n; i += len) {
            float cur_real = 1.0f;
            float cur_imag = 0.0f;

            for (int j = 0; j < len / 2; j++) {
                int even = i + j;
                int odd = i + j + len / 2;

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
