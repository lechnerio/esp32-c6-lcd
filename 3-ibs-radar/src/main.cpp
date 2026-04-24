#include <Arduino.h>
#include <Arduino_GFX_Library.h>
#include <BLEAdvertisedDevice.h>
#include <BLEDevice.h>
#include <BLEScan.h>
#include <algorithm>
#include <map>
#include <math.h>
#include <vector>

#define LCD_MOSI 6
#define LCD_SCLK 7
#define LCD_CS   14
#define LCD_DC   15
#define LCD_RST  21
#define LCD_BL   22

#define BTN_BOOT     9
#define DEBOUNCE_MS 30

#define WINDOW_SIZE   10
#define FRESH_MS    2000
#define STALE_MS    5000
#define LOST_MS    10000
#define EVICT_MS   30000

#define RSSI_AT_1M    -59.0f
#define PATH_LOSS_EXP  2.5f

#define MAX_ROWS   11
#define ROW_HEIGHT 10
#define HEADER_PX  14
#define FOOTER_PX  12
#define MARGIN_X    6
#define MARGIN_Y    4

struct BeaconState {
  String name;
  String addr;
  String addrSuffix;
  int8_t rssiWindow[WINDOW_SIZE];
  uint8_t windowCount;
  uint8_t windowHead;
  int8_t lastRssi;
  uint32_t lastSeenMs;
};

Arduino_DataBus *bus = new Arduino_ESP32SPI(LCD_DC, LCD_CS, LCD_SCLK, LCD_MOSI, GFX_NOT_DEFINED);
Arduino_GFX *gfx = new Arduino_ST7789(bus, LCD_RST, 3, true, 172, 320, 34, 0, 34, 0);

BLEScan *scanner = nullptr;
std::map<String, BeaconState> beacons;
SemaphoreHandle_t beaconsMutex = nullptr;

int lastBtnState = HIGH;
uint32_t lastBtnChange = 0;
uint32_t lastRenderMs = 0;

int rowsAreaY() { return HEADER_PX + MARGIN_Y + 8; }
int rowsAreaH() { return gfx->height() - rowsAreaY() - FOOTER_PX - MARGIN_Y - 2; }

void pushSample(BeaconState &b, int8_t rssi) {
  b.rssiWindow[b.windowHead] = rssi;
  b.windowHead = (b.windowHead + 1) % WINDOW_SIZE;
  if (b.windowCount < WINDOW_SIZE) b.windowCount++;
  b.lastRssi = rssi;
  b.lastSeenMs = millis();
}

int8_t medianRssi(const BeaconState &b) {
  if (b.windowCount == 0) return 0;
  int8_t tmp[WINDOW_SIZE];
  for (uint8_t i = 0; i < b.windowCount; i++) tmp[i] = b.rssiWindow[i];
  std::sort(tmp, tmp + b.windowCount);
  return tmp[b.windowCount / 2];
}

float estimateDistanceMeters(int rssi) {
  return powf(10.0f, (RSSI_AT_1M - (float)rssi) / (10.0f * PATH_LOSS_EXP));
}

void formatDistance(float meters, char *out, size_t n) {
  if (meters < 1.0f)       snprintf(out, n, "~%3dcm", (int)roundf(meters * 100.0f));
  else if (meters < 10.0f) snprintf(out, n, "~%.1fm", meters);
  else                     snprintf(out, n, "~%3dm", (int)roundf(meters));
}

uint16_t freshnessColor(uint32_t ageMs) {
  if (ageMs < FRESH_MS) return RGB565_GREEN;
  if (ageMs < STALE_MS) return RGB565_YELLOW;
  if (ageMs < LOST_MS)  return RGB565_ORANGE;
  return RGB565_RED;
}

class BeaconCallbacks : public BLEAdvertisedDeviceCallbacks {
  void onResult(BLEAdvertisedDevice d) override {
    String addr = String(d.getAddress().toString().c_str());
    String name = d.haveName() ? String(d.getName().c_str()) : String("");
    int8_t rssi = d.getRSSI();

    static uint32_t debugCount = 0;
    if (debugCount < 40) {
      Serial.printf("pkt %s rssi=%d name=[%s] mfg=%u\n",
                    addr.c_str(), rssi, name.c_str(),
                    d.haveManufacturerData() ? (unsigned)d.getManufacturerData().length() : 0U);
      debugCount++;
    }

    if (!name.startsWith("iBS")) return;

    Serial.printf("[%lu] iBS pkt  %s  %s  rssi=%d\n",
                  (unsigned long)millis(), name.c_str(), addr.c_str(), rssi);

    xSemaphoreTake(beaconsMutex, portMAX_DELAY);
    auto &b = beacons[addr];
    if (b.name.length() == 0) {
      b.name = name;
      b.addr = addr;
      if (addr.length() >= 17) {
        String suffix = addr.substring(9);
        suffix.replace(":", "");
        suffix.toUpperCase();
        b.addrSuffix = suffix;
      } else {
        b.addrSuffix = addr;
      }
    }
    pushSample(b, rssi);
    xSemaphoreGive(beaconsMutex);
  }
};

