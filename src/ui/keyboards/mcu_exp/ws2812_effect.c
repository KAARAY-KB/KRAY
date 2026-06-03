#include "ws2812_effect.h"
#include <string.h>

/* ======================== sin 查表（256点，0~255） ======================== */
/* sin_table[i] = sin(i * 2π / 256) * 255，纯整数避免浮点运算 */
static const uint8_t sin_table[256] = {
    128,131,134,137,140,143,146,149,152,155,158,162,165,167,170,173,
    176,179,182,185,188,190,193,196,198,201,203,206,208,211,213,215,
    218,220,222,224,226,228,230,232,234,235,237,239,240,241,243,244,
    245,246,248,249,250,250,251,252,253,253,254,254,254,255,255,255,
    255,255,255,255,254,254,254,253,253,252,251,250,250,249,248,246,
    245,244,243,241,240,239,237,235,234,232,230,228,226,224,222,220,
    218,215,213,211,208,206,203,201,198,196,193,190,188,185,182,179,
    176,173,170,167,165,162,158,155,152,149,146,143,140,137,134,131,
    128,124,121,118,115,112,109,106,103,100, 97, 93, 90, 88, 85, 82,
     79, 76, 73, 70, 67, 65, 62, 59, 57, 54, 52, 49, 47, 44, 42, 40,
     37, 35, 33, 31, 29, 27, 25, 23, 21, 20, 18, 16, 15, 14, 12, 11,
     10,  9,  7,  6,  5,  5,  4,  3,  2,  2,  1,  1,  1,  0,  0,  0,
      0,  0,  0,  0,  1,  1,  1,  2,  2,  3,  4,  5,  5,  6,  7,  9,
     10, 11, 12, 14, 15, 16, 18, 20, 21, 23, 25, 27, 29, 31, 33, 35,
     37, 40, 42, 44, 47, 49, 52, 54, 57, 59, 62, 65, 67, 70, 73, 76,
     79, 82, 85, 88, 90, 93, 97,100,103,106,109,112,115,118,121,124,
};

/* 查表取 sin 值，输入 0~255 对应 0~2π，输出 0~255 */
static uint8_t fx_sin(uint8_t phase)
{
    return sin_table[phase];
}

/* ======================== HSV → RGB（整数运算） ======================== */
/* h: 0~255 对应 0°~360°，s: 0~255，v: 0~255 */
static ws_rgb_t hsv_to_rgb(uint8_t h, uint8_t s, uint8_t v)
{
    ws_rgb_t rgb = {0, 0, 0};

    if (s == 0) {
        rgb.r = rgb.g = rgb.b = v;
        return rgb;
    }

    /* h 分6个扇区，每个扇区约42.5° */
    uint8_t region = h / 43;
    uint8_t remainder = (h - region * 43) * 6;

    uint8_t p = (uint16_t)v * (255 - s) / 255;
    uint8_t q = (uint16_t)v * (255 - (uint16_t)s * remainder / 255) / 255;
    uint8_t t = (uint16_t)v * (255 - (uint16_t)s * (255 - remainder) / 255) / 255;

    switch (region) {
    case 0: rgb.r = v; rgb.g = t; rgb.b = p; break;
    case 1: rgb.r = q; rgb.g = v; rgb.b = p; break;
    case 2: rgb.r = p; rgb.g = v; rgb.b = t; break;
    case 3: rgb.r = p; rgb.g = q; rgb.b = v; break;
    case 4: rgb.r = t; rgb.g = p; rgb.b = v; break;
    default: rgb.r = v; rgb.g = p; rgb.b = q; break;
    }
    return rgb;
}

/* ======================== 颜色混合 ======================== */
/* base 按 val(0~255) 比例混入 effect，val=0 返回 base，val=255 返回 effect */
static ws_rgb_t blend(ws_rgb_t base, ws_rgb_t effect, uint8_t val)
{
    ws_rgb_t c;
    c.r = base.r   + (uint16_t)(effect.r   - base.r)   * val / 255;
    c.g = base.g   + (uint16_t)(effect.g   - base.g)   * val / 255;
    c.b = base.b   + (uint16_t)(effect.b   - base.b)   * val / 255;
    return c;
}

/* 计算最终亮度：效果动画值 × 外部亮度调制 */
/* anim: 效果动画值 0~255，brightness: 外部亮度 0~255 */
static uint8_t calc_val(uint8_t anim, const uint8_t *brightness, uint16_t i, uint16_t count)
{
    if (brightness && i < count) {
        return (uint16_t)anim * brightness[i] / 255;
    }
    return anim;
}

