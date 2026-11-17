#if defined(CW_PANEL_DIAGNOSTIC)
#include <Arduino.h>
#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>
#include <esp_rom_gpio.h>
#include <soc/gpio_sig_map.h>
#include <soc/lcd_cam_struct.h>
#include "../lib/cw-commons/CWHardware.h"

MatrixPanel_I2S_DMA *panel = nullptr;
unsigned long lastChange = 0;
uint8_t step = 255;

void showTest(uint8_t index)
{
  const uint8_t variant = index / 5;
  const uint8_t color = index % 5;
  const bool inverted = variant >= 2;
  const uint8_t divider = (variant % 2 == 0) ? 40 : 16;
  const uint8_t mhz = 160 / divider;
  // Set the actual LCD_CAM clock, bypassing the old driver's speed rounding.
  LCD_CAM.lcd_clock.lcd_clkm_div_num = divider;
  esp_rom_gpio_connect_out_signal(cw::hardware::PinClock, LCD_PCLK_IDX, inverted, false);
  panel->clearScreen();
  const uint16_t colors[] = {0xf800, 0x07e0, 0x001f, 0xffff};
  const char *names[] = {"RED", "GREEN", "BLUE", "WHITE", "RGB"};
  if (color < 4) {
    panel->fillRect(0, 12, 64, 52, colors[color]);
  } else {
    panel->fillRect(0, 12, 21, 52, colors[0]);
    panel->fillRect(21, 12, 21, 52, colors[1]);
    panel->fillRect(42, 12, 22, 52, colors[2]);
  }
  panel->setTextColor(0xffff);
  panel->setTextSize(1);
  panel->setCursor(0, 1);
  panel->printf("%uM P%u", mhz, inverted ? 1 : 0);
  Serial.printf("[PANEL TEST] %uMHz phase=%u pattern=%s; next change in 5 seconds\n",
                mhz, inverted ? 1 : 0, names[color]);
}

void setup()
{
  Serial.begin(115200);
  delay(500);
  HUB75_I2S_CFG config(64, 64, 1);
  config.gpio = {
    cw::hardware::PinR1, cw::hardware::PinG1, cw::hardware::PinB1,
    cw::hardware::PinR2, cw::hardware::PinG2, cw::hardware::PinB2,
    cw::hardware::PinA, cw::hardware::PinB, cw::hardware::PinC,
    cw::hardware::PinD, cw::hardware::PinE,
    cw::hardware::PinLatch, cw::hardware::PinOe, cw::hardware::PinClock
  };
  config.driver = HUB75_I2S_CFG::FM6126A;
  config.i2sspeed = HUB75_I2S_CFG::HZ_8M;
  config.clkphase = false;
  panel = new MatrixPanel_I2S_DMA(config);
  if (!panel->begin()) {
    Serial.println("[PANEL TEST] DMA initialization failed");
    while (true) delay(1000);
  }
  panel->setBrightness8(24);
  Serial.println("[PANEL TEST] No Wi-Fi or clock code. Confirmed GPIO map. FM6126A init. Brightness 24/255.");
}

void loop()
{
  if (step == 255 || millis() - lastChange >= 5000) {
    step = (step == 255) ? 0 : (step + 1) % 20;
    showTest(step);
    lastChange = millis();
  }
  delay(5);
}
#endif
