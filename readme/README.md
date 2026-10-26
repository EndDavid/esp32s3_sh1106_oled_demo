# ESP32-S3 + 1.3 寸 OLED（I2C）示例

一个可以直接编译烧录的 ESP-IDF 工程：在 128×64 的 4 针 I2C OLED 上显示开机画面、运行时间、引脚信息和进度条。

- 芯片：ESP32-S3（N16R8）
- 屏幕：1.3 英寸、4 针、I2C、128×64
- 驱动：内置 SSD1306 / SH1106 驱动（无需安装第三方组件）
- 字库：内置 5×7 ASCII 字库，支持整数倍放大

> 完整的使用说明（环境配置、命令行编译、API、FAQ、进阶方向）见 **[使用说明.md](使用说明.md)**。

## 1. 接线

4 针 OLED 的丝印顺序不同厂家可能不一样，请以模块背面的丝印为准：

| OLED 引脚 | 接到 ESP32-S3 |
| --------- | ------------- |
| VCC / VDD | 3V3（不要接 5V） |
| GND       | GND |
| SCL / SCK | GPIO9（默认） |
| SDA       | GPIO8（默认） |

> 本工程默认使用 **SDA = GPIO8、SCL = GPIO9**。如果你的接法不同，见第 5 节修改。
>
> N16R8 的 GPIO33-GPIO37 被 Octal PSRAM 占用，不要使用。建议用 GPIO1-GPIO18、GPIO21、GPIO38~GPIO48 中未被占用的引脚。

## 2. 安装/确认 ESP-IDF 环境

如果还没有装好 ESP-IDF：

1. 按 `Ctrl+Shift+P`，输入并执行 **ESP-IDF: Configure ESP-IDF Extension**。
2. 选择 **Express**。
3. 安装目录建议选一个**没有空格、没有中文**的路径，例如 `D:\Espressif`。
4. 版本选择最新的稳定版，Target 选 `esp32s3`，等待安装完成。

> 只安装 VS Code 的 ESP-IDF 插件是不够的，还必须通过上面的步骤把 ESP-IDF 框架、工具链和 Python 环境装好。

## 3. 用 VS Code 打开工程

选择 **文件 → 打开文件夹**，打开`esp32s3_sh1106_oled_demo`。

## 4. 设置芯片、串口并烧录

1. 底部状态栏点击芯片图标，或执行 **ESP-IDF: Set Espressif Device Target**，选择 `esp32s3`。
2. 用 USB 线连接开发板。优先使用板上标注 **UART / COM** 的 USB 口（通常经过 CH340 等串口芯片）。
3. 底部状态栏选择对应的 COM 口（例如 `COM5`）。
   - 如果列表里没有 COM 口，需要先安装 USB 转串口驱动（CH340 或 CP210x）。
4. 执行 **ESP-IDF: Build your project**（或底部火焰图标旁边的锤子）。
5. 执行 **ESP-IDF: Select Flash Method** 可保持默认 `UART`。
6. 执行 **ESP-IDF: Flash your project**，然后 **ESP-IDF: Monitor your device**。
7. 也可以直接执行 **ESP-IDF: Build, Flash and Start a Monitor on Your Device** 一键完成。

烧录时如果提示无法连接：

- 按住板上的 **BOOT** 键，点一下 **RST/EN** 键，再松开 BOOT，然后重新烧录。
- 换一根**能传数据**的 USB 线（很多线只能充电）。

## 5. 修改引脚 / 屏幕型号 / I2C 地址

执行 **ESP-IDF: SDK Configuration Editor (menuconfig)**：

```text
Component config
  └── OLED Demo Configuration
        ├── OLED I2C SDA GPIO        (默认 8)
        ├── OLED I2C SCL GPIO        (默认 9)
        ├── I2C port number          (默认 0)
        ├── OLED I2C address         (默认 0x3C，可试 0x3D)
        ├── I2C clock frequency      (默认 400000)
        └── OLED controller          (默认 SH1106，可切 SSD1306)
```

保存后重新编译、烧录即可。

- **1.3 寸屏通常是 SH1106**；如果图像整体偏移 2 像素或显示异常，切换成 SSD1306 试试。
- 如果屏幕完全不亮，先把地址在 `0x3C` / `0x3D` 之间换一下。
- 如果连地址探测都失败，串口会打印 `no ACK from OLED at 0x3C`，优先检查 SDA/SCL 是否接反、VCC 是否接 3V3。

## 6. 代码结构

```text
oled_demo/
├── CMakeLists.txt
├── sdkconfig.defaults        # 默认目标 esp32s3、16MB Flash
└── main/
    ├── CMakeLists.txt
    ├── Kconfig.projbuild     # menuconfig 中的配置项
    ├── main.c                # 演示程序
    ├── oled.c / oled.h       # OLED 驱动（I2C + 显存 + 绘图 API）
    └── font5x7.h             # 5x7 ASCII 字库
```

常用绘图 API（`oled.h`）：

```c
oled_clear();                              // 清屏（清显存）
oled_draw_pixel(x, y, true);               // 画点
oled_draw_hline(x, y, w, true);            // 水平线
oled_draw_vline(x, y, h, true);            // 垂直线
oled_draw_rect(x, y, w, h, true);          // 矩形边框
oled_draw_string(x, y, "Hello");           // 5x7 文本
oled_draw_string_scale(x, y, "HELLO", 2);  // 2 倍放大文本
oled_display();                            // 刷新到屏幕
```

修改 `main.c` 里的 `app_main()` 就能显示你自己的内容。字库仅支持 ASCII，显示中文需要另外加入中文字模。

## 7. 常见问题

| 现象 | 可能原因 / 解决 |
| ---- | ---------------- |
| 屏幕完全不亮 | 检查 VCC(3V3)、GND；确认 I2C 地址 0x3C/0x3D；确认 SDA/SCL 没接反 |
| 串口打印 `no ACK from OLED` | 接线错误、屏幕没供电、地址不对 |
| 屏幕亮了但花屏 | 控制器型号选错，在 menuconfig 里切换 SH1106 / SSD1306 |
| 图像整体偏左/偏右 2 像素 | 切换控制器型号；SH1106 已内置 2 列偏移补偿 |
| 烧录失败 | 按住 BOOT + 点 RST 进入下载模式；换数据线；确认 COM 口正确 |
| 打开监视器没有任何输出 | 本工程已默认把控制台设为 USB Serial/JTAG（适配板载原生 USB 口）。如果你改用板载 UART 口，请在 menuconfig 中把 Console 改回 UART0 |
| 编译提示找不到 `driver/i2c_master.h` | ESP-IDF 版本太旧（需要 v5.2 及以上），请升级 ESP-IDF；IDF 6.x 需要在 `main/CMakeLists.txt` 中依赖 `esp_driver_i2c`（本工程已配置） |