/* ======================== 伪随机数（单片机无需 stdlib） ======================== */
static uint16_t rng_state = 0xACED;

/* 设置随机种子 */
static void rng_seed(uint16_t seed)
{
    rng_state = seed;
}

/* 返回 0~255 随机数 */
static uint8_t rng_next(void)
{
    rng_state ^= rng_state << 7;
    rng_state ^= rng_state >> 9;
    rng_state ^= rng_state << 8;
    return (uint8_t)(rng_state & 0xFF);
}

/* ======================== 灯效实现 ======================== */

/* 静态色：固定颜色 */
static void fx_static(ws_fx_t *fx, const uint8_t *brightness)
{
    for (uint16_t i = 0; i < fx->count; i++) {
        uint8_t val = calc_val(255, brightness, i, fx->count);
        ws_rgb_t black = {0, 0, 0};
        fx->buf[i] = blend(black, fx->base_color, val);
    }
}

/* 呼吸灯：亮度正弦波缓慢变化 */
static void fx_breathing(ws_fx_t *fx, const uint8_t *brightness)
{
    /* 呼吸周期约4秒（120帧@30fps），tick 每帧+1 */
    /* phase: 0~255 对应一个完整正弦周期 */
    uint8_t phase = (fx->tick % 120) * 255 / 120;
    /* breath: 0~255 正弦变化 */
    uint8_t breath = fx_sin(phase);

    for (uint16_t i = 0; i < fx->count; i++) {
        uint8_t val = calc_val(breath, brightness, i, fx->count);
        ws_rgb_t black = {0, 0, 0};
        fx->buf[i] = blend(black, fx->base_color, val);
    }
}

/* 色彩循环：统一色相随时间旋转，所有灯同色 */
static void fx_color_cycle(ws_fx_t *fx, const uint8_t *brightness)
{
    /* 每帧色相+2，约128帧（4秒@30fps）转一圈 */
    uint8_t hue = (fx->tick * 2) & 0xFF;
    ws_rgb_t effect = hsv_to_rgb(hue, 255, 255);

    for (uint16_t i = 0; i < fx->count; i++) {
        uint8_t val = calc_val(255, brightness, i, fx->count);
        ws_rgb_t black = {0, 0, 0};
        fx->buf[i] = blend(black, effect, val);
    }
}

/* 彩虹旋转：色相随时间+位置旋转 */
static void fx_rainbow(ws_fx_t *fx, const uint8_t *brightness)
{
    uint8_t hue_base = (fx->tick * 2) & 0xFF;

    for (uint16_t i = 0; i < fx->count; i++) {
        /* 每个灯按位置偏移色相，形成彩虹流动 */
        uint8_t hue = (hue_base + (uint8_t)((uint16_t)i * 256 / fx->count)) & 0xFF;
        ws_rgb_t effect = hsv_to_rgb(hue, 255, 255);
        uint8_t val = calc_val(255, brightness, i, fx->count);
        ws_rgb_t black = {0, 0, 0};
        fx->buf[i] = blend(black, effect, val);
    }
}

/* 波浪：颜色从左到右流动 */
static void fx_wave(ws_fx_t *fx, const uint8_t *brightness)
{
    /* 波浪周期约2秒（60帧@30fps） */
    uint8_t phase = (fx->tick % 60) * 255 / 60;

    for (uint16_t i = 0; i < fx->count; i++) {
        /* 每个灯按位置偏移相位 */
        uint8_t p = phase + (uint8_t)((uint16_t)i * 256 / fx->count);
        uint8_t wave = fx_sin(p);
        uint8_t val = calc_val(wave, brightness, i, fx->count);
        ws_rgb_t black = {0, 0, 0};
        fx->buf[i] = blend(black, fx->base_color, val);
    }
}

/* 星火：随机位置闪烁 */
static void fx_sparkle(ws_fx_t *fx, const uint8_t *brightness)
{
    for (uint16_t i = 0; i < fx->count; i++) {
        /* 每帧约3%概率点亮 */
        if (rng_next() < 8) {
            fx->sparkle_target[i] = 255;
        } else {
            /* 目标逐渐衰减 */
            fx->sparkle_target[i] = (uint16_t)fx->sparkle_target[i] * 217 / 255;
        }
        /* 当前亮度向目标平滑过渡 */
        int16_t diff = (int16_t)fx->sparkle_target[i] - fx->sparkle_cur[i];
        fx->sparkle_cur[i] += diff * 77 / 255;

        uint8_t val = calc_val(fx->sparkle_cur[i], brightness, i, fx->count);
        ws_rgb_t black = {0, 0, 0};
        fx->buf[i] = blend(black, fx->base_color, val);
    }
}

