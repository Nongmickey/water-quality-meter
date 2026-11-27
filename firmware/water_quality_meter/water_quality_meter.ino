/*
 * เครื่องวัดคุณภาพน้ำ (Smart Water System) — ESP32
 *
 * การทำงาน
 *   1) เซนเซอร์ pH, TDS, อุณหภูมิ อ่านค่าทุก 1 วินาที แสดงบนจอ LCD 20x4
 *      เซนเซอร์ตัวไหนไม่ได้ต่อ/ไม่ได้จุ่มน้ำ จะแสดงเป็น "-"
 *   2) กดปุ่มสีเขียว (หรือกด "เริ่มวัด" บนหน้าเว็บ) -> วัดตามขั้นตอนมาตรฐาน แล้วส่งผลไปหน้าเว็บ
 *   3) หน้าเว็บ (เปิดจากมือถือ) วิเคราะห์น้ำด้วย AI, ทำกราฟ, บันทึกประวัติ วันเวลา สถานที่
 *
 * หลักการวัด
 *   - รอค่านิ่ง: อ่านทุก 0.5 วินาที ถ้า 6 ค่าล่าสุด (3 วินาที) แกว่งไม่เกินเกณฑ์ใน config.h
 *     (pH ±0.05, TDS 3%, อุณหภูมิ 0.3 °C) ถือว่านิ่ง แล้วใช้ค่าเฉลี่ย 6 ค่านั้น
 *     ถ้าเกิน 30 วินาทียังไม่นิ่ง ใช้ค่าเฉลี่ยล่าสุดและติดป้าย "ค่ายังไม่นิ่ง"
 *   - pH: ปรับเทียบ 2 จุด (บัฟเฟอร์ 6.86 และ 4.00) และชดเชยอุณหภูมิตามสมการเนิร์นสต์
 *         (ความชันของหัววัดแปรผันตามอุณหภูมิสัมบูรณ์: 59.16 mV/pH ที่ 25 °C)
 *   - TDS: ชดเชยอุณหภูมิเป็นค่าที่ 25 °C (2% ต่อ °C) แปลงด้วยสูตรของ DFRobot (ตัวคูณ TDS 0.5)
 *          แล้วคูณค่าปรับเทียบ K ที่ได้จากน้ำยามาตรฐาน
 *   - อุณหภูมิ: NTC 10k สมการ Beta (B = 3950)
 *   - เก็บผลตรวจล่าสุด 80 ครั้งไว้ในเครื่อง (ไม่หายเมื่อถอดปลั๊ก) หน้าเว็บดึงไปทำกราฟและประวัติได้
 *
 * แก้ขาและค่าต่างๆ ได้ในแท็บ config.h
 * หน้าเว็บอยู่ในไฟล์ web_page.h (สร้างจากโฟลเดอร์ web/ ด้วย tools/build_web.py)
 * ไลบรารีที่ต้องติดตั้ง: "LiquidCrystal I2C" (by Frank de Brabander)
 * ดูวิธีต่อสายใน docs/wiring-breadboard.png
 */

#include <Wire.h>
#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>
#include <LiquidCrystal_I2C.h>
#include <math.h>
#include "web_page.h"  // หน้าเว็บ (สร้างจากโฟลเดอร์ web/ — ห้ามแก้ตรงๆ)

#include "config.h"   // ขา ค่าวงจร วิธีวัด และ WiFi อยู่ในแท็บ config.h

// ================= ตัวแปรระบบ =================
LiquidCrystal_I2C *lcd = nullptr;
WebServer server(80);
Preferences prefs;

float phNeutralMv = 2500.0;  // แรงดันโมดูล pH (ก่อนแบ่ง) ในน้ำยาจุดกลาง (PH_BUFFER_MID = 6.86)
float phAcidMv    = 3030.0;  // แรงดันโมดูล pH (ก่อนแบ่ง) ในน้ำยา pH 4.00
float phCalT      = 25.0;    // อุณหภูมิน้ำยาตอนคาลิเบรต pH (°C)
float tdsK        = 1.0;     // ค่าปรับเทียบ TDS

