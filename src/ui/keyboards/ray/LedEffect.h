#ifndef LED_EFFECT_H
#define LED_EFFECT_H

#include <QColor>
#include <QVector>

// 灯效模式枚举
enum LedEffectMode {
    LED_FX_COLOR_CYCLE = 0, // 色彩循环：统一色相随时间旋转，覆盖全部RGB色
    LED_FX_RAINBOW     = 1, // 彩虹旋转：色相随时间+位置旋转
    LED_FX_BREATHING   = 2, // 呼吸灯：亮度正弦波缓慢变化
    LED_FX_WAVE        = 3, // 波浪：颜色从左到右流动
    LED_FX_SPARKLE     = 4, // 星火：随机位置闪烁
    LED_FX_STATIC      = 5, // 静态色：固定颜色
    LED_FX_MAX
};

// 灯效算法类：根据模式计算每个灯的颜色
class LedEffect
{
public:
    // 构造函数
    LedEffect();

    // 设置灯效模式
    void set_mode(LedEffectMode mode);
    // 获取当前灯效模式
    LedEffectMode get_mode() const;

    // 设置基础颜色（静态/呼吸/波浪模式使用）
    void set_base_color(const QColor &color);
    // 获取基础颜色
    QColor get_base_color() const;

    // 设置灯数量
    void set_count(int count);
    // 获取灯数量
    int get_count() const;

    // 推进一帧，返回每个灯的颜色数组
    // brightness: 外部亮度数据（如音乐律动），0~100，为空则不使用
    // base_colors: 每个灯的原始底色，用于颜色混合
    QVector<QColor> next_frame(const QVector<float> &brightness = QVector<float>(),
                               const QVector<QColor> &base_colors = QVector<QColor>());

    // 获取模式名称（用于UI显示）
    static const char* mode_name(LedEffectMode mode);

private:
    LedEffectMode m_mode;       // 当前灯效模式
    QColor m_base_color;        // 基础颜色
    int m_count;                // 灯数量
    int m_tick;                 // 帧计数器

    // 各灯效实现
    QVector<QColor> fx_color_cycle(const QVector<float> &brightness, const QVector<QColor> &base_colors);
    QVector<QColor> fx_rainbow(const QVector<float> &brightness, const QVector<QColor> &base_colors);
    QVector<QColor> fx_breathing(const QVector<float> &brightness, const QVector<QColor> &base_colors);
    QVector<QColor> fx_wave(const QVector<float> &brightness, const QVector<QColor> &base_colors);
    QVector<QColor> fx_sparkle(const QVector<float> &brightness, const QVector<QColor> &base_colors);
    QVector<QColor> fx_static(const QVector<float> &brightness, const QVector<QColor> &base_colors);

    // 星火模式：每个灯的随机目标亮度和当前亮度
    QVector<float> m_sparkle_target;
    QVector<float> m_sparkle_cur;

    // 颜色混合：base色按brightness比例混入effect色
    static QColor blend(const QColor &base, const QColor &effect, float brightness);
};

#endif // LED_EFFECT_H