/* ======================== 公共 API ======================== */

/* 初始化灯效上下文 */
void ws_fx_init(ws_fx_t *fx, ws_rgb_t *buf, uint16_t count)
{
    fx->mode       = WS_FX_STATIC;
    fx->base_color = (ws_rgb_t){0, 255, 0};
    fx->count      = count;
    fx->tick       = 0;
    fx->buf        = buf;
    fx->sparkle_target = (uint8_t *)0;
    fx->sparkle_cur    = (uint8_t *)0;

    /* 清空帧缓冲区 */
    memset(buf, 0, count * sizeof(ws_rgb_t));

    /* 用 tick 值做随机种子 */
    rng_seed(0xACED);
}

/* 设置灯效模式 */
void ws_fx_set_mode(ws_fx_t *fx, ws_fx_mode_t mode)
{
    fx->mode = mode;
    fx->tick = 0;

    /* 切换模式时重置星火状态 */
    if (fx->sparkle_target) {
        memset(fx->sparkle_target, 0, fx->count);
    }
    if (fx->sparkle_cur) {
        memset(fx->sparkle_cur, 0, fx->count);
    }
}

/* 设置基础颜色 */
void ws_fx_set_color(ws_fx_t *fx, ws_rgb_t color)
{
    fx->base_color = color;
}

/* 推进一帧 */
void ws_fx_tick(ws_fx_t *fx, const uint8_t *brightness)
{
    fx->tick++;

    switch (fx->mode) {
    case WS_FX_STATIC:      fx_static(fx, brightness);      break;
    case WS_FX_BREATHING:   fx_breathing(fx, brightness);   break;
    case WS_FX_COLOR_CYCLE: fx_color_cycle(fx, brightness); break;
    case WS_FX_RAINBOW:     fx_rainbow(fx, brightness);     break;
    case WS_FX_WAVE:        fx_wave(fx, brightness);        break;
    case WS_FX_SPARKLE:     fx_sparkle(fx, brightness);     break;
    default:                fx_static(fx, brightness);      break;
    }
}

/* 获取模式名称 */
const char *ws_fx_name(ws_fx_mode_t mode)
{
    static const char *names[] = {
        "Static",
        "Breathing",
        "ColorCycle",
        "Rainbow",
        "Wave",
        "Sparkle",
    };
    if (mode < WS_FX_MAX) return names[mode];
    return "Unknown";
}

/* ======================== 音乐律动映射 ======================== */

/* 将 PC 端 16×5 网格亮度映射到 66 颗 LED 亮度 */
void ws_fx_map_grid(const uint8_t *grid, uint8_t *out)
{
    /*
     * MCU LED 蛇形走线排列：
     *   行0 (y=4): LED  0~13   → grid row4, col 0~13  正序
     *   行1 (y=3): LED 27~14   → grid row3, col 0~13  反序
     *   行2 (y=2): LED 28~40   → grid row2, col 0~12  正序
     *   行3 (y=1): LED 52~41   → grid row1, col 0~11  反序
     *   行4 (y=0): LED 53~65   → grid row0, col 0~12  正序
     *
     * grid 索引 = row * WS_GRID_COLS + col
     * row0~4 对应 y=4~0（PC 端 sig_led_grid 行优先）
     */

    /* 行0 (y=4): LED 0~13, grid row4, 正序 */
    for (uint16_t c = 0; c < 14; c++) {
        out[c] = grid[4 * WS_GRID_COLS + c];
    }

    /* 行1 (y=3): LED 27~14, grid row3, 反序 */
    for (uint16_t c = 0; c < 14; c++) {
        out[27 - c] = grid[3 * WS_GRID_COLS + c];
    }

    /* 行2 (y=2): LED 28~40, grid row2, 正序 */
    for (uint16_t c = 0; c < 13; c++) {
        out[28 + c] = grid[2 * WS_GRID_COLS + c];
    }

    /* 行3 (y=1): LED 52~41, grid row1, 反序 */
    for (uint16_t c = 0; c < 12; c++) {
        out[52 - c] = grid[1 * WS_GRID_COLS + c];
    }

    /* 行4 (y=0): LED 53~65, grid row0, 正序 */
    for (uint16_t c = 0; c < 13; c++) {
        out[53 + c] = grid[0 * WS_GRID_COLS + c];
    }
}