struct Values {
  float ph = NAN, tds = NAN, temp = NAN;
};
struct Sample {
  float mv;      // ค่ากลาง
  float spread;  // ความแกว่ง (max-min ของช่วงกลาง)
};

Values live;                  // ค่าสด อัปเดตทุก 1 วินาที
float rawTdsMv = 0, rawPhMv = 0, rawTempMv = 0;

// ผลตรวจล่าสุด (จากการกดปุ่ม)
struct Reading {
  uint32_t id = 0;       // ลำดับผลตรวจ (ไม่ซ้ำ แม้ปิดเปิดเครื่อง)
  Values v;
  uint32_t epoch = 0;    // เวลาจริง (ได้จากมือถือ) 0 = ยังไม่ทราบ
  uint32_t ms = 0;       // millis() ตอนวัดเสร็จ
  bool stable = false;   // ค่านิ่งตามเกณฑ์หรือไม่
  uint8_t wait = 0;      // วินาทีที่รอค่านิ่ง
};
Reading last;
uint32_t bootId = 0;
int64_t epochOffset = 0;      // epoch = millis()/1000 + epochOffset
bool timeSet = false;

// การวัดแบบรอค่านิ่ง
bool measureRequested = false;
bool measuring = false;
unsigned long measureStart = 0, nextSampleAt = 0;
Values win[STABLE_WINDOW];
int winCount = 0, winPos = 0;

// บันทึกผลตรวจในเครื่อง (วนทับเมื่อเต็ม)
struct __attribute__((packed)) LogEntry {
  uint32_t seq;      // ลำดับผลตรวจ
  uint32_t epoch;    // เวลาจริง (0 = ไม่ทราบ)
  uint32_t boot;     // รอบที่เปิดเครื่อง
  uint32_t ms;       // millis() ตอนวัด
  int16_t  ph100;    // pH x100 (INT16_MIN = ไม่ได้วัด)
  int16_t  tds;      // ppm (-1 = ไม่ได้วัด)
  int16_t  temp10;   // °C x10 (INT16_MIN = ไม่ได้วัด)
  uint8_t  flags;    // bit0 = ค่านิ่ง
  uint8_t  wait;     // วินาทีที่รอค่านิ่ง
};
LogEntry logBuf[LOG_SIZE];
uint8_t logHead = 0, logCount = 0;

String statusLine = "";
unsigned long statusUntil = 0;
unsigned long infoUntil = 0;

// ================= อ่าน ADC =================
Sample readPin(int pin) {
  const int N = 25;
  int buf[N];
  for (int i = 0; i < N; i++) {
    buf[i] = analogReadMilliVolts(pin);
    delayMicroseconds(800);
  }
  for (int i = 1; i < N; i++) {  // insertion sort
    int v = buf[i], j = i - 1;
    while (j >= 0 && buf[j] > v) { buf[j + 1] = buf[j]; j--; }
    buf[j + 1] = v;
  }
  Sample s;
  s.mv = buf[N / 2];
  s.spread = buf[N - 4] - buf[3];  // ตัดหัวท้ายกันค่ากระโดด
  return s;
}

// ================= เซนเซอร์ =================
float readTemperature() {
  Sample s = readPin(PIN_TEMP);
  rawTempMv = s.mv;
  if (s.spread > NOISE_SPREAD_MV) return NAN;
  if (s.mv < 50 || s.mv > ADC_MAX_MV) return NAN;  // สายขาด หรือ ช็อต
  float r = NTC_SERIES_OHM * s.mv / (VREF_MV - s.mv);
  float invT = 1.0 / 298.15 + log(r / NTC_NOMINAL_OHM) / NTC_BETA;
  float c = 1.0 / invT - 273.15;
  if (c < -5 || c > 80) return NAN;
  return c;
}

