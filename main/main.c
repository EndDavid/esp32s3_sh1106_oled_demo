#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "sdkconfig.h"

#include "oled.h"

static const char *TAG = "oled_demo";

static void draw_splash(void)
{
    oled_clear();

    /* 双层边框，看起来更有“开机画面”的感觉 */
    oled_draw_rect(0, 0, 128, 64, true);
    oled_draw_rect(2, 2, 124, 60, true);

    oled_draw_string_scale(16, 12, "ESP32-S3", 2);
    oled_draw_hline(12, 36, 104, true);
    oled_draw_string(22, 44, "1.3\" OLED DEMO");
    oled_draw_string(31, 54, "Hello World");

    oled_display();
}

void app_main(void)
{
    ESP_LOGI(TAG, "OLED demo starting");
    ESP_LOGI(TAG, "I2C%d: SDA=GPIO%d, SCL=GPIO%d, addr=0x%02X",
             CONFIG_OLED_I2C_PORT,
             CONFIG_OLED_SDA_GPIO,
             CONFIG_OLED_SCL_GPIO,
             (unsigned)CONFIG_OLED_I2C_ADDR);

    esp_err_t err = oled_init();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "OLED init failed: %s", esp_err_to_name(err));
        ESP_LOGE(TAG, "Please check the wiring, then rebuild/flash again.");
        return;
    }

    draw_splash();
    vTaskDelay(pdMS_TO_TICKS(2000));

    const int64_t start_us = esp_timer_get_time();
    char line[32];

    while (true) {
        int seconds = (int)((esp_timer_get_time() - start_us) / 1000000);

        oled_clear();

        /* 标题 */
        oled_draw_string(0, 0, "ESP32-S3 OLED DEMO");
        oled_draw_hline(0, 9, 128, true);

        /* 运行时间 */
        snprintf(line, sizeof(line), "Uptime: %d s", seconds);
        oled_draw_string(0, 14, line);

        /* 当前使用的引脚 */
        snprintf(line, sizeof(line), "SDA=%d  SCL=%d",
                 CONFIG_OLED_SDA_GPIO, CONFIG_OLED_SCL_GPIO);
        oled_draw_string(0, 26, line);

        /* 静态文本 + 闪烁的符号 */
        snprintf(line, sizeof(line), "Hello, world! %s", (seconds % 2) ? "*" : " ");
        oled_draw_string(0, 38, line);

        /* 进度条：每 10 秒走满一次 */
        oled_draw_rect(0, 50, 128, 13, true);
        int filled = (seconds % 10) * 126 / 10;
        for (int y = 52; y < 61; y++) {
            oled_draw_hline(1, y, filled, true);
        }

        oled_display();
        vTaskDelay(pdMS_TO_TICKS(200));
    }
}
