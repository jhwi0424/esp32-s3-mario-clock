#include <Arduino.h>
#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>

// Clockface
#include <Clockface.h>
// Commons
#include <WiFiController.h>
#include <CWDateTime.h>
#include <CWPreferences.h>
#include <CWWebServer.h>
#include <StatusController.h>
#include <CWHardware.h>
#include <utility>

#define MIN_BRIGHT_DISPLAY_ON 4
#define MIN_BRIGHT_DISPLAY_OFF 0

MatrixPanel_I2S_DMA *dma_display = nullptr;

Clockface *clockface;

WiFiController wifi;
CWDateTime cwDateTime;

bool autoBrightEnabled;
long autoBrightMillis = 0;
uint8_t currentBrightSlot = -1;

bool isValidI2SSpeed(uint32_t speed) {
  return speed == 8000000 || speed == 16000000 || speed == 20000000;
}

bool isValidDriver(uint32_t drv) {
  return drv >= 0 && drv <= 5;
}



void displaySetup(bool swapBlueGreen, bool swapBlueRed, uint8_t displayBright, uint8_t displayRotation, uint8_t driver, uint32_t i2cSpeed, uint8_t E_pin)
{
  HUB75_I2S_CFG mxconfig(64, 64, 1);

#if defined(CW_BOARD_ANIMATED_PIXEL_S3)
  mxconfig.gpio = {
    cw::hardware::PinR1, cw::hardware::PinG1, cw::hardware::PinB1,
    cw::hardware::PinR2, cw::hardware::PinG2, cw::hardware::PinB2,
    cw::hardware::PinA, cw::hardware::PinB, cw::hardware::PinC,
    cw::hardware::PinD, cw::hardware::PinE,
    cw::hardware::PinLatch, cw::hardware::PinOe, cw::hardware::PinClock
  };
#endif

  if (swapBlueGreen)
  {
    // Swap Blue and Green pins because the panel is RBG instead of RGB.
    std::swap(mxconfig.gpio.b1, mxconfig.gpio.g1);
    std::swap(mxconfig.gpio.b2, mxconfig.gpio.g2);
  }

  if (swapBlueRed)
  {
    // Swap Blue and Red pins. 
    std::swap(mxconfig.gpio.b1, mxconfig.gpio.r1);
    std::swap(mxconfig.gpio.b2, mxconfig.gpio.r2);
  }

#if defined(CW_BOARD_ANIMATED_PIXEL_S3)
  mxconfig.gpio.e = cw::hardware::PinE;
#else
  mxconfig.gpio.e = E_pin;
#endif
  mxconfig.clkphase = false;

  if (isValidDriver(driver)) {
    mxconfig.driver = static_cast<HUB75_I2S_CFG::shift_driver>(driver);
  } else {
    Serial.printf("[ERROR] Invalid driver from config:%d\n", driver);
  }
  if (isValidI2SSpeed(i2cSpeed)) {
    mxconfig.i2sspeed = static_cast<HUB75_I2S_CFG::clk_speed>(i2cSpeed);
  } else {
    Serial.printf("[ERROR] Invalid I2S speed from config:%d\n", i2cSpeed);
  }

  // Display Setup
  dma_display = new MatrixPanel_I2S_DMA(mxconfig);
  if (!dma_display->begin()) {
    Serial.println("[ERROR] HUB75 DMA initialization failed");
    while (true) { delay(1000); }
  }
  Serial.printf("[HUB75] 64x64 initialized, brightness=%u, driver=%u, clock=%lu\n",
                displayBright, driver, static_cast<unsigned long>(i2cSpeed));
  dma_display->setBrightness8(displayBright);
  dma_display->clearScreen();
  dma_display->setRotation(displayRotation);
}