float tdsFromMv(float mv, float tempC) {
  float t = isnan(tempC) ? 25.0 : tempC;
  float v = (mv / 1000.0) / (1.0 + 0.02 * (t - 25.0));  // ชดเชยอุณหภูมิเป็นค่าที่ 25 °C
  float tds = (133.42 * v * v * v - 255.86 * v * v + 857.39 * v) * 0.5 * tdsK;
  return tds < 0 ? 0 : tds;
}

float readTds(float tempC) {
  Sample s = readPin(PIN_TDS);
  rawTdsMv = s.mv;
  if (s.spread > NOISE_SPREAD_MV) return NAN;  // ไม่ได้ต่อโมดูล
  if (s.mv < TDS_MIN_MV) return 0;             // ต่ำกว่าที่ ESP32 อ่านได้ (<60 ppm) หรือไม่ได้จุ่มน้ำ
  return tdsFromMv(s.mv, tempC);
}

// pH จากแรงดันโมดูล + ชดเชยอุณหภูมิ (ความชันแปรผันตามอุณหภูมิสัมบูรณ์ ตามสมการเนิร์นสต์)
float phFromModuleMv(float moduleMv, float tempC) {
  float diff = phNeutralMv - phAcidMv;
  if (fabs(diff) < 50) diff = -530;                               // คาลิเบรตผิด ใช้ค่ามาตรฐาน
  float slope = (PH_BUFFER_MID - PH_BUFFER_LOW) / diff;           // pH ต่อ mV ที่อุณหภูมิตอนคาลิเบรต
  float t = isnan(tempC) ? 25.0 : tempC;
  slope *= (phCalT + 273.15) / (t + 273.15);
  return constrain(PH_BUFFER_MID + slope * (moduleMv - phNeutralMv), 0.0, 14.0);
}

float readPh(float tempC) {
  Sample s = readPin(PIN_PH);
  rawPhMv = s.mv * PH_DIVIDER;  // แรงดันจริงที่ขา Po
  if (s.spread > NOISE_SPREAD_MV) return NAN;
  if (s.mv < PH_MIN_MV || s.mv > ADC_MAX_MV) return NAN;
  return phFromModuleMv(rawPhMv, tempC);
}

Values readAll() {
  Values v;
  v.temp = readTemperature();   // อ่านอุณหภูมิก่อน เอาไปชดเชย TDS และ pH
  v.tds = readTds(v.temp);
  v.ph = readPh(v.temp);
  return v;
}

uint32_t nowEpoch() {
  return timeSet ? (uint32_t)(millis() / 1000 + epochOffset) : 0;
}

// ================= จอ LCD =================
void initLcd() {
  Wire.begin(PIN_SDA, PIN_SCL);
  // โมดูล I2C ของจอใช้ที่อยู่ 0x20-0x27 (PCF8574) หรือ 0x38-0x3F (PCF8574A) ไล่หาทุกค่า
  const uint8_t candidates[] = {0x27, 0x3F, 0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26,
                                0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E};
  uint8_t addr = 0;
  for (uint8_t a : candidates) {
    Wire.beginTransmission(a);
    if (Wire.endTransmission() == 0) { addr = a; break; }
  }
  if (addr == 0) {
    Serial.println("ไม่พบจอ LCD! ตรวจสาย SDA/SCL/VCC/GND");
    return;
  }
  Serial.printf("พบจอ LCD ที่ 0x%02X\n", addr);
  lcd = new LiquidCrystal_I2C(addr, 20, 4);
  lcd->init();
  lcd->backlight();
}

void lcdLine(int row, String s) {
  if (!lcd) return;
  while (s.length() < 20) s += ' ';
  lcd->setCursor(0, row);
  lcd->print(s.substring(0, 20));
}

String fmt(float v, int digits) { return isnan(v) ? String("-") : String(v, digits); }

void showStatus(const String &s, unsigned long ms) {
  statusLine = s;
  statusUntil = millis() + ms;
}

