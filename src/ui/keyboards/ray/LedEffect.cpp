#include "LedEffect.h"
#include <QtMath>
#include <QRandomGenerator>

// 构造函数
LedEffect::LedEffect()
    : m_mode(LED_FX_COLOR_CYCLE)
    , m_base_color(0, 255, 0)
    , m_count(0)
    , m_tick(0)
{
}

// 设置灯效模式
void LedEffect::set_mode(LedEffectMode mode)
{
    m_mode = mode;
    // 切换模式时重置状态
    m_tick = 0;
    m_sparkle_target.clear();
    m_sparkle_cur.clear();
}

// 获取当前灯效模式
LedEffectMode LedEffect::get_mode() const
{
    return m_mode;
}

// 设置基础颜色
void LedEffect::set_base_color(const QColor &color)
{
    m_base_color = color;
}

// 获取基础颜色
QColor LedEffect::get_base_color() const
{
    return m_base_color;
}

// 设置灯数量
void LedEffect::set_count(int count)
{
    m_count = count;
    // 星火模式缓存跟随灯数变化
    m_sparkle_target.resize(count);
    m_sparkle_cur.resize(count);
    m_sparkle_target.fill(0.0f);
    m_sparkle_cur.fill(0.0f);
}

// 获取灯数量
int LedEffect::get_count() const
{
    return m_count;
}

// 推进一帧，返回每个灯的颜色
QVector<QColor> LedEffect::next_frame(const QVector<float> &brightness,
                                       const QVector<QColor> &base_colors)
{
    m_tick++;
    switch (m_mode) {
    case LED_FX_COLOR_CYCLE: return fx_color_cycle(brightness, base_colors);
    case LED_FX_RAINBOW:     return fx_rainbow(brightness, base_colors);
    case LED_FX_BREATHING:   return fx_breathing(brightness, base_colors);
    case LED_FX_WAVE:        return fx_wave(brightness, base_colors);
    case LED_FX_SPARKLE:     return fx_sparkle(brightness, base_colors);
    case LED_FX_STATIC:      return fx_static(brightness, base_colors);
    default:                 return fx_color_cycle(brightness, base_colors);
    }
}

// 获取模式名称
const char* LedEffect::mode_name(LedEffectMode mode)
{
    static const char* names[] = {
        "色彩循环",
        "彩虹旋转",
        "呼吸灯",
        "波浪",
        "星火",
        "静态色",
    };
    if (mode >= 0 && mode < LED_FX_MAX) return names[mode];
    return "未知";
}

// 颜色混合：base色按brightness(0~100)比例混入effect色
QColor LedEffect::blend(const QColor &base, const QColor &effect, float brightness)
{
    int t = qBound(0, (int)brightness, 100);
    int r = base.red()   + (effect.red()   - base.red())   * t / 100;
    int g = base.green() + (effect.green() - base.green()) * t / 100;
    int b = base.blue()  + (effect.blue()  - base.blue())  * t / 100;
    return QColor(r, g, b);
}

// 获取第i个灯的底色
static QColor get_base(const QVector<QColor> &base_colors, int i)
{
    if (i < base_colors.size()) return base_colors[i];
    return QColor(0, 0, 0);
}

// 计算最终亮度：效果动画值 × 外部亮度调制
// anim: 效果动画值 0~100
// brightness: 外部亮度数据 0~100
static float calc_val(float anim, const QVector<float> &brightness, int i)
{
    float b = (i < brightness.size()) ? brightness[i] : 100.0f;
    return anim * b / 100.0f;
}

// 色彩循环：统一色相随时间旋转，所有灯同色，覆盖全部RGB色
QVector<QColor> LedEffect::fx_color_cycle(const QVector<float> &brightness, const QVector<QColor> &base_colors)
{
    QVector<QColor> colors(m_count);
    // 每帧色相偏移2，约6秒转一圈（30fps）
    int hue = (m_tick * 2) % 360;
    QColor effect = QColor::fromHsv(hue, 255, 255);

    for (int i = 0; i < m_count; i++) {
        float val = calc_val(100.0f, brightness, i);
        colors[i] = blend(get_base(base_colors, i), effect, val);
    }
    return colors;
}

