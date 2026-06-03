#ifndef WS2812_EFFECT_H
#define WS2812_EFFECT_H

#include <stdint.h>

/* RGB 颜色 */
typedef struct {
    uint8_t r;
    uint8_t g;
    uint8_t b;
} ws_rgb_t;

/* 灯效模式 */
typedef enum {
    WS_FX_STATIC      = 0, /* 静态色：固定颜色 */
    WS_FX_BREATHING   = 1, /* 呼吸灯：亮度正弦波缓慢变化 */
    WS_FX_COLOR_CYCLE = 2, /* 色彩循环：统一色相随时间旋转 */
    WS_FX_RAINBOW     = 3, /* 彩虹旋转：色相随时间+位置旋转 */
    WS_FX_WAVE        = 4, /* 波浪：颜色从左到右流动 */
    WS_FX_SPARKLE     = 5, /* 星火：随机位置闪烁 */
    WS_FX_MAX
} ws_fx_mode_t;

/* 灯效上下文 */
typedef struct {
    ws_fx_mode_t mode;       /* 当前灯效模式 */
    ws_rgb_t     base_color; /* 基础颜色（静态/呼吸/波浪/星火使用） */
    uint16_t     count;      /* LED 数量 */
    uint16_t     tick;       /* 帧计数器 */
    ws_rgb_t    *buf;        /* 帧缓冲区，由外部分配，驱动层读取发送 */
    /* 星火模式内部状态 */
    uint8_t *sparkle_target; /* 星火目标亮度 0~255 */
    uint8_t *sparkle_cur;    /* 星火当前亮度 0~255 */
} ws_fx_t;

/*
 * 初始化灯效上下文
 * fx:       灯效上下文
 * buf:      帧缓冲区（外部分配，大小 count 个 ws_rgb_t）
 * count:    LED 数量
 */
void ws_fx_init(ws_fx_t *fx, ws_rgb_t *buf, uint16_t count);

/*
 * 设置灯效模式（切换时自动重置内部状态）
 */
void ws_fx_set_mode(ws_fx_t *fx, ws_fx_mode_t mode);

/*
 * 设置基础颜色
 */
void ws_fx_set_color(ws_fx_t *fx, ws_rgb_t color);

/*
 * 推进一帧，将结果写入 fx->buf
 * brightness: 外部亮度调制数组，每个 LED 0~255，255=全亮；传 NULL 则不调制
 * 驱动层在调用后读取 fx->buf 发送给 WS2812
 */
void ws_fx_tick(ws_fx_t *fx, const uint8_t *brightness);

/*
 * 获取模式名称字符串
 */
const char *ws_fx_name(ws_fx_mode_t mode);

/* ======================== 音乐律动映射 ======================== */

/* LED 网格参数：16列 × 5行 = 80 */
#define WS_GRID_COLS  16
#define WS_GRID_ROWS  5
#define WS_GRID_SIZE  (WS_GRID_COLS * WS_GRID_ROWS)

/* GT64HE LED 总数 */
#define WS_LED_COUNT  66

/*
 * 将 PC 端 16×5 LED 网格亮度映射到 66 颗 LED 的亮度数组
 * grid:   PC 端发来的 16×5 网格数据，80 个值，范围 0~255
 * out:    输出亮度数组，66 个值，范围 0~255
 *
 * 映射规则（蛇形走线）：
 *   行0 (y=4): LED 0~13   → grid[4][0~13]
 *   行1 (y=3): LED 27~14  → grid[3][0~13]（反序）
 *   行2 (y=2): LED 28~40  → grid[2][0~12]
 *   行3 (y=1): LED 52~41  → grid[1][0~11]（反序）
 *   行4 (y=0): LED 53~65  → grid[0][0~12]
 */
void ws_fx_map_grid(const uint8_t *grid, uint8_t *out);

#endif /* WS2812_EFFECT_H */
