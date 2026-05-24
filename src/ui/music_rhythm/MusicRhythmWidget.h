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

// LED灯效模式
enum LedMode {
    LED_MODE_LAYERS = 0,    // 强度分层：16列×5级强度
    LED_MODE_BANDS  = 1,    // 80频段：16列×5行独立频段
    LED_MODE_MIRROR = 2,    // 镜像波形：中间基线，上下对称
};

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
    // LED网格数据输出（16列×5行=80个值，行优先）
    void sig_led_grid(const QVector<float> &data);

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
    // 切换LED灯效模式
    void on_led_mode_changed(int index);

private:
    // 绘制事件
    void paintEvent(QPaintEvent *event) override;
    // 绘制频谱柱状图
    void draw_spectrum(QPainter &painter, const QRect &rect);
    // 绘制波形图
    void draw_waveform(QPainter &painter, const QRect &rect);
    // 绘制能量指示器
    void draw_energy(QPainter &painter, const QRect &rect);
    // 映射LED网格数据
    void map_led_grid();
    // 绘制LED网格预览
    void draw_led_grid(QPainter &painter, const QRect &rect);
    // 创建控制面板
    void create_ctrl_panel();
    // 刷新设备列表
    void refresh_devices();

    AudioCapture *m_capture = nullptr;          // 音频采集器
    QTimer *m_timer = nullptr;                  // 刷新定时器
    QVector<float> m_spectrum;        // 频谱数据
    QVector<float> m_waveform;        // 波形数据
    float m_energy = 0.0f;                   // 当前能量值
    QVector<float> m_spectrum_peak;   // 频谱峰值（下落效果）
    QVector<float> m_spectrum_fall;   // 频谱下落速度
    QComboBox *m_device_combo = nullptr;        // 设备选择下拉框
    QLabel *m_device_label = nullptr;           // 设备标签
    QSlider *m_spec_gain_slider = nullptr;      // 频谱增益滑块
    QLabel *m_spec_gain_label = nullptr;        // 频谱增益标签
    float m_spec_gain = 1.0f;                // 频谱增益值
    QSlider *m_wave_gain_slider = nullptr;      // 波形增益滑块
    QLabel *m_wave_gain_label = nullptr;        // 波形增益标签
    float m_wave_gain = 1.0f;                // 波形增益值
    QSlider *m_energy_gain_slider = nullptr;    // 能量增益滑块
    QLabel *m_energy_gain_label = nullptr;      // 能量增益标签
    float m_energy_gain = 1.0f;              // 能量增益值
    QSlider *m_wave_pts_slider = nullptr;       // 波形点数滑块
    QLabel *m_wave_pts_label = nullptr;         // 波形点数标签
    int m_wave_pts = 512;                   // 波形点数
    QSlider *m_bar_cnt_slider = nullptr;        // 频谱柱数滑块
    QLabel *m_bar_cnt_label = nullptr;          // 频谱柱数标签
    int m_bar_cnt = 16;                    // 频谱柱数
    LedMode m_led_mode = LED_MODE_LAYERS;               // LED灯效模式
    QComboBox *m_led_mode_combo = nullptr;      // LED模式选择下拉框
    QLabel *m_led_mode_label = nullptr;         // LED模式标签
    QVector<float> m_led_grid;        // LED网格数据(16×5=80)
    QVector<float> m_led_grid_prev;   // LED网格上一帧(余辉衰减)
    int m_user_bar_cnt = 16;               // 用户自定义柱数
    float m_auto_gain = 10.0f;                // 自动增益值
};

#endif // MUSIC_RHYTHM_WIDGET_H
