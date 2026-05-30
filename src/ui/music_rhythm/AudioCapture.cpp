#include "AudioCapture.h"
#include <QDebug>

// 构造函数：注册元类型 + 绑定回调
AudioCapture::AudioCapture(QObject *parent)
    : QObject(parent)
    , m_dev_dir(AUDIO_DEV_OUT)
{
    qRegisterMetaType<QVector<float>>("QVector<float>");

    // 采集层 → 处理层：原始PCM数据送入处理层
    m_capture.on_pcm_data([this](const float *data, uint32_t frames, uint32_t rate) {
        m_process.feed(data, frames, rate);
    });

    // 处理层 → Qt信号：频谱/波形/能量
    m_process.on_spectrum([this](const float *data, int len) {
        QVector<float> vec(len);
        memcpy(vec.data(), data, len * sizeof(float));
        emit sig_spectrum(vec);
    });

    m_process.on_waveform([this](const float *data, int len) {
        QVector<float> vec(len);
        memcpy(vec.data(), data, len * sizeof(float));
        emit sig_waveform(vec);
    });

    m_process.on_energy([this](float energy) {
        emit sig_energy(energy);
    });

    // 采集层 → Qt信号：错误
    m_capture.on_error([this](const char *msg) {
        emit sig_error(QString::fromUtf8(msg));
    });
}

// 析构函数
AudioCapture::~AudioCapture()
{
    stop();
}

// 枚举指定方向的音频设备
QVector<QPair<QString, QString>> AudioCapture::enum_devices(AudioDevDir dir)
{
    QVector<QPair<QString, QString>> result;
    auto devices = AudioCore::enum_devices(dir);
    for (const auto &dev : devices) {
        result.append(qMakePair(
            QString::fromUtf8(dev.name.c_str()),
            QString::fromUtf8(dev.id.c_str())
        ));
    }
    return result;
}

// 启动采集
bool AudioCapture::start()
{
    return m_capture.start();
}

// 停止采集
void AudioCapture::stop()
{
    m_capture.stop();
}

// 是否正在采集
bool AudioCapture::is_running() const
{
    return m_capture.is_running();
}

// 设置采集设备 ID 和方向
void AudioCapture::set_device(const QString &device_id, AudioDevDir dir)
{
    m_device_id = device_id;
    m_dev_dir = dir;
    m_capture.set_device(device_id.toUtf8().toStdString(), dir);
}

// 获取当前设备 ID
QString AudioCapture::device() const
{
    return m_device_id;
}

// 获取当前设备方向
AudioDevDir AudioCapture::dev_dir() const
{
    return m_dev_dir;
}

// 获取当前波形数据点数
int AudioCapture::get_waveform_points() const
{
    return m_process.get_waveform_points();
}

// 设置波形数据点数
void AudioCapture::set_waveform_points(int points)
{
    m_process.set_waveform_points(points);
}

// 获取当前频谱柱子数量
int AudioCapture::get_bar_count() const
{
    return m_process.get_bar_count();
}

// 设置频谱柱子数量
void AudioCapture::set_bar_count(int count)
{
    m_process.set_bar_count(count);
}
