#include <Arduino.h>
#include <Arduino_GFX_Library.h>

#define LCD_MOSI  6
#define LCD_SCLK  7
#define LCD_CS    14
#define LCD_DC    15
#define LCD_RST   21
#define LCD_BL    22

#define LCD_WIDTH  172
#define LCD_HEIGHT 320

Arduino_DataBus *bus = new Arduino_ESP32SPI(LCD_DC, LCD_CS, LCD_SCLK, LCD_MOSI, GFX_NOT_DEFINED);
Arduino_GFX *gfx = new Arduino_ST7789(bus, LCD_RST, 0, true, LCD_WIDTH, LCD_HEIGHT, 34, 0, 34, 0);

void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.println("ESP32-C6 Hello World booting...");

  pinMode(LCD_BL, OUTPUT);
  digitalWrite(LCD_BL, HIGH);

  gfx->begin();
  gfx->fillScreen(BLACK);

  gfx->setCursor(10, 40);
  gfx->setTextColor(WHITE);
  gfx->setTextSize(3);
  gfx->println("Hello,");

  gfx->setCursor(10, 80);
  gfx->setTextColor(CYAN);
  gfx->println("World!");

  gfx->setCursor(10, 140);
  gfx->setTextColor(YELLOW);
  gfx->setTextSize(2);
  gfx->println("ESP32-C6");

  gfx->setCursor(10, 170);
  gfx->setTextColor(MAGENTA);
  gfx->println("Waveshare 1.47\"");
}

void loop() {
  delay(1000);
  Serial.println("tick");
}
