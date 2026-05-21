#include "AudioCapture.h"
#include <QDebug>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// ============================================================================
// AudioCaptureThread
// ============================================================================

// 构造函数：初始化成员变量
AudioCaptureThread::AudioCaptureThread(QObject *parent)
    : QThread(parent)
    , m_running(false)
    , m_fft_size(1024)
    , m_sample_rate(44100)
{
}

// 析构函数：确保线程停止
AudioCaptureThread::~AudioCaptureThread()
{
    stop();
    wait();
}

// 停止采集
void AudioCaptureThread::stop()
{
    m_running = false;
}

// 线程入口：初始化 COM 并启动采集循环
void AudioCaptureThread::run()
{
    // 初始化 COM 为多线程公寓模式
    HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    if (FAILED(hr)) {
        emit sig_error(QString("COM 初始化失败: 0x%1").arg(hr, 8, 16, QChar('0')));
        return;
    }

    // 执行采集循环
    capture_loop();

    // 反初始化 COM
    CoUninitialize();
}

// WASAPI loopback 采集主循环
bool AudioCaptureThread::capture_loop()
{
    HRESULT hr;

    // 1. 获取设备枚举器
    IMMDeviceEnumerator *p_enumerator = nullptr;
    hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr,
                          CLSCTX_ALL, __uuidof(IMMDeviceEnumerator),
                          (void **)&p_enumerator);
    if (FAILED(hr)) {
        emit sig_error("无法创建设备枚举器");
        return false;
    }

    // 2. 获取默认音频渲染设备（扬声器）
    IMMDevice *p_device = nullptr;
    hr = p_enumerator->GetDefaultAudioEndpoint(eRender, eConsole, &p_device);
    p_enumerator->Release();
    if (FAILED(hr)) {
        emit sig_error("无法获取默认音频输出设备");
        return false;
    }

    // 3. 激活音频客户端
    IAudioClient *p_audio_client = nullptr;
    hr = p_device->Activate(__uuidof(IAudioClient), CLSCTX_ALL,
                            nullptr, (void **)&p_audio_client);
    p_device->Release();
    if (FAILED(hr)) {
        emit sig_error("无法激活音频客户端");
        return false;
    }

    // 4. 获取设备混合格式
    WAVEFORMATEX *p_format = nullptr;
    hr = p_audio_client->GetMixFormat(&p_format);
    if (FAILED(hr)) {
        p_audio_client->Release();
        emit sig_error("无法获取音频格式");
        return false;
    }

    // 记录采样率和通道数
    m_sample_rate = p_format->nSamplesPerSec;
    UINT32 channels = p_format->nChannels;
    UINT32 bits_per_sample = p_format->wBitsPerSample;

    // 5. 初始化音频客户端（loopback 模式）
    // REFTIMES_PER_SEC = 10000000 (1秒的100纳秒单位数)
    REFERENCE_TIME hns_duration = 10000000; // 缓冲区1秒
    hr = p_audio_client->Initialize(
        AUDCLNT_SHAREMODE_SHARED,
        AUDCLNT_STREAMFLAGS_LOOPBACK, // loopback 捕获扬声器输出
        hns_duration,
        0,
        p_format,
        nullptr);
    if (FAILED(hr)) {
        CoTaskMemFree(p_format);
        p_audio_client->Release();
        emit sig_error("音频客户端初始化失败");
        return false;
    }

    // 6. 获取采集客户端
    IAudioCaptureClient *p_capture = nullptr;
    hr = p_audio_client->GetService(__uuidof(IAudioCaptureClient),
                                    (void **)&p_capture);
    if (FAILED(hr)) {
        CoTaskMemFree(p_format);
        p_audio_client->Release();
        emit sig_error("无法获取采集客户端");
        return false;
    }

    // 7. 开始采集
    hr = p_audio_client->Start();
    if (FAILED(hr)) {
        p_capture->Release();
        CoTaskMemFree(p_format);
        p_audio_client->Release();
        emit sig_error("无法启动音频采集");
        return false;
    }

    m_running = true;

    // 8. 采集循环
    UINT32 packet_len = 0;
    UINT32 buffer_frames = 0;
    BYTE *p_data = nullptr;
    DWORD flags = 0;

    while (m_running) {
        hr = p_capture->GetNextPacketSize(&packet_len);
        if (FAILED(hr)) {
            break;
        }

        // 没有数据时短暂休眠
        if (packet_len == 0) {
            QThread::usleep(1000); // 1ms
            continue;
        }

        // 获取采集缓冲区
        hr = p_capture->GetBuffer(&p_data, &buffer_frames, &flags, nullptr, nullptr);
        if (FAILED(hr)) {
            break;
        }

        // 静音标志时跳过处理
        if (!(flags & AUDCLNT_BUFFERFLAGS_SILENT) && p_data != nullptr) {
            // 转换为 float 格式
            QVector<float> samples;
            convert_to_float(p_data, buffer_frames, channels, bits_per_sample, samples);

            // 处理音频数据（FFT + 波形）
            if (!samples.isEmpty()) {
                process_audio(samples.data(), buffer_frames);
            }
        }

        // 释放缓冲区
        p_capture->ReleaseBuffer(buffer_frames);
    }

    // 9. 停止采集并释放资源
    p_audio_client->Stop();
    p_capture->Release();
    CoTaskMemFree(p_format);
    p_audio_client->Release();

    return true;
}