void drawLcd() {
  if (millis() < infoUntil) {
    lcdLine(0, "WiFi: " + String(AP_SSID));
    lcdLine(1, "Pass: " + String(AP_PASS));
    lcdLine(2, "Web : " + WiFi.softAPIP().toString());
    lcdLine(3, "Phones: " + String(WiFi.softAPgetStationNum()));
    return;
  }
  lcdLine(0, "pH  : " + fmt(live.ph, 2));
  lcdLine(1, "TDS : " + (isnan(live.tds) ? String("-") : live.tds < 1 ? String("<60 ppm") : fmt(live.tds, 0) + " ppm"));
  lcdLine(2, "Temp: " + (isnan(live.temp) ? String("-") : fmt(live.temp, 1) + " C"));
  if (millis() < statusUntil) {
    lcdLine(3, statusLine);
  } else if (WiFi.softAPgetStationNum() == 0) {
    lcdLine(3, "WiFi: " + String(AP_SSID));
  } else {
    lcdLine(3, "GREEN = measure");
  }
}

// ================= บันทึกผลตรวจในเครื่อง =================
void logSave() {
  prefs.putBytes("log", logBuf, sizeof(logBuf));
  prefs.putUChar("lhead", logHead);
  prefs.putUChar("lcount", logCount);
}

void logLoad() {
  if (prefs.getBytesLength("log") == sizeof(logBuf)) {
    prefs.getBytes("log", logBuf, sizeof(logBuf));
    logHead = prefs.getUChar("lhead", 0);
    logCount = prefs.getUChar("lcount", 0);
  }
  if (logHead >= LOG_SIZE || logCount > LOG_SIZE) { logHead = 0; logCount = 0; }
}

void logAppend(const Reading &r) {
  LogEntry &e = logBuf[logHead];
  e.seq = r.id;
  e.epoch = r.epoch;
  e.boot = bootId;
  e.ms = r.ms;
  e.ph100 = isnan(r.v.ph) ? INT16_MIN : (int16_t)lroundf(r.v.ph * 100);
  e.tds = isnan(r.v.tds) ? -1 : (int16_t)lroundf(fminf(r.v.tds, 30000));
  e.temp10 = isnan(r.v.temp) ? INT16_MIN : (int16_t)lroundf(r.v.temp * 10);
  e.flags = r.stable ? 1 : 0;
  e.wait = r.wait;
  logHead = (logHead + 1) % LOG_SIZE;
  if (logCount < LOG_SIZE) logCount++;
  logSave();
}

// ================= วัดแบบรอค่านิ่ง =================
float getK(const Values &v, int k) { return k == 0 ? v.ph : (k == 1 ? v.tds : v.temp); }
void setK(Values &v, int k, float x) { if (k == 0) v.ph = x; else if (k == 1) v.tds = x; else v.temp = x; }

// เฉลี่ยค่าใน window และตัดสินว่านิ่งหรือยัง
// เซนเซอร์ที่อ่านได้ไม่ถึงครึ่ง = ไม่ได้ต่อ ไม่ต้องรอ
bool windowStable(Values &avg) {
  bool ok = true;
  for (int k = 0; k < 3; k++) {
    float mn = 1e9, mx = -1e9, sum = 0;
    int n = 0;
    for (int i = 0; i < winCount; i++) {
      float x = getK(win[i], k);
      if (isnan(x)) continue;
      mn = fminf(mn, x);
      mx = fmaxf(mx, x);
      sum += x;
      n++;
    }
    float m = (n * 2 >= winCount) ? sum / n : NAN;
    setK(avg, k, m);
    if (isnan(m)) continue;
    float tol = (k == 0) ? STABLE_PH : (k == 1) ? fmaxf(STABLE_TDS * m, 3.0) : STABLE_TEMP;
    if (n < winCount || mx - mn > tol) ok = false;
  }
  return ok;
}

void startMeasure() {
  measuring = true;
  measureStart = millis();
  nextSampleAt = 0;
  winCount = 0;
  winPos = 0;
  Serial.println("เริ่มวัด: รอค่านิ่ง...");
}