BeaconCallbacks beaconCallbacks;

void drawHeader(int tracked, int fresh) {
  gfx->fillRect(0, 0, gfx->width(), rowsAreaY() - 2, RGB565_BLACK);
  gfx->setTextSize(1);
  gfx->setTextColor(RGB565_CYAN);
  gfx->setCursor(MARGIN_X, MARGIN_Y);
  gfx->printf("iBS radar  %d tracked  %d fresh  [iBS]", tracked, fresh);
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

struct RenderRow {
  String name;
  String addrSuffix;
  int8_t medRssi;
  uint8_t samples;
  uint32_t ageMs;
};

void renderList() {
  lastRenderMs = millis();
  uint32_t now = millis();

  std::vector<RenderRow> rows;
  int tracked = 0;
  int freshCount = 0;

  xSemaphoreTake(beaconsMutex, portMAX_DELAY);
  for (auto it = beacons.begin(); it != beacons.end(); ) {
    if (now - it->second.lastSeenMs > EVICT_MS) {
      it = beacons.erase(it);
      continue;
    }
    RenderRow r;
    r.name = it->second.name;
    r.addrSuffix = it->second.addrSuffix;
    r.medRssi = medianRssi(it->second);
    r.samples = it->second.windowCount;
    r.ageMs = now - it->second.lastSeenMs;
    rows.push_back(r);
    ++it;
  }
  tracked = (int)beacons.size();
  xSemaphoreGive(beaconsMutex);

  for (auto &r : rows) {
    if (r.ageMs < STALE_MS) freshCount++;
  }

  std::sort(rows.begin(), rows.end(),
            [](const RenderRow &a, const RenderRow &b) { return a.medRssi > b.medRssi; });

  drawHeader(tracked, freshCount);
  gfx->fillRect(0, rowsAreaY(), gfx->width(), rowsAreaH(), RGB565_BLACK);

  const int distCol = gfx->width() - MARGIN_X - 36;
  const int rssiCol = distCol - 8 - 42;

  if (rows.empty()) {
    gfx->setCursor(MARGIN_X, rowsAreaY());
    gfx->setTextColor(RGB565_WHITE);
    gfx->println("Waiting for iBS03 / iBS05 beacons...");
    return;
  }

  int shown = std::min((int)rows.size(), MAX_ROWS);
  for (int i = 0; i < shown; i++) {
    const RenderRow &r = rows[i];
    int y = rowsAreaY() + i * ROW_HEIGHT;
    uint16_t color = freshnessColor(r.ageMs);

    gfx->setTextColor(color);
    gfx->setCursor(MARGIN_X, y);
    gfx->printf("%s %s", r.name.c_str(), r.addrSuffix.c_str());

    gfx->setCursor(rssiCol, y);
    gfx->printf("%4ddBm", r.medRssi);

    gfx->setCursor(distCol, y);
    if (r.ageMs < LOST_MS) {
      float m = estimateDistanceMeters(r.medRssi);
      char buf[12];
      formatDistance(m, buf, sizeof(buf));
      gfx->print(buf);
    } else {
      gfx->print("(lost)");
    }
  }
}

bool pollButton() {
  int s = digitalRead(BTN_BOOT);
  uint32_t now = millis();
  if (s != lastBtnState && (now - lastBtnChange) > DEBOUNCE_MS) {
    lastBtnChange = now;
    lastBtnState = s;
    if (s == LOW) {
      xSemaphoreTake(beaconsMutex, portMAX_DELAY);
      beacons.clear();
      xSemaphoreGive(beaconsMutex);
      Serial.println("Beacons cleared");
      renderList();
      return true;
    }
  }
  return false;
}

void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.println("iBS radar booting...");

  pinMode(LCD_BL, OUTPUT);
  digitalWrite(LCD_BL, HIGH);
  pinMode(BTN_BOOT, INPUT_PULLUP);

  beaconsMutex = xSemaphoreCreateMutex();

  gfx->begin();
  gfx->fillScreen(RGB565_BLACK);
  gfx->setTextWrap(false);
  gfx->setTextSize(1);
  gfx->setTextColor(RGB565_CYAN);
  gfx->setCursor(MARGIN_X, MARGIN_Y);
  gfx->println("iBS radar");
  gfx->setCursor(MARGIN_X, MARGIN_Y + ROW_HEIGHT);
  gfx->setTextColor(RGB565_WHITE);
  gfx->println("Initializing BLE...");

  BLEDevice::init("esp32c6-ibs-radar");
  scanner = BLEDevice::getScan();
  scanner->setAdvertisedDeviceCallbacks(&beaconCallbacks, true);
  scanner->setActiveScan(true);
  scanner->setInterval(100);
  scanner->setWindow(99);

  drawFooter("Starting scan...");
}

void loop() {
  scanner->start(1, false);
  scanner->clearResults();
  pollButton();

  if (millis() - lastRenderMs > 400) {
    renderList();
  }

  char msg[40];
  snprintf(msg, sizeof(msg), "Live  %d tracked  (BOOT: reset)", (int)beacons.size());
  drawFooter(msg);

  pollButton();
}