// 彩虹旋转：色相随时间+位置旋转
QVector<QColor> LedEffect::fx_rainbow(const QVector<float> &brightness, const QVector<QColor> &base_colors)
{
    QVector<QColor> colors(m_count);
    // 每帧色相偏移2，约6秒转一圈（30fps）
    int hue_base = (m_tick * 2) % 360;

    for (int i = 0; i < m_count; i++) {
        // 每个灯按位置偏移色相，形成彩虹流动
        int hue = (hue_base + i * 6) % 360;
        QColor effect = QColor::fromHsv(hue, 255, 255);
        float val = calc_val(100.0f, brightness, i);
        colors[i] = blend(get_base(base_colors, i), effect, val);
    }
    return colors;
}

// 呼吸灯：亮度正弦波缓慢变化
QVector<QColor> LedEffect::fx_breathing(const QVector<float> &brightness, const QVector<QColor> &base_colors)
{
    QVector<QColor> colors(m_count);
    // 正弦周期约120帧（4秒@30fps）
    float phase = (m_tick % 120) / 120.0f * 2.0f * M_PI;
    // 亮度0~100正弦变化
    float breath = (qSin(phase) + 1.0f) / 2.0f * 100.0f;

    for (int i = 0; i < m_count; i++) {
        float val = calc_val(breath, brightness, i);
        colors[i] = blend(get_base(base_colors, i), m_base_color, val);
    }
    return colors;
}

// 波浪：颜色从左到右流动
QVector<QColor> LedEffect::fx_wave(const QVector<float> &brightness, const QVector<QColor> &base_colors)
{
    QVector<QColor> colors(m_count);
    // 波浪周期约60帧（2秒@30fps）
    float phase = (m_tick % 60) / 60.0f * 2.0f * M_PI;

    for (int i = 0; i < m_count; i++) {
        // 每个灯按位置偏移相位
        float p = phase + (float)i / m_count * 2.0f * M_PI;
        float wave = (qSin(p) + 1.0f) / 2.0f * 100.0f;
        float val = calc_val(wave, brightness, i);
        colors[i] = blend(get_base(base_colors, i), m_base_color, val);
    }
    return colors;
}

// 星火：随机位置闪烁
QVector<QColor> LedEffect::fx_sparkle(const QVector<float> &brightness, const QVector<QColor> &base_colors)
{
    QVector<QColor> colors(m_count);

    // 确保缓存大小正确
    if (m_sparkle_target.size() != m_count) {
        m_sparkle_target.resize(m_count);
        m_sparkle_cur.resize(m_count);
        m_sparkle_target.fill(0.0f);
        m_sparkle_cur.fill(0.0f);
    }

    for (int i = 0; i < m_count; i++) {
        // 随机决定是否点亮（每帧约3%概率）
        if (QRandomGenerator::global()->bounded(100) < 3) {
            m_sparkle_target[i] = 100.0f;
        } else {
            // 目标逐渐暗下去
            m_sparkle_target[i] *= 0.85f;
        }
        // 当前亮度向目标平滑过渡
        m_sparkle_cur[i] += (m_sparkle_target[i] - m_sparkle_cur[i]) * 0.3f;

        float val = calc_val(m_sparkle_cur[i], brightness, i);
        colors[i] = blend(get_base(base_colors, i), m_base_color, val);
    }
    return colors;
}

// 静态色：固定颜色
QVector<QColor> LedEffect::fx_static(const QVector<float> &brightness, const QVector<QColor> &base_colors)
{
    QVector<QColor> colors(m_count);

    for (int i = 0; i < m_count; i++) {
        float val = calc_val(100.0f, brightness, i);
        colors[i] = blend(get_base(base_colors, i), m_base_color, val);
    }
    return colors;
}
