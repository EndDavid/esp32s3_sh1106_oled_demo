#include "oled.h"

#include <string.h>

#include "driver/i2c_master.h"
#include "esp_check.h"
#include "esp_log.h"
#include "sdkconfig.h"

#include "font5x7.h"

#define OLED_WIDTH          128
#define OLED_HEIGHT         64
#define OLED_PAGES          (OLED_HEIGHT / 8)
#define OLED_I2C_TIMEOUT_MS 100

/* I2C 控制字节：0x00 = 命令，0x40 = 显存数据 */
#define OLED_CTRL_CMD       0x00
#define OLED_CTRL_DATA      0x40

/* SH1106 的显存是 132 列，可见区域从第 2 列开始 */
#if defined(CONFIG_OLED_CTRL_SH1106)
#define OLED_COL_OFFSET     2
#else
#define OLED_COL_OFFSET     0
#endif

static const char *TAG = "oled";

static i2c_master_bus_handle_t s_bus = NULL;
static i2c_master_dev_handle_t s_dev = NULL;
static uint8_t s_fb[OLED_WIDTH * OLED_PAGES];

static esp_err_t oled_write(uint8_t control, const uint8_t *data, size_t len)
{
    uint8_t buf[1 + OLED_WIDTH];

    if (data == NULL || len == 0 || len > OLED_WIDTH) {
        return ESP_ERR_INVALID_ARG;
    }

    buf[0] = control;
    memcpy(&buf[1], data, len);

    return i2c_master_transmit(s_dev, buf, len + 1, OLED_I2C_TIMEOUT_MS);
}

static esp_err_t oled_write_cmds(const uint8_t *cmds, size_t len)
{
    return oled_write(OLED_CTRL_CMD, cmds, len);
}

esp_err_t oled_init(void)
{
    if (s_dev != NULL) {
        return ESP_OK;
    }

    /* 1. 创建 I2C 主机总线 */
    i2c_master_bus_config_t bus_cfg = {
        .i2c_port = CONFIG_OLED_I2C_PORT,
        .sda_io_num = CONFIG_OLED_SDA_GPIO,
        .scl_io_num = CONFIG_OLED_SCL_GPIO,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };
    ESP_RETURN_ON_ERROR(i2c_new_master_bus(&bus_cfg, &s_bus), TAG, "create I2C bus failed");

    /* 2. 挂载 OLED 设备 */
    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = CONFIG_OLED_I2C_ADDR,
        .scl_speed_hz = CONFIG_OLED_I2C_SPEED_HZ,
    };
    ESP_RETURN_ON_ERROR(i2c_master_bus_add_device(s_bus, &dev_cfg, &s_dev), TAG,
                        "add OLED device failed");

    /* 3. 探测地址，能收到 ACK 才说明接线正确 */
    esp_err_t err = i2c_master_probe(s_bus, CONFIG_OLED_I2C_ADDR, OLED_I2C_TIMEOUT_MS);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "no ACK from OLED at 0x%02X", (unsigned)CONFIG_OLED_I2C_ADDR);
        ESP_LOGE(TAG, "check VCC/GND, SDA/SCL wiring and the I2C address (0x3C or 0x3D)");
        return err;
    }
    ESP_LOGI(TAG, "OLED found at 0x%02X", (unsigned)CONFIG_OLED_I2C_ADDR);

    /* 4. 发送初始化序列 */
#if defined(CONFIG_OLED_CTRL_SH1106)
    static const uint8_t init_cmds[] = {
        0xAE,             /* display off */
        0xD5, 0x80,       /* clock divide ratio */
        0xA8, 0x3F,       /* multiplex ratio 1/64 */
        0xD3, 0x00,       /* display offset */
        0x40,             /* start line 0 */
        0xAD, 0x8B,       /* SH1106: DC-DC on */
        0xA1,             /* segment remap */
        0xC8,             /* COM scan direction reversed */
        0xDA, 0x12,       /* COM pins configuration */
        0x81, 0xCF,       /* contrast */
        0xD9, 0xF1,       /* pre-charge period */
        0xDB, 0x40,       /* VCOMH */
        0xA4,             /* display from RAM */
        0xA6,             /* normal (not inverted) */
        0x2E,             /* deactivate scroll */
        0xAF,             /* display on */
    };
