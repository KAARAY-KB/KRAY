#ifndef AUDIO_CAPTURE_H
#define AUDIO_CAPTURE_H

#include <QObject>
#include <QThread>
#include <QVector>
#include <QMutex>
#include <atomic>

#ifdef _WIN32
#include <windows.h>
#include <mmdeviceapi.h>
#include <audioclient.h>
#include <comdef.h>
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "mmdevapi.lib")
#endif

// 音频采集线程，在后台运行 WASAPI loopback 采集
class AudioCaptureThread : public QThread
{
    Q_OBJECT

public:
    // 构造函数
    explicit AudioCaptureThread(QObject *parent = nullptr);
    // 析构函数
    ~AudioCaptureThread();

    // 停止采集
    void stop();

signals:
    // 发送频谱数据（FFT 结果，归一化 0~1）
    void sig_spectrum(const QVector<float> &data);
    // 发送波形数据（原始 PCM 采样，归一化 -1~1）
    void sig_waveform(const QVector<float> &data);
    // 发送总能量值（归一化 0~1）
    void sig_energy(float energy);
    // 发送错误信息
    void sig_error(const QString &msg);

protected:
    // 线程入口
    void run() override;

private:
    // 执行 WASAPI loopback 采集
    bool capture_loop();
    // 对音频数据做 FFT 并发射信号
    void process_audio(const float *samples, UINT32 frame_count);
    // 基2 Cooley-Tukey FFT（原地计算）
    void fft(QVector<float> &real, QVector<float> &imag);
    // 将采集到的数据转为 float 格式
    void convert_to_float(BYTE *data, UINT32 frames, UINT32 channels, UINT32 bits_per_sample, QVector<float> &out);

    std::atomic<bool> m_running; // 运行标志
    int m_fft_size;              // FFT 大小（2的幂次）
    int m_sample_rate;           // 采样率
};

// 音频采集控制器，管理采集线程的生命周期
class AudioCapture : public QObject
{
    Q_OBJECT

public:
    // 构造函数
    explicit AudioCapture(QObject *parent = nullptr);
    // 析构函数
    ~AudioCapture();

    // 启动采集
    bool start();
    // 停止采集
    void stop();
    // 是否正在采集
    bool is_running() const;

signals:
    // 转发频谱数据
    void sig_spectrum(const QVector<float> &data);
    // 转发波形数据
    void sig_waveform(const QVector<float> &data);
    // 转发能量数据
    void sig_energy(float energy);
    // 转发错误信息
    void sig_error(const QString &msg);

private:
    AudioCaptureThread *m_thread; // 采集线程
};

#endif // AUDIO_CAPTURE_H
