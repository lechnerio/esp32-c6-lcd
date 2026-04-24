#include <Arduino.h>
#include <Arduino_GFX_Library.h>
#include <BLEDevice.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>
#include <algorithm>
#include <math.h>
#include <vector>

#define LCD_MOSI 6
#define LCD_SCLK 7
#define LCD_CS   14
#define LCD_DC   15
#define LCD_RST  21
#define LCD_BL   22

#define BTN_BOOT 9
#define DEBOUNCE_MS 30

#define SCAN_SECONDS 4
#define MAX_ROWS     12
#define ROW_HEIGHT   10
#define HEADER_PX    14
#define FOOTER_PX    12
#define MARGIN_X      6
#define MARGIN_Y      4

#define RSSI_AT_1M       -59.0f
#define PATH_LOSS_EXP     2.5f

struct BleEntry {
  String label;
  int rssi;
  bool hasName;
};

Arduino_DataBus *bus = new Arduino_ESP32SPI(LCD_DC, LCD_CS, LCD_SCLK, LCD_MOSI, GFX_NOT_DEFINED);
Arduino_GFX *gfx = new Arduino_ST7789(bus, LCD_RST, 3, true, 172, 320, 34, 0, 34, 0);

BLEScan *scanner = nullptr;
uint32_t scanNumber = 0;

std::vector<BleEntry> lastResults;
bool onlyNamed = false;
int lastBtnState = HIGH;
uint32_t lastBtnChange = 0;

uint16_t rssiColor(int rssi) {
  if (rssi >= -60) return RGB565_GREEN;
  if (rssi >= -75) return RGB565_YELLOW;
  if (rssi >= -90) return RGB565_ORANGE;
  return RGB565_RED;
}

float estimateDistanceMeters(int rssi) {
  return powf(10.0f, (RSSI_AT_1M - (float)rssi) / (10.0f * PATH_LOSS_EXP));
}

void formatDistance(float meters, char *out, size_t n) {
  if (meters < 1.0f) {
    snprintf(out, n, "~%3dcm", (int)roundf(meters * 100.0f));
  } else if (meters < 10.0f) {
    snprintf(out, n, "~%.1fm", meters);
  } else {
    snprintf(out, n, "~%3dm", (int)roundf(meters));
  }
}

int rowsAreaY() { return HEADER_PX + MARGIN_Y + 8; }
int rowsAreaH() { return gfx->height() - rowsAreaY() - FOOTER_PX - MARGIN_Y - 2; }

void drawHeader(int shown, int total) {
  gfx->fillRect(0, 0, gfx->width(), rowsAreaY() - 2, RGB565_BLACK);
  gfx->setTextSize(1);
  gfx->setTextColor(RGB565_CYAN);
  gfx->setCursor(MARGIN_X, MARGIN_Y);
  gfx->printf("BLE radar  #%lu  %d/%d  %s",
              (unsigned long)scanNumber, shown, total,
              onlyNamed ? "[named]" : "[all]");
  gfx->drawFastHLine(MARGIN_X, HEADER_PX + MARGIN_Y + 2,
                     gfx->width() - 2 * MARGIN_X, RGB565_DARKGREY);
}

void drawFooter(const char *msg) {
  int y = gfx->height() - FOOTER_PX - MARGIN_Y;
  gfx->fillRect(MARGIN_X, y, gfx->width() - 2 * MARGIN_X, FOOTER_PX, RGB565_BLUE);
  gfx->setTextSize(1);
  gfx->setTextColor(RGB565_WHITE);
  gfx->setCursor(MARGIN_X + 4, y + 2);
  gfx->print(msg);
}

