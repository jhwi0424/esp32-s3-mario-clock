# ESP32-S3 单屏马里奥时钟

基于 [jnthas/clockwise](https://github.com/jnthas/clockwise) 的 ESP32-S3 适配，
使用 `cw-cf-0x01` 马里奥表盘，保留上游 MIT 许可证。
表盘固定于 `a56389f4efb2fb1c531e17ef8a2fde382a32c9a0`；构建脚本自动修正字体头文件的 S3 判断，并创建库链接。

源码和预编译固件均不包含实际 Wi-Fi 名称或密码。
Wi-Fi 配置由用户首次开机时输入，保存在设备 NVS；本机配网文件、串口日志和整板备份不上传。

## 硬件与接线

- ESP32-S3，16MB Flash、8MB OPI PSRAM（N16R8）。
- 一块 64×64 HUB75E RGB 点阵屏，1/32 扫描。
- 独立、稳定的 5V 屏幕电源；与 ESP32 共地。

采用 AnimatedPixelClock 的裸板引脚分配：

| HUB75E 针脚 | 信号 | ESP32-S3 |
|---|---|---|
| 1 | R1 | GPIO1 |
| 2 | G1 | GPIO2 |
| 3 | B1 | GPIO4 |
| 4 | GND | GND |
| 5 | R2 | GPIO5 |
| 6 | G2 | GPIO6 |
| 7 | B2 | GPIO7 |
| 8 | E | GPIO12 |
| 9 | A | GPIO8 |
| 10 | B | GPIO9 |
| 11 | C | GPIO10 |
| 12 | D | GPIO11 |
| 13 | CLK | GPIO13 |
| 14 | LAT / STB | GPIO14 |
| 15 | OE | GPIO38 |
| 16 | GND | GND |

定义见 `firmware/lib/cw-commons/CWHardware.h`。GPIO2 用于 G1，停用原版状态灯。
无光敏电阻，停用自动亮度及相关网页选项。
默认亮度 32/255、24 小时制、北京时间 `Asia/Shanghai` / `CST-8`。
面板默认 FM6126A 初始化，可在设置页切换驱动。

现场曾出现仅红色正常，修复独立 5V 供电不稳后恢复彩色显示。
请先确认屏幕端电压和共地，再判断数据通道或时序。
旧版驱动的“8MHz”配置在 S3 上实际约为 10MHz，诊断程序可对照 4MHz / 10MHz 和两种 CLK 相位。

## 预编译固件

`prebuilt/esp32s3/` 提供从编译产物生成的镜像，未从已配网设备读取：

| 文件 | 烧录地址 | 用途 |
|---|---|---|
| `Mario-S3-64x64.full.bin` | `0x0` | 首次安装，包含引导程序和分区表 |
| `Mario-S3-64x64.app.bin` | `0x10000` | 更新应用，保留已存配网信息 |

完整镜像中的 NVS 区域为空；首次安装会清空其覆盖区域的原有配置。校验值见同目录 `SHA256SUMS.txt`。

```powershell
python -m pip install esptool==5.2.0
esptool --chip esp32s3 --port COM_PORT --after watchdog-reset write-flash 0x0 prebuilt/esp32s3/Mario-S3-64x64.full.bin
```

将 `COM_PORT` 改为实际串口。如果无法进入下载模式，按住 BOOT、点按 RESET，然后松开 BOOT。
烧录后释放 BOOT 并点按 RESET 即可运行。看门狗复位会使原生 USB 短暂重新枚举；
以 `Hash of data verified` 和重连后的启动日志共同确认结果。

## 从源码编译

固定使用 PlatformIO 6.2.0、espressif32 6.12.0 / Arduino-ESP32 2.0.17，S3 依赖版本已固定。
默认目标 `esp32s3-mario-usb` 使用 S3 原生 USB-Serial/JTAG；外接 USB-UART 使用 `esp32s3-mario`。

```powershell
git clone https://github.com/jhwi0424/esp32-s3-mario-clock.git
cd esp32-s3-mario-clock
git submodule update --init firmware/clockfaces/cw-cf-0x01
python -m pip install platformio==6.2.0
python -m platformio run -d firmware -e esp32s3-mario-usb
python -m platformio run -d firmware -e esp32s3-mario-usb -t upload --upload-port COM_PORT
```

Windows 自动创建目录联接，Linux/macOS 自动创建符号链接。
字体补丁仅改一行平台判断；构建后子模块可能显示该处本地修改，这是预期行为。

## 配网与使用

1. 首次开机没有已存凭据时，约 15 秒后出现 `Clockwise-Wifi` 热点。
2. 用手机连接该热点，打开 `http://192.168.4.1`，输入自己的 **2.4GHz Wi-Fi** 名称和密码。
3. 已存网络开机连接最多等待 40 秒；失败后显示 `Setup WiFi via AP`，可重新配网。
4. 联网后，通过 `http://clockwise.local` 或串口日志中的设备 IP 打开设置页。

也可通过 USB 使用 Improv Serial 配网，凭据保存在本地忽略目录：

```powershell
New-Item -ItemType Directory -Force .secrets
Copy-Item examples/wifi.example.json .secrets/wifi.json
# 只编辑 .secrets/wifi.json，填入自己的名称和密码。
python -m pip install pyserial==3.5
python tools/provision_wifi.py --port COM_PORT --credentials-file .secrets/wifi.json
Remove-Item -LiteralPath .secrets/wifi.json
```

不要将已填写的配网文件提交到仓库；例子只包含占位符。
默认 NTP 为 `ntp.aliyun.com`。NTP 超时时尝试 HTTP Date 后备校时，后台 NTP 重试仍启用。
`GET /status` 返回 Unix 时间、校时状态、Wi-Fi 信号和运行时长。
马里奥每分钟跳起更新时间。亮度、旋转、颜色顺序和面板驱动可在网页修改后重启生效。

## 测试与诊断

Linux/macOS 有 GCC 时：`pio test -d firmware -e native`。
Windows 有 Visual Studio 2022 C++ 工具时：`tools/run-native-tests.cmd`。
11 项原生测试已通过；硬件已验证联网、校时、重启自动重连和彩色马里奥显示。

独立屏幕测试仅作为可选工具，不启动 Wi-Fi：

```bash
pio run -d firmware -e esp32s3-panel-test
```

测试程序循环显示红、绿、蓝、白和 RGB 色条，每幅保持 5 秒，对照 4MHz / 10MHz、P0 / P1 四种时序。
如烧录了诊断固件，重新烧录马里奥应用即可恢复时钟。
