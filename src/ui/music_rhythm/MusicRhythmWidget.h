#ifndef MUSIC_RHYTHM_WIDGET_H
#define MUSIC_RHYTHM_WIDGET_H

#include <QWidget>
#include <QCloseEvent>
#include <QTimer>
#include <QVector>

#include "AudioCapture.h"

// 音乐律动可视化窗口
class MusicRhythmWidget : public QWidget
{
    Q_OBJECT

public:
    // 构造函数
    explicit MusicRhythmWidget(QWidget *parent = nullptr);
    // 析构函数
    ~MusicRhythmWidget();

    // 关闭事件处理
    void closeEvent(QCloseEvent *event) override;
    // 置顶显示
    void show_top(void);

signals:
    // 窗口关闭信号
    void exitWindow();
    // 频谱数据输出（可用于外部设备控制）
    void sig_spectrum(const QVector<float> &data);
    // 波形数据输出
    void sig_waveform(const QVector<float> &data);
    // 能量数据输出
    void sig_energy(float energy);

private slots:
    // 更新频谱数据
    void on_spectrum(const QVector<float> &data);
    // 更新波形数据
    void on_waveform(const QVector<float> &data);
    // 更新能量数据
    void on_energy(float energy);
    // 刷新显示
    void on_refresh();

private:
    // 绘制事件
    void paintEvent(QPaintEvent *event) override;
    // 绘制频谱柱状图
    void draw_spectrum(QPainter &painter, const QRect &rect);
    // 绘制波形图
    void draw_waveform(QPainter &painter, const QRect &rect);
    // 绘制能量指示器
    void draw_energy(QPainter &painter, const QRect &rect);

    AudioCapture *m_capture;          // 音频采集器
    QTimer *m_timer;                  // 刷新定时器
    QVector<float> m_spectrum;        // 频谱数据
    QVector<float> m_waveform;        // 波形数据
    float m_energy;                   // 当前能量值
    QVector<float> m_spectrum_peak;   // 频谱峰值（下落效果）
    QVector<float> m_spectrum_fall;   // 频谱下落速度
};

#endif // MUSIC_RHYTHM_WIDGET_H