void drawResults(const std::vector<BleEntry>& results, int total) {
  int shown = std::min((int)results.size(), MAX_ROWS);
  drawHeader(shown, total);

  gfx->fillRect(0, rowsAreaY(), gfx->width(), rowsAreaH(), RGB565_BLACK);

  const int distCol = gfx->width() - MARGIN_X - 36;
  const int rssiCol = distCol - 8 - 42;
  const int nameChars = (rssiCol - MARGIN_X - 4) / 6;

  for (int i = 0; i < shown; i++) {
    const BleEntry& e = results[i];
    int y = rowsAreaY() + i * ROW_HEIGHT;

    String name = e.label;
    if ((int)name.length() > nameChars) name = name.substring(0, nameChars);

    gfx->setTextSize(1);
    gfx->setCursor(MARGIN_X, y);
    gfx->setTextColor(e.hasName ? RGB565_WHITE : RGB565_LIGHTGREY);
    gfx->print(name);

    gfx->setCursor(rssiCol, y);
    gfx->setTextColor(rssiColor(e.rssi));
    gfx->printf("%4ddBm", e.rssi);

    char dist[12];
    formatDistance(estimateDistanceMeters(e.rssi), dist, sizeof(dist));
    gfx->setCursor(distCol, y);
    gfx->setTextColor(RGB565_LIGHTGREY);
    gfx->print(dist);
  }

  if (results.empty()) {
    gfx->setCursor(MARGIN_X, rowsAreaY());
    gfx->setTextColor(RGB565_WHITE);
    gfx->println(total == 0 ? "No devices found." : "No named devices.");
  }
}

void renderLast() {
  std::vector<BleEntry> filtered;
  filtered.reserve(lastResults.size());
  for (const auto& e : lastResults) {
    if (!onlyNamed || e.hasName) filtered.push_back(e);
  }
  drawResults(filtered, (int)lastResults.size());
}

bool pollButton() {
  int s = digitalRead(BTN_BOOT);
  uint32_t now = millis();
  if (s != lastBtnState && (now - lastBtnChange) > DEBOUNCE_MS) {
    lastBtnChange = now;
    lastBtnState = s;
    if (s == LOW) {
      onlyNamed = !onlyNamed;
      Serial.printf("Filter toggled: %s\n", onlyNamed ? "named only" : "all");
      renderLast();
      return true;
    }
  }
  return false;
}

void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.println("Bluetooth radar booting...");

  pinMode(LCD_BL, OUTPUT);
  digitalWrite(LCD_BL, HIGH);
  pinMode(BTN_BOOT, INPUT_PULLUP);

  gfx->begin();
  gfx->fillScreen(RGB565_BLACK);
  gfx->setTextWrap(false);
  gfx->setTextSize(1);
  gfx->setTextColor(RGB565_CYAN);
  gfx->setCursor(MARGIN_X, MARGIN_Y);
  gfx->println("BLE radar");
  gfx->setCursor(MARGIN_X, MARGIN_Y + ROW_HEIGHT);
  gfx->setTextColor(RGB565_WHITE);
  gfx->println("Initializing BLE...");

  BLEDevice::init("esp32c6-radar");
  scanner = BLEDevice::getScan();
  scanner->setActiveScan(true);
  scanner->setInterval(100);
  scanner->setWindow(99);
}

void loop() {
  scanNumber++;
  Serial.printf("Scan #%lu starting...\n", (unsigned long)scanNumber);

  scanner->clearResults();
  BLEScanResults *found = nullptr;
  char msg[24];
  for (int remaining = SCAN_SECONDS; remaining > 0; remaining--) {
    snprintf(msg, sizeof(msg), "Scanning  %d...", remaining);
    drawFooter(msg);
    pollButton();
    found = scanner->start(1, true);
    pollButton();
  }
  drawFooter("Scanning  ...");

  int n = found ? found->getCount() : 0;
  lastResults.clear();
  lastResults.reserve(n);
  for (int i = 0; i < n; i++) {
    BLEAdvertisedDevice d = found->getDevice(i);
    BleEntry e;
    if (d.haveName()) {
      e.label = String(d.getName().c_str());
      e.hasName = true;
    } else {
      e.label = String(d.getAddress().toString().c_str());
      e.hasName = false;
    }
    e.rssi = d.getRSSI();
    lastResults.push_back(e);
  }

  std::sort(lastResults.begin(), lastResults.end(),
            [](const BleEntry& a, const BleEntry& b) { return a.rssi > b.rssi; });

  renderLast();

  snprintf(msg, sizeof(msg), "Idle  %d devices  (BOOT: toggle)", n);
  drawFooter(msg);

  Serial.printf("Scan #%lu done: %d devices\n", (unsigned long)scanNumber, n);

  uint32_t until = millis() + 300;
  while (millis() < until) {
    pollButton();
    delay(10);
  }
}