void finishMeasure(const Values &v, bool stable, unsigned long elapsed) {
  measuring = false;
  uint32_t seq = prefs.getULong("seq", 0) + 1;
  prefs.putULong("seq", seq);
  last.id = seq;
  last.v = v;
  last.epoch = nowEpoch();
  last.ms = millis();
  last.stable = stable;
  last.wait = (uint8_t)min(elapsed / 1000, 255UL);
  logAppend(last);

  Serial.printf("ผลตรวจ #%u: pH=%s TDS=%s T=%s  %s (%u วินาที)\n", last.id,
                fmt(v.ph, 2).c_str(), fmt(v.tds, 0).c_str(), fmt(v.temp, 1).c_str(),
                stable ? "ค่านิ่ง" : "ค่ายังไม่นิ่ง", last.wait);
  if (isnan(v.ph) && isnan(v.tds) && isnan(v.temp)) {
    showStatus("No sensor reading!", 5000);
  } else if (!stable) {
    showStatus("#" + String(last.id) + " saved (unstable)", 5000);
  } else if (WiFi.softAPgetStationNum() == 0) {
    showStatus("#" + String(last.id) + " saved (no phone)", 5000);
  } else {
    showStatus("#" + String(last.id) + " sent to web", 5000);
  }
  drawLcd();
}

void stepMeasure() {
  if (millis() < nextSampleAt) return;
  nextSampleAt = millis() + SAMPLE_MS;

  Values v = readAll();
  live = v;
  win[winPos] = v;
  winPos = (winPos + 1) % STABLE_WINDOW;
  if (winCount < STABLE_WINDOW) winCount++;

  unsigned long elapsed = millis() - measureStart;
  showStatus("Wait stable... " + String(elapsed / 1000) + "s", 2000);
  drawLcd();
  if (winCount < STABLE_WINDOW) return;

  Values avg;
  bool stable = windowStable(avg);
  if (stable || elapsed >= MEASURE_MAX_MS) finishMeasure(avg, stable, elapsed);
}

// ================= ปุ่ม =================
void handleButton() {
  static bool lastState = HIGH;
  static unsigned long pressedAt = 0, lastChange = 0;
  static bool longDone = false;
  bool state = digitalRead(PIN_BUTTON);
  if (state != lastState && millis() - lastChange > 40) {
    lastChange = millis();
    lastState = state;
    if (state == LOW) {          // กดลง
      pressedAt = millis();
      longDone = false;
    } else if (!longDone && !measuring) {   // ปล่อย (กดสั้น) -> เริ่มวัด
      measureRequested = true;
    }
  }
  // กดค้าง 2 วินาที -> แสดงข้อมูล WiFi 6 วินาที
  if (state == LOW && !longDone && millis() - pressedAt > 2000) {
    longDone = true;
    infoUntil = millis() + 6000;
    drawLcd();
  }
}

// ================= Web API =================
String jnum(float v, int digits) { return isnan(v) ? String("null") : String(v, digits); }

String valuesJson(const Values &v) {
  return "{\"ph\":" + jnum(v.ph, 2) + ",\"tds\":" + jnum(v.tds, 0) +
         ",\"temp\":" + jnum(v.temp, 1) + "}";
}

String readingJson(uint32_t id, uint32_t epoch, const Values &v, bool stable, uint8_t wait) {
  return "{\"id\":" + String(id) + ",\"epoch\":" + String(epoch) + ",\"values\":" + valuesJson(v) +
         ",\"stable\":" + String(stable ? "true" : "false") + ",\"wait\":" + String(wait) + "}";
}

void sendJson(const String &j) {
  server.sendHeader("Cache-Control", "no-store");
  server.send(200, "application/json", j);
}