#else
    static const uint8_t init_cmds[] = {
        0xAE,             /* display off */
        0xD5, 0x80,       /* clock divide ratio */
        0xA8, 0x3F,       /* multiplex ratio 1/64 */
        0xD3, 0x00,       /* display offset */
        0x40,             /* start line 0 */
        0x8D, 0x14,       /* SSD1306: charge pump on */
        0x20, 0x02,       /* page addressing mode */
        0xA1,             /* segment remap */
        0xC8,             /* COM scan direction reversed */
        0xDA, 0x12,       /* COM pins configuration */
        0x81, 0xCF,       /* contrast */
        0xD9, 0xF1,       /* pre-charge period */
        0xDB, 0x40,       /* VCOMH */
        0xA4,             /* display from RAM */
        0xA6,             /* normal (not inverted) */
        0x2E,             /* deactivate scroll */
        0xAF,             /* display on */
    };
#endif

    ESP_RETURN_ON_ERROR(oled_write_cmds(init_cmds, sizeof(init_cmds)), TAG, "OLED init failed");

    oled_clear();
    oled_display();

    return ESP_OK;
}

void oled_clear(void)
{
    memset(s_fb, 0, sizeof(s_fb));
}

void oled_display(void)
{
    if (s_dev == NULL) {
        return;
    }

    for (int page = 0; page < OLED_PAGES; page++) {
        uint8_t col = OLED_COL_OFFSET;
        uint8_t cmds[3] = {
            (uint8_t)(0xB0 | page),        /* page address */
            (uint8_t)(col & 0x0F),         /* lower column address */
            (uint8_t)(0x10 | (col >> 4)),  /* higher column address */
        };

        if (oled_write_cmds(cmds, sizeof(cmds)) != ESP_OK) {
            return;
        }
        if (oled_write(OLED_CTRL_DATA, &s_fb[page * OLED_WIDTH], OLED_WIDTH) != ESP_OK) {
            return;
        }
    }
}

void oled_draw_pixel(int x, int y, bool on)
{
    if (x < 0 || x >= OLED_WIDTH || y < 0 || y >= OLED_HEIGHT) {
        return;
    }

    uint8_t *byte = &s_fb[(y / 8) * OLED_WIDTH + x];
    uint8_t mask = (uint8_t)(1u << (y % 8));

    if (on) {
        *byte |= mask;
    } else {
        *byte &= (uint8_t)~mask;
    }
}

void oled_draw_hline(int x, int y, int w, bool on)
{
    for (int i = 0; i < w; i++) {
        oled_draw_pixel(x + i, y, on);
    }
}

void oled_draw_vline(int x, int y, int h, bool on)
{
    for (int i = 0; i < h; i++) {
        oled_draw_pixel(x, y + i, on);
    }
}

void oled_draw_rect(int x, int y, int w, int h, bool on)
{
    if (w <= 0 || h <= 0) {
        return;
    }

    oled_draw_hline(x, y, w, on);
    oled_draw_hline(x, y + h - 1, w, on);
    oled_draw_vline(x, y, h, on);
    oled_draw_vline(x + w - 1, y, h, on);
}

static void oled_draw_char(int x, int y, char c, uint8_t scale)
{
    if (c < FONT_FIRST_CHAR || c > FONT_LAST_CHAR) {
        c = '?';
    }

    const uint8_t *glyph = &font5x7[(c - FONT_FIRST_CHAR) * FONT_WIDTH];

    for (int col = 0; col < FONT_WIDTH; col++) {
        uint8_t column = glyph[col];

        for (int row = 0; row < FONT_HEIGHT; row++) {
            if ((column & (1u << row)) == 0) {
                continue;
            }

            for (int dx = 0; dx < scale; dx++) {
                for (int dy = 0; dy < scale; dy++) {
                    oled_draw_pixel(x + col * scale + dx,
                                    y + row * scale + dy,
                                    true);
                }
            }
        }
    }
}

void oled_draw_string_scale(int x, int y, const char *text, uint8_t scale)
{
    if (text == NULL || scale == 0) {
        return;
    }

    int cursor_x = x;
    while (*text != '\0') {
        oled_draw_char(cursor_x, y, *text, scale);
        cursor_x += (FONT_WIDTH + 1) * scale;
        text++;
    }
}

void oled_draw_string(int x, int y, const char *text)
{
    oled_draw_string_scale(x, y, text, 1);
}
