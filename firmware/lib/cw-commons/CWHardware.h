#pragma once

namespace cw {
namespace hardware {
#if defined(CW_BOARD_ANIMATED_PIXEL_S3)
// Confirmed AnimatedPixelClock wiring for one 64x64 HUB75E panel.
constexpr int PinR1 = 1, PinG1 = 2, PinB1 = 4;
constexpr int PinR2 = 5, PinG2 = 6, PinB2 = 7;
constexpr int PinA = 8, PinB = 9, PinC = 10, PinD = 11, PinE = 12;
constexpr int PinClock = 13, PinLatch = 14, PinOe = 38;
// GPIO2 is PinG1. The original ESP32 status LED must remain disabled.
constexpr int StatusLed = -1;
constexpr int LdrPin = 3;
constexpr int DefaultPanelDriver = 2;  // FM6126A initialization for 64x64 panels.
constexpr const char *DefaultTimezone = "Asia/Shanghai";
constexpr const char *DefaultPosixTz = "CST-8";
constexpr const char *DefaultNtpServer = "ntp.aliyun.com";
#else
constexpr int StatusLed = 2;
constexpr int LdrPin = 35;
constexpr int DefaultPanelDriver = 0;
constexpr int PinE = 18;
constexpr const char *DefaultTimezone = "America/Sao_Paulo";
constexpr const char *DefaultPosixTz = "";
constexpr const char *DefaultNtpServer = "time.google.com";
#endif
}  // namespace hardware
}  // namespace cw