void handleState() {
  String j = "{\"boot\":" + String(bootId);
  j += ",\"live\":" + valuesJson(live);
  j += ",\"raw\":{\"ph_mv\":" + String(rawPhMv, 0) + ",\"tds_mv\":" + String(rawTdsMv, 0) +
       ",\"temp_mv\":" + String(rawTempMv, 0) + "}";
  j += ",\"busy\":" + String(measuring || measureRequested ? "true" : "false");
  j += ",\"busy_s\":" + String(measuring ? (millis() - measureStart) / 1000 : 0);
  j += ",\"busy_max\":" + String(MEASURE_MAX_MS / 1000);
  j += ",\"last\":" + (last.id > 0 ? readingJson(last.id, last.epoch, last.v, last.stable, last.wait) : String("null"));
  j += ",\"log_count\":" + String(logCount);
  j += ",\"time_set\":" + String(timeSet ? "true" : "false") + "}";
  sendJson(j);
}

void handleLog() {
  String j = "{\"items\":[";
  for (int i = 0; i < logCount; i++) {
    const LogEntry &e = logBuf[(logHead - logCount + i + LOG_SIZE) % LOG_SIZE];
    Values v;
    v.ph = e.ph100 == INT16_MIN ? NAN : e.ph100 / 100.0;
    v.tds = e.tds < 0 ? NAN : e.tds;
    v.temp = e.temp10 == INT16_MIN ? NAN : e.temp10 / 10.0;
    if (i) j += ",";
    j += readingJson(e.seq, e.epoch, v, e.flags & 1, e.wait);
  }
  j += "]}";
  sendJson(j);
}

void handleLogClear() {
  logHead = 0;
  logCount = 0;
  logSave();
  sendJson("{\"ok\":true}");
}

void handleSetTime() {
  if (server.hasArg("epoch")) {
    uint32_t e = strtoul(server.arg("epoch").c_str(), nullptr, 10);
    if (e > 1600000000UL) {
      epochOffset = (int64_t)e - (int64_t)(millis() / 1000);
      timeSet = true;
      // เติมเวลาจริงให้ผลตรวจที่วัดไปก่อนมือถือเชื่อมต่อ (ในรอบเปิดเครื่องนี้)
      bool changed = false;
      for (int i = 0; i < LOG_SIZE; i++) {
        if (logBuf[i].boot == bootId && logBuf[i].epoch == 0 && logBuf[i].seq > 0) {
          logBuf[i].epoch = (uint32_t)(logBuf[i].ms / 1000 + epochOffset);
          changed = true;
        }
      }
      if (last.id > 0 && last.epoch == 0) last.epoch = (uint32_t)(last.ms / 1000 + epochOffset);
      if (changed) logSave();
    }
  }
  sendJson("{\"ok\":true}");
}

void handleMeasure() {
  if (!measuring) measureRequested = true;
  sendJson("{\"ok\":true,\"busy\":true}");
}

void handleRoot() {
  server.send_P(200, "text/html; charset=utf-8", INDEX_HTML);
}

// ================= คาลิเบรตผ่าน Serial Monitor =================
void printHelp() {
  Serial.println(F("คำสั่ง: cal7 (จุ่มน้ำยา pH 6.86) | cal4 (จุ่มน้ำยา pH 4.00) | tds <ppmจริง> | show | reset"));
}

