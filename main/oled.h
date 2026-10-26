#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 初始化 I2C 总线与 OLED 屏幕
 *
 * 引脚、I2C 地址、控制器型号都在 menuconfig 中配置
 * （Component config -> OLED Demo Configuration）。
 */
esp_err_t oled_init(void);

/** @brief 清空显存（不会立即刷新到屏幕） */
void oled_clear(void);

/** @brief 把显存内容刷新到 OLED 屏幕 */
void oled_display(void);

/** @brief 点亮/熄灭一个像素 */
void oled_draw_pixel(int x, int y, bool on);

/** @brief 画水平线 */
void oled_draw_hline(int x, int y, int w, bool on);

/** @brief 画垂直线 */
void oled_draw_vline(int x, int y, int h, bool on);

/** @brief 画矩形边框（不填充） */
void oled_draw_rect(int x, int y, int w, int h, bool on);

/**
 * @brief 在 (x, y) 处画一串 ASCII 文本
 * @param x 左上角 x 坐标（像素）
 * @param y 左上角 y 坐标（像素）
 * @param text 只支持可打印 ASCII（0x20 ~ 0x7E）
 *
 * 字符宽 5 像素、步进 6 像素、高 7 像素。
 */
void oled_draw_string(int x, int y, const char *text);

/**
 * @brief 同上，但可以按整数倍放大
 * @param scale 1 = 原始大小，2 = 2 倍，最大建议 4
 */
void oled_draw_string_scale(int x, int y, const char *text, uint8_t scale);

#ifdef __cplusplus
}
#endif
