/*
 * เครื่องวัดคุณภาพน้ำ (Water Quality Meter) — ESP32
 *
 * การทำงาน
 *   1) เซนเซอร์ pH, TDS, อุณหภูมิ วัดค่าตลอดเวลา แสดงบนจอ LCD 20x4
 *      เซนเซอร์ตัวไหนไม่ได้ต่อ/ไม่ได้จุ่มน้ำ จะแสดงเป็น "-"
 *   2) กดปุ่มสีเขียว -> ESP32 วัดค่าละเอียด (เฉลี่ยหลายรอบ) แล้วส่งผลไปที่หน้าเว็บ
 *   3) หน้าเว็บ (เปิดจากมือถือ) วิเคราะห์น้ำ ประเมินแหล่งที่มา แสดงวันเวลาและสถานที่
 *
 * แก้ขาและค่าต่างๆ ได้ในแท็บ config.h
 * หน้าเว็บอยู่ในไฟล์ web_page.h (สร้างจากโฟลเดอร์ web/ ด้วย tools/build_web.py)
 * ไลบรารีที่ต้องติดตั้ง: "LiquidCrystal I2C" (by Frank de Brabander)
 * ดูวิธีต่อสายใน README.md และ docs/wiring.svg
 */

#include <Wire.h>
#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>
#include <LiquidCrystal_I2C.h>
#include <math.h>
#include "web_page.h"  // หน้าเว็บ (สร้างจากโฟลเดอร์ web/ — ห้ามแก้ตรงๆ)

#include "config.h"   // ขา ค่าวงจร และ WiFi อยู่ในแท็บ config.h

// ================= ตัวแปรระบบ =================
LiquidCrystal_I2C *lcd = nullptr;
WebServer server(80);
Preferences prefs;

float phNeutralMv = 2500.0;  // แรงดันโมดูล pH (ก่อนแบ่ง) ในน้ำยา pH 7.00
float phAcidMv    = 3030.0;  // แรงดันโมดูล pH (ก่อนแบ่ง) ในน้ำยา pH 4.00
float tdsK        = 1.0;

struct Values {
  float ph = NAN, tds = NAN, temp = NAN;
};
struct Sample {
  float mv;      // ค่ากลาง
  float spread;  // ความแกว่ง (max-min ของช่วงกลาง)
};

Values live;                  // ค่าสด อัปเดตทุก 1 วินาที
float rawTdsMv = 0, rawPhMv = 0, rawTempMv = 0;

// ผลที่ส่งจากการกดปุ่ม
struct Reading {
  uint32_t id = 0;
  Values v;
  uint32_t epoch = 0;  // เวลาจริง (ถ้ามือถือส่งเวลามาให้แล้ว) ไม่งั้น 0
};
Reading last;
uint32_t bootId = 0;
int64_t epochOffset = 0;      // epoch = millis()/1000 + epochOffset
bool timeSet = false;

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
  float v = (mv / 1000.0) / (1.0 + 0.02 * (t - 25.0));  // ชดเชยอุณหภูมิ
  float tds = (133.42 * v * v * v - 255.86 * v * v + 857.39 * v) * 0.5 * tdsK;
  return tds < 0 ? 0 : tds;
}

float readTds(float tempC) {
  Sample s = readPin(PIN_TDS);
  rawTdsMv = s.mv;
  if (s.spread > NOISE_SPREAD_MV) return NAN;  // ไม่ได้ต่อโมดูล
  if (s.mv < TDS_MIN_MV) return NAN;           // หัววัดไม่ได้อยู่ในน้ำ
  return tdsFromMv(s.mv, tempC);
}

float phFromModuleMv(float moduleMv) {
  float slope = (7.0 - 4.0) / (phNeutralMv - phAcidMv);  // pH ต่อ mV
  return constrain(7.0 + slope * (moduleMv - phNeutralMv), 0.0, 14.0);
}

float readPh() {
  Sample s = readPin(PIN_PH);
  rawPhMv = s.mv * PH_DIVIDER;  // แรงดันจริงที่ขา Po
  if (s.spread > NOISE_SPREAD_MV) return NAN;
  if (s.mv < PH_MIN_MV || s.mv > ADC_MAX_MV) return NAN;
  return phFromModuleMv(rawPhMv);
}

Values readAll() {
  Values v;
  v.temp = readTemperature();
  v.tds = readTds(v.temp);
  v.ph = readPh();
  return v;
}

// วัดละเอียด: 8 รอบ เอาค่าเฉลี่ยเฉพาะรอบที่อ่านได้
Values measureCareful() {
  float sum[3] = {0, 0, 0};
  int n[3] = {0, 0, 0};
  for (int i = 0; i < 8; i++) {
    Values v = readAll();
    if (!isnan(v.ph))   { sum[0] += v.ph;   n[0]++; }
    if (!isnan(v.tds))  { sum[1] += v.tds;  n[1]++; }
    if (!isnan(v.temp)) { sum[2] += v.temp; n[2]++; }
  }
  Values r;
  // ต้องอ่านได้อย่างน้อยครึ่งหนึ่ง ถึงจะนับว่าวัดได้
  r.ph   = n[0] >= 4 ? sum[0] / n[0] : NAN;
  r.tds  = n[1] >= 4 ? sum[1] / n[1] : NAN;
  r.temp = n[2] >= 4 ? sum[2] / n[2] : NAN;
  return r;
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
  lcdLine(1, "TDS : " + (isnan(live.tds) ? String("-") : fmt(live.tds, 0) + " ppm"));
  lcdLine(2, "Temp: " + (isnan(live.temp) ? String("-") : fmt(live.temp, 1) + " C"));
  if (millis() < statusUntil) {
    lcdLine(3, statusLine);
  } else if (WiFi.softAPgetStationNum() == 0) {
    lcdLine(3, "WiFi: " + String(AP_SSID));
  } else {
    lcdLine(3, "Press GREEN to send");
  }
}

