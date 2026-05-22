#include "AudioCapture.h"
#include <QDebug>

// 构造函数：注册元类型 + 绑定回调
AudioCapture::AudioCapture(QObject *parent)
    : QObject(parent)
    , m_dev_dir(AUDIO_DEV_OUT)
{
    // 注册跨线程信号所需的元类型
    qRegisterMetaType<QVector<float>>("QVector<float>");

    // 绑定 AudioCore 回调，桥接到 Qt 信号
    m_core.on_spectrum([this](const float *data, int len) {
        QVector<float> vec(len);
        memcpy(vec.data(), data, len * sizeof(float));
        emit sig_spectrum(vec);
    });

    m_core.on_waveform([this](const float *data, int len) {
        QVector<float> vec(len);
        memcpy(vec.data(), data, len * sizeof(float));
        emit sig_waveform(vec);
    });

    m_core.on_energy([this](float energy) {
        emit sig_energy(energy);
    });

    m_core.on_error([this](const char *msg) {
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
    return m_core.start();
}

// 停止采集
void AudioCapture::stop()
{
    m_core.stop();
}

// 是否正在采集
bool AudioCapture::is_running() const
{
    return m_core.is_running();
}

// 设置采集设备 ID 和方向
void AudioCapture::set_device(const QString &device_id, AudioDevDir dir)
{
    m_device_id = device_id;
    m_dev_dir = dir;
    m_core.set_device(device_id.toUtf8().toStdString(), dir);
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