void handleSerial() {
  if (!Serial.available()) return;
  String cmd = Serial.readStringUntil('\n');
  cmd.trim();
  if (cmd == "cal7" || cmd == "cal4") {
    Sample s = readPin(PIN_PH);
    float mv = s.mv * PH_DIVIDER;
    if (s.mv < PH_MIN_MV) { Serial.println("อ่านค่า pH ไม่ได้ ตรวจสายก่อน"); return; }
    float t = readTemperature();
    phCalT = isnan(t) ? 25.0 : t;            // จำอุณหภูมิน้ำยาไว้ชดเชยภายหลัง
    prefs.putFloat("phT", phCalT);
    if (cmd == "cal7") { phNeutralMv = mv; prefs.putFloat("ph7", mv); }
    else               { phAcidMv = mv;    prefs.putFloat("ph4", mv); }
    Serial.printf("บันทึก %s = %.0f mV (น้ำยา %.1f °C)\n", cmd.c_str(), mv, phCalT);
  } else if (cmd.startsWith("tds ")) {
    float real = cmd.substring(4).toFloat();
    Sample s = readPin(PIN_TDS);
    float oldK = tdsK;
    tdsK = 1.0;
    float measured = s.mv < TDS_MIN_MV ? 0 : tdsFromMv(s.mv, readTemperature());
    if (real > 0 && measured > 1) {
      tdsK = real / measured;
      prefs.putFloat("tdsK", tdsK);
      Serial.printf("บันทึก TDS K = %.3f\n", tdsK);
    } else {
      tdsK = oldK;
      Serial.println("ค่าไม่ถูกต้อง (จุ่มหัววัดในน้ำยามาตรฐานก่อน)");
    }
  } else if (cmd == "reset") {
    prefs.remove("ph7");
    prefs.remove("ph4");
    prefs.remove("phT");
    prefs.remove("tdsK");
    phNeutralMv = 2500; phAcidMv = 3030; phCalT = 25.0; tdsK = 1.0;
    Serial.println("ล้างค่าคาลิเบรตแล้ว (ผลตรวจที่บันทึกไว้ยังอยู่)");
  } else if (cmd == "show") {
    Serial.printf("pH 6.86=%.0fmV pH 4.00=%.0fmV (น้ำยา %.1f °C)  tdsK=%.3f  ผลตรวจในเครื่อง=%u\n",
                  phNeutralMv, phAcidMv, phCalT, tdsK, logCount);
  } else {
    printHelp();
  }
}

// ================= setup / loop =================
void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.println("\n=== เครื่องวัดคุณภาพน้ำ ===");

  analogReadResolution(12);  // ค่าเริ่มต้นของ ESP32 อ่านได้ 0-3.1V อยู่แล้ว
  pinMode(PIN_BUTTON, INPUT_PULLUP);

  prefs.begin("wqm", false);
  phNeutralMv = prefs.getFloat("ph7", phNeutralMv);
  phAcidMv    = prefs.getFloat("ph4", phAcidMv);
  phCalT      = prefs.getFloat("phT", phCalT);
  tdsK        = prefs.getFloat("tdsK", tdsK);
  logLoad();

  initLcd();
  lcdLine(0, "Smart Water System");
  lcdLine(1, "Starting WiFi...");

  WiFi.mode(WIFI_AP);
  WiFi.softAP(AP_SSID, AP_PASS);
  bootId = esp_random() % 1000000;

  server.enableCORS(true);  // ให้หน้าเว็บที่เปิดจาก VS Code เรียก API ได้
  server.on("/", HTTP_GET, handleRoot);
  server.on("/api/state", HTTP_GET, handleState);
  server.on("/api/log", HTTP_GET, handleLog);
  server.on("/api/log/clear", HTTP_POST, handleLogClear);
  server.on("/api/time", HTTP_POST, handleSetTime);
  server.on("/api/measure", HTTP_POST, handleMeasure);
  server.begin();

  Serial.printf("WiFi: %s  รหัส: %s  เปิดเว็บ: http://%s\n",
                AP_SSID, AP_PASS, WiFi.softAPIP().toString().c_str());
  printHelp();
}

void loop() {
  static unsigned long lastRead = 0;

  handleButton();
  server.handleClient();
  handleSerial();

  if (measureRequested && !measuring) {
    measureRequested = false;
    startMeasure();
  }
  if (measuring) {          // ระหว่างวัด อ่านค่าทุก 0.5 วินาที (เว็บยังตอบได้ตลอด)
    stepMeasure();
    return;
  }

  if (millis() - lastRead >= 1000) {
    lastRead = millis();
    live = readAll();
    drawLcd();
    Serial.printf("pH=%s TDS=%s T=%s | mV pH=%.0f TDS=%.0f T=%.0f\n",
                  fmt(live.ph, 2).c_str(), fmt(live.tds, 0).c_str(), fmt(live.temp, 1).c_str(),
                  rawPhMv, rawTdsMv, rawTempMv);
  }
}