// ================= ปุ่ม =================
void sendMeasurement() {
  lcdLine(3, "Measuring...");
  Values v = measureCareful();
  last.id++;
  last.v = v;
  last.epoch = nowEpoch();
  Serial.printf("ส่งผล #%u: pH=%s TDS=%s T=%s\n", last.id,
                fmt(v.ph, 2).c_str(), fmt(v.tds, 0).c_str(), fmt(v.temp, 1).c_str());
  if (isnan(v.ph) && isnan(v.tds) && isnan(v.temp)) {
    showStatus("No sensor reading!", 4000);
  } else if (WiFi.softAPgetStationNum() == 0) {
    showStatus("Saved #" + String(last.id) + " (no phone)", 4000);
  } else {
    showStatus("Sent #" + String(last.id) + " to web", 4000);
  }
  drawLcd();
}

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
    } else if (!longDone) {      // ปล่อย (กดสั้น) -> วัดและส่ง
      sendMeasurement();
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

void handleState() {
  String j = "{\"boot\":" + String(bootId);
  j += ",\"live\":" + valuesJson(live);
  j += ",\"raw\":{\"ph_mv\":" + String(rawPhMv, 0) + ",\"tds_mv\":" + String(rawTdsMv, 0) +
       ",\"temp_mv\":" + String(rawTempMv, 0) + "}";
  if (last.id > 0) {
    j += ",\"last\":{\"id\":" + String(last.id) + ",\"epoch\":" + String(last.epoch) +
         ",\"values\":" + valuesJson(last.v) + "}";
  } else {
    j += ",\"last\":null";
  }
  j += ",\"time_set\":" + String(timeSet ? "true" : "false") + "}";
  server.sendHeader("Cache-Control", "no-store");
  server.send(200, "application/json", j);
}

void handleSetTime() {
  if (server.hasArg("epoch")) {
    uint32_t e = strtoul(server.arg("epoch").c_str(), nullptr, 10);
    if (e > 1600000000UL) {
      epochOffset = (int64_t)e - (int64_t)(millis() / 1000);
      timeSet = true;
    }
  }
  server.send(200, "application/json", "{\"ok\":true}");
}

void handleMeasure() {
  sendMeasurement();
  server.send(200, "application/json", "{\"ok\":true,\"id\":" + String(last.id) + "}");
}

void handleRoot() {
  server.send_P(200, "text/html; charset=utf-8", INDEX_HTML);
}

// ================= คาลิเบรตผ่าน Serial Monitor =================
void printHelp() {
  Serial.println(F("คำสั่ง: cal7 | cal4 | tds <ppmจริง> | show | reset"));
}

void handleSerial() {
  if (!Serial.available()) return;
  String cmd = Serial.readStringUntil('\n');
  cmd.trim();
  if (cmd == "cal7" || cmd == "cal4") {
    Sample s = readPin(PIN_PH);
    float mv = s.mv * PH_DIVIDER;
    if (s.mv < PH_MIN_MV) { Serial.println("อ่านค่า pH ไม่ได้ ตรวจสายก่อน"); return; }
    if (cmd == "cal7") { phNeutralMv = mv; prefs.putFloat("ph7", mv); }
    else               { phAcidMv = mv;    prefs.putFloat("ph4", mv); }
    Serial.printf("บันทึก %s = %.0f mV\n", cmd.c_str(), mv);
  } else if (cmd.startsWith("tds ")) {
    float real = cmd.substring(4).toFloat();
    Sample s = readPin(PIN_TDS);
    float oldK = tdsK;
    tdsK = 1.0;
    float measured = tdsFromMv(s.mv, readTemperature());
    if (real > 0 && measured > 1) {
      tdsK = real / measured;
      prefs.putFloat("tdsK", tdsK);
      Serial.printf("บันทึก TDS K = %.3f\n", tdsK);
    } else {
      tdsK = oldK;
      Serial.println("ค่าไม่ถูกต้อง (จุ่มหัววัดในน้ำยามาตรฐานก่อน)");
    }
  } else if (cmd == "reset") {
    prefs.clear();
    phNeutralMv = 2500; phAcidMv = 3030; tdsK = 1.0;
    Serial.println("ล้างค่าคาลิเบรตแล้ว");
  } else if (cmd == "show") {
    Serial.printf("pH7=%.0fmV pH4=%.0fmV tdsK=%.3f\n", phNeutralMv, phAcidMv, tdsK);
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
  tdsK        = prefs.getFloat("tdsK", tdsK);

  initLcd();
  lcdLine(0, "Water Quality Meter");
  lcdLine(1, "Starting WiFi...");

  WiFi.mode(WIFI_AP);
  WiFi.softAP(AP_SSID, AP_PASS);
  bootId = esp_random() % 1000000;

  server.enableCORS(true);  // ให้หน้าเว็บที่เปิดจาก VS Code เรียก API ได้
  server.on("/", HTTP_GET, handleRoot);
  server.on("/api/state", HTTP_GET, handleState);
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

  if (millis() - lastRead >= 1000) {
    lastRead = millis();
    live = readAll();
    drawLcd();
    Serial.printf("pH=%s TDS=%s T=%s | mV pH=%.0f TDS=%.0f T=%.0f\n",
                  fmt(live.ph, 2).c_str(), fmt(live.tds, 0).c_str(), fmt(live.temp, 1).c_str(),
                  rawPhMv, rawTdsMv, rawTempMv);
  }
}
