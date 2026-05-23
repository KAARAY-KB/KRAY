#ifndef MUSIC_RHYTHM_WIDGET_H
#define MUSIC_RHYTHM_WIDGET_H

#include <QWidget>
#include <QCloseEvent>
#include <QTimer>
#include <QVector>
#include <QSlider>
#include <QLabel>
#include <QComboBox>

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
    // 切换采集设备
    void on_device_changed(int index);

private:
    // 绘制事件
    void paintEvent(QPaintEvent *event) override;
    // 绘制频谱柱状图
    void draw_spectrum(QPainter &painter, const QRect &rect);
    // 绘制波形图
    void draw_waveform(QPainter &painter, const QRect &rect);
    // 绘制能量指示器
    void draw_energy(QPainter &painter, const QRect &rect);
    // 创建控制面板
    void create_ctrl_panel();
    // 刷新设备列表
    void refresh_devices();

    AudioCapture *m_capture;          // 音频采集器
    QTimer *m_timer;                  // 刷新定时器
    QVector<float> m_spectrum;        // 频谱数据
    QVector<float> m_waveform;        // 波形数据
    float m_energy;                   // 当前能量值
    QVector<float> m_spectrum_peak;   // 频谱峰值（下落效果）
    QVector<float> m_spectrum_fall;   // 频谱下落速度
    QComboBox *m_device_combo;        // 设备选择下拉框
    QLabel *m_device_label;           // 设备标签
    QSlider *m_spec_gain_slider;      // 频谱增益滑块
    QLabel *m_spec_gain_label;        // 频谱增益标签
    float m_spec_gain;                // 频谱增益值
    QSlider *m_wave_gain_slider;      // 波形增益滑块
    QLabel *m_wave_gain_label;        // 波形增益标签
    float m_wave_gain;                // 波形增益值
    QSlider *m_energy_gain_slider;    // 能量增益滑块
    QLabel *m_energy_gain_label;      // 能量增益标签
    float m_energy_gain;              // 能量增益值
    QSlider *m_wave_pts_slider;       // 波形点数滑块
    QLabel *m_wave_pts_label;         // 波形点数标签
    int m_wave_pts;                   // 波形点数
    QSlider *m_bar_cnt_slider;        // 频谱柱数滑块
    QLabel *m_bar_cnt_label;          // 频谱柱数标签
    int m_bar_cnt;                    // 频谱柱数
};

#endif // MUSIC_RHYTHM_WIDGET_H