// 将采集到的音频数据转为 float 格式（取左声道）
void AudioCaptureThread::convert_to_float(BYTE *data, UINT32 frames,
                                           UINT32 channels, UINT32 bits_per_sample,
                                           QVector<float> &out)
{
    out.resize(frames);

    if (bits_per_sample == 16) {
        // 16位 PCM：取左声道
        INT16 *p = (INT16 *)data;
        for (UINT32 i = 0; i < frames; i++) {
            out[i] = (float)p[i * channels] / 32768.0f;
        }
    } else if (bits_per_sample == 32) {
        // 32位浮点 PCM
        float *p = (float *)data;
        for (UINT32 i = 0; i < frames; i++) {
            out[i] = p[i * channels];
        }
    } else if (bits_per_sample == 24) {
        // 24位 PCM：取左声道
        for (UINT32 i = 0; i < frames; i++) {
            UINT32 offset = i * channels * 3;
            INT32 val = (INT32)data[offset]
                      | ((INT32)data[offset + 1] << 8)
                      | ((INT32)(INT8)data[offset + 2] << 16);
            out[i] = (float)val / 8388608.0f;
        }
    } else {
        // 不支持的格式，输出静音
        out.fill(0.0f);
    }
}

// 处理音频数据：FFT + 波形 + 能量
void AudioCaptureThread::process_audio(const float *samples, UINT32 frame_count)
{
    // ---- 波形数据 ----
    // 降采样到 256 个点用于波形显示
    int wave_points = 256;
    QVector<float> waveform(wave_points);
    for (int i = 0; i < wave_points; i++) {
        int idx = (int)((float)i / wave_points * frame_count);
        if (idx >= (int)frame_count) idx = frame_count - 1;
        waveform[i] = samples[idx];
    }
    emit sig_waveform(waveform);

    // ---- FFT 频谱数据 ----
    // 准备 FFT 输入（补零到 m_fft_size）
    int fft_n = m_fft_size;
    QVector<float> real(fft_n, 0.0f);
    QVector<float> imag(fft_n, 0.0f);

    // 复制采样数据，加汉宁窗减少频谱泄漏
    int copy_len = (int)frame_count < fft_n ? (int)frame_count : fft_n;
    for (int i = 0; i < copy_len; i++) {
        float window = 0.5f * (1.0f - cosf(2.0f * (float)M_PI * i / (copy_len - 1)));
        real[i] = samples[i] * window;
    }

    // 执行 FFT
    fft(real, imag);

    // 计算幅度谱（取前半部分，64 个频段）
    int bar_count = 64;
    QVector<float> spectrum(bar_count);
    int half_n = fft_n / 2;
    int bins_per_bar = half_n / bar_count;

    // 先计算所有频段的最大幅度，用于动态归一化
    float max_mag = 0.0f;
    QVector<float> raw_mag(bar_count);
    for (int i = 0; i < bar_count; i++) {
        float mag_sum = 0.0f;
        for (int j = 0; j < bins_per_bar; j++) {
            int idx = i * bins_per_bar + j;
            float mag = sqrtf(real[idx] * real[idx] + imag[idx] * imag[idx]);
            mag_sum += mag;
        }
        raw_mag[i] = mag_sum / bins_per_bar;
        if (raw_mag[i] > max_mag) {
            max_mag = raw_mag[i];
        }
    }

    // 动态归一化：以最大幅度为基准，放大到 0~1 范围
    // 加最小门限防止除零，加增益系数提升视觉效果
    float norm = max_mag > 0.001f ? max_mag : 0.001f;
    for (int i = 0; i < bar_count; i++) {
        spectrum[i] = raw_mag[i] / norm;
    }

    emit sig_spectrum(spectrum);

    // ---- 总能量 ----
    float energy = 0.0f;
    for (UINT32 i = 0; i < frame_count; i++) {
        energy += samples[i] * samples[i];
    }
    energy = sqrtf(energy / frame_count); // RMS
    // 限幅到 0~1
    if (energy > 1.0f) energy = 1.0f;
    emit sig_energy(energy);
}

// 基2 Cooley-Tukey FFT（原地计算）
void AudioCaptureThread::fft(QVector<float> &real, QVector<float> &imag)
{
    int n = real.size();

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
        float angle = -2.0f * (float)M_PI / len;
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

                // 旋转因子递推
                float new_real = cur_real * w_real - cur_imag * w_imag;
                float new_imag = cur_real * w_imag + cur_imag * w_real;
                cur_real = new_real;
                cur_imag = new_imag;
            }
        }
    }
}

// ============================================================================
// AudioCapture
// ============================================================================

// 构造函数
AudioCapture::AudioCapture(QObject *parent)
    : QObject(parent)
    , m_thread(nullptr)
{
}

// 析构函数：停止并销毁线程
AudioCapture::~AudioCapture()
{
    stop();
}

// 启动采集
bool AudioCapture::start()
{
    if (m_thread != nullptr && m_thread->isRunning()) {
        return true; // 已在运行
    }

    // 创建新线程
    m_thread = new AudioCaptureThread(this);

    // 连接信号转发
    connect(m_thread, &AudioCaptureThread::sig_spectrum,
            this, &AudioCapture::sig_spectrum);
    connect(m_thread, &AudioCaptureThread::sig_waveform,
            this, &AudioCapture::sig_waveform);
    connect(m_thread, &AudioCaptureThread::sig_energy,
            this, &AudioCapture::sig_energy);
    connect(m_thread, &AudioCaptureThread::sig_error,
            this, &AudioCapture::sig_error);

    // 线程结束时自动清理
    connect(m_thread, &QThread::finished, m_thread, &QObject::deleteLater);

    m_thread->start();
    return true;
}

// 停止采集
void AudioCapture::stop()
{
    if (m_thread != nullptr) {
        m_thread->stop();
        m_thread->wait();
        // 线程会在 finished 后 deleteLater，这里置空防止重复访问
        m_thread = nullptr;
    }
}

// 是否正在采集
bool AudioCapture::is_running() const
{
    return m_thread != nullptr && m_thread->isRunning();
}