void automaticBrightControl()
{
  if (autoBrightEnabled) {
    if (millis() - autoBrightMillis > 3000)
    {
      int16_t currentValue = analogRead(ClockwiseParams::getInstance()->ldrPin);

      uint16_t ldrMin = ClockwiseParams::getInstance()->autoBrightMin;
      uint16_t ldrMax = ClockwiseParams::getInstance()->autoBrightMax;

      const uint8_t minBright = (currentValue < ldrMin ? MIN_BRIGHT_DISPLAY_OFF : MIN_BRIGHT_DISPLAY_ON);
      uint8_t maxBright = ClockwiseParams::getInstance()->displayBright;

      uint8_t slots = 10; //10 slots
      uint8_t mapLDR = map(currentValue > ldrMax ? ldrMax : currentValue, ldrMin, ldrMax, 1, slots);
      uint8_t mapBright = map(mapLDR, 1, slots, minBright, maxBright);

      // Serial.printf("LDR: %d, mapLDR: %d, Bright: %d\n", currentValue, mapLDR, mapBright);
      if(abs(currentBrightSlot - mapLDR ) >= 2 || mapBright == 0){
           dma_display->setBrightness8(mapBright);
           currentBrightSlot=mapLDR;
          //  Serial.printf("setBrightness: %d , Update currentBrightSlot to %d\n", mapBright, mapLDR);
      }
      autoBrightMillis = millis();
    }
  }
}

void setup()
{
  Serial.begin(115200);
#if defined(CW_BOARD_ANIMATED_PIXEL_S3)
  delay(500);
  Serial.printf("[Mario-S3] Flash=%lu PSRAM=%lu; HUB75 R1/G1/B1=1/2/4 R2/G2/B2=5/6/7 A/B/C/D/E=8/9/10/11/12 CLK/LAT/OE=13/14/38\n",
                static_cast<unsigned long>(ESP.getFlashChipSize()),
                static_cast<unsigned long>(ESP.getPsramSize()));
#endif
  if (cw::hardware::StatusLed >= 0)
    pinMode(cw::hardware::StatusLed, OUTPUT);

  StatusController::getInstance()->blink_led(5, 100);

  ClockwiseParams::getInstance()->load();

  if (ClockwiseParams::getInstance()->autoBrightMax > 0)
    pinMode(ClockwiseParams::getInstance()->ldrPin, INPUT);

  uint8_t driver = ClockwiseParams::getInstance()->driver;
  uint32_t i2cSpeed = ClockwiseParams::getInstance()->i2cSpeed;
  uint8_t E_pin = ClockwiseParams::getInstance()->E_pin;
  
  displaySetup(ClockwiseParams::getInstance()->swapBlueGreen, ClockwiseParams::getInstance()->swapBlueRed, ClockwiseParams::getInstance()->displayBright, ClockwiseParams::getInstance()->displayRotation, driver, i2cSpeed, E_pin);
  clockface = new Clockface(dma_display);

  autoBrightEnabled = (ClockwiseParams::getInstance()->autoBrightMax > 0);

  StatusController::getInstance()->clockwiseLogo();
  delay(1000);

  StatusController::getInstance()->wifiConnecting();
  wifi.begin(); // non-blocking; provisioning continues in loop()
}

// Runs once, as soon as Wi-Fi is up (stored credentials, Improv or AP portal).
void onNetworkReady()
{
  StatusController::getInstance()->ntpConnecting();
  cwDateTime.begin(ClockwiseParams::getInstance()->timeZone.c_str(), 
      ClockwiseParams::getInstance()->use24hFormat, 
      ClockwiseParams::getInstance()->ntpServer.c_str(),
      ClockwiseParams::getInstance()->manualPosix.c_str());
  clockface->setup(&cwDateTime);
  Serial.printf("[Mario] Clockface started. Local time: %s\n", cwDateTime.getFormattedTime().c_str());
}

void loop()
{
  cwDateTime.update();

  if (wifi.update()) // also services Improv Serial on every pass
  {
    onNetworkReady();
  }

  if (wifi.isConnected())
  {
    ClockwiseWebServer::getInstance()->handleHttpRequest();
    ezt::events();
  }

  if (wifi.connectionSucessfulOnce)
  {
    clockface->update();
  }

  automaticBrightControl();
}
