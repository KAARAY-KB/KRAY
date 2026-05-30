#ifndef AUDIO_CAPTURE_H
#define AUDIO_CAPTURE_H

#include <QObject>
#include <QVector>
#include <QString>

#include "AudioCore.h"
#include "AudioProcess.h"

// Qt 音频采集包装层
// 桥接采集层(AudioCore) + 处理层(AudioProcess) 和 Qt 信号槽机制
class AudioCapture : public QObject
{
    Q_OBJECT

public:
    // 构造函数
    explicit AudioCapture(QObject *parent = nullptr);
    // 析构函数
    ~AudioCapture();

    // 枚举指定方向的音频设备（name, id）
    static QVector<QPair<QString, QString>> enum_devices(AudioDevDir dir);
    // 启动采集
    bool start();
    // 停止采集
    void stop();
    // 是否正在采集
    bool is_running() const;
    // 设置采集设备 ID 和方向
    void set_device(const QString &device_id, AudioDevDir dir);
    // 获取当前设备 ID
    QString device() const;
    // 获取当前设备方向
    AudioDevDir dev_dir() const;
    // 设置获取波形数据点数
    void set_waveform_points(int points);
    // 获取当前获取波形数据点数
    int get_waveform_points() const;
    // 获取当前获取频谱柱子数量
    int get_bar_count() const;
    // 设置获取频谱柱子数量
    void set_bar_count(int count);

signals:
    // 频谱数据
    void sig_spectrum(const QVector<float> &data);
    // 波形数据
    void sig_waveform(const QVector<float> &data);
    // 能量数据
    void sig_energy(float energy);
    // 错误信息
    void sig_error(const QString &msg);

private:
    AudioCore m_capture;         // 采集层
    AudioProcess m_process;      // 处理层
    QString m_device_id;         // 当前设备 ID
    AudioDevDir m_dev_dir;       // 当前设备方向
};

#endif // AUDIO_CAPTURE_H
