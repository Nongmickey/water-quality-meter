/*
 * เครื่องวัดคุณภาพน้ำ (Water Quality Meter) — ESP32
 *
 * วัด: pH, TDS (ppm), อุณหภูมิน้ำ (NTC 10k แบบ 2 สาย)
 * แสดงผล: จอ LCD 20x4 (I2C) + หน้าเว็บผ่าน WiFi ของเครื่องเอง
 * ปุ่ม: กดสั้น = เปลี่ยนหน้าจอ
 *
 * ไลบรารีที่ต้องติดตั้ง: "LiquidCrystal I2C" (by Frank de Brabander)
 * ดูวิธีต่อสายและวิธีคาลิเบรตใน README.md
 */

#include <Wire.h>
#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>
#include <LiquidCrystal_I2C.h>
#include <math.h>

// ---------------- ขาที่ใช้ ----------------
// ขาอนาล็อกต้องเป็นกลุ่ม ADC1 (32-39) เท่านั้น เพราะ ADC2 ใช้ไม่ได้ตอนเปิด WiFi
const int PIN_TDS    = 34;  // TDS: ขา A
const int PIN_PH     = 35;  // pH: ขา Po (ผ่านตัวแบ่งแรงดัน 10k/20k)
const int PIN_TEMP   = 32;  // NTC 10k (ต่อกับ 10k ไป 3V3)
const int PIN_BUTTON = 15;  // ปุ่มกด (อีกขาต่อ GND)
const int PIN_SDA    = 21;
const int PIN_SCL    = 22;

// ---------------- ค่าวงจร ----------------
const float VREF_MV         = 3300.0;  // แรงดันเลี้ยงตัวแบ่งแรงดันของ NTC
const float NTC_SERIES_OHM  = 10000.0; // ตัวต้านทานที่ต่อกับ NTC
const float NTC_NOMINAL_OHM = 10000.0; // NTC 10k ที่ 25°C
const float NTC_BETA        = 3950.0;  // ค่า B ของ NTC (ส่วนใหญ่ 3950)
const float PH_DIVIDER      = 1.5;     // (10k + 20k) / 20k  → แรงดันจริง = ที่อ่านได้ x 1.5

// ---------------- WiFi (ESP32 ปล่อย WiFi เอง) ----------------
const char *AP_SSID = "WaterMeter";
const char *AP_PASS = "12345678";      // อย่างน้อย 8 ตัวอักษร

// ---------------- เกณฑ์คุณภาพน้ำ ----------------
const float PH_MIN_OK  = 6.5;
const float PH_MAX_OK  = 8.5;
const float TDS_MAX_OK = 500.0;        // ppm (เกณฑ์น้ำดื่มทั่วไป)

// ---------------- ตัวแปรระบบ ----------------
LiquidCrystal_I2C *lcd = nullptr;
WebServer server(80);
Preferences prefs;

// ค่าคาลิเบรต (เก็บถาวรในหน่วยความจำ)
float phNeutralMv = 2500.0;  // แรงดันโมดูล pH ในน้ำยา pH 7.00
float phAcidMv    = 3030.0;  // แรงดันโมดูล pH ในน้ำยา pH 4.00
float tdsK        = 1.0;     // ตัวคูณแก้ค่า TDS

// ค่าล่าสุด
float temperatureC = NAN;
float tdsPpm  = NAN;
float phValue = NAN;
float tdsMv = 0, phMv = 0, tempMv = 0;

int page = 0;
const int PAGE_COUNT = 3;

// ---------------- อ่าน ADC แบบกรองสัญญาณรบกวน ----------------
float readMilliVoltsMedian(int pin) {
  const int N = 21;
  int buf[N];
  for (int i = 0; i < N; i++) {
    buf[i] = analogReadMilliVolts(pin);
    delay(2);
  }
  // insertion sort แล้วเอาค่ากลาง
  for (int i = 1; i < N; i++) {
    int v = buf[i], j = i - 1;
    while (j >= 0 && buf[j] > v) { buf[j + 1] = buf[j]; j--; }
    buf[j + 1] = v;
  }
  return buf[N / 2];
}

// ---------------- อุณหภูมิ (NTC) ----------------
float readTemperature() {
  tempMv = readMilliVoltsMedian(PIN_TEMP);
  if (tempMv < 50 || tempMv > VREF_MV - 50) return NAN;  // สายหลุด/ช็อต
  float r = NTC_SERIES_OHM * tempMv / (VREF_MV - tempMv);
  float invT = 1.0 / 298.15 + log(r / NTC_NOMINAL_OHM) / NTC_BETA;
  return 1.0 / invT - 273.15;
}

// ---------------- TDS ----------------
float readTds(float tempC) {
  tdsMv = readMilliVoltsMedian(PIN_TDS);
  float t = isnan(tempC) ? 25.0 : tempC;
  float v = (tdsMv / 1000.0) / (1.0 + 0.02 * (t - 25.0));  // ชดเชยอุณหภูมิ
  float tds = (133.42 * v * v * v - 255.86 * v * v + 857.39 * v) * 0.5 * tdsK;
  return tds < 0 ? 0 : tds;
}

// ---------------- pH ----------------
float readPh() {
  phMv = readMilliVoltsMedian(PIN_PH) * PH_DIVIDER;
  float slope = (7.0 - 4.0) / (phNeutralMv - phAcidMv);  // pH ต่อ mV
  float ph = 7.0 + slope * (phMv - phNeutralMv);
  return constrain(ph, 0.0, 14.0);
}

bool phOk()  { return !isnan(phValue) && phValue >= PH_MIN_OK && phValue <= PH_MAX_OK; }
bool tdsOk() { return !isnan(tdsPpm) && tdsPpm <= TDS_MAX_OK; }

const char *tdsLevel(float tds) {
  if (isnan(tds))   return "-";
  if (tds < 300)    return "Excellent";
  if (tds < 600)    return "Good";
  if (tds < 900)    return "Fair";
  if (tds < 1200)   return "Poor";
  return "Bad";
}

// ---------------- จอ LCD ----------------
void initLcd() {
  Wire.begin(PIN_SDA, PIN_SCL);
  uint8_t addr = 0;
  const uint8_t candidates[] = {0x27, 0x3F};  // ที่อยู่ I2C ที่พบบ่อย
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

void lcdLine(int row, const String &text) {
  if (!lcd) return;
  String s = text;
  while (s.length() < 20) s += ' ';
  lcd->setCursor(0, row);
  lcd->print(s.substring(0, 20));
}

String fmt(float v, int digits) { return isnan(v) ? String("--") : String(v, digits); }

void drawPage() {
  switch (page) {
    case 0:  // หน้าหลัก
      lcdLine(0, "  WATER QUALITY");
      lcdLine(1, "pH  : " + fmt(phValue, 2) + (phOk() ? "  OK" : "  !!"));
      lcdLine(2, "TDS : " + fmt(tdsPpm, 0) + " ppm" + (tdsOk() ? " OK" : " !!"));
      lcdLine(3, "Temp: " + fmt(temperatureC, 1) + " C");
      break;
    case 1:  // สรุปผล
      lcdLine(0, "RESULT:");
      lcdLine(1, (phOk() && tdsOk()) ? "  WATER IS GOOD" : "  CHECK WATER!");
      lcdLine(2, String("pH ") + (phOk() ? "normal" : (phValue < PH_MIN_OK ? "acidic" : "alkaline")));
      lcdLine(3, String("TDS ") + tdsLevel(tdsPpm));
      break;
    case 2:  // ข้อมูลระบบ
      lcdLine(0, "WiFi: " + String(AP_SSID));
      lcdLine(1, "Pass: " + String(AP_PASS));
      lcdLine(2, "Web : " + WiFi.softAPIP().toString());
      lcdLine(3, "mV T" + String((int)tempMv) + " D" + String((int)tdsMv) + " P" + String((int)phMv));
      break;
  }
}

// ---------------- ปุ่ม ----------------
void handleButton() {
  static bool lastState = HIGH;
  static unsigned long lastChange = 0;
  bool state = digitalRead(PIN_BUTTON);
  if (state != lastState && millis() - lastChange > 50) {
    lastChange = millis();
    lastState = state;
    if (state == LOW) {  // กดลง
      page = (page + 1) % PAGE_COUNT;
      drawPage();
    }
  }
}

// ---------------- หน้าเว็บ ----------------
void handleJson() {
  String j = "{";
  j += "\"temperature\":" + (isnan(temperatureC) ? String("null") : String(temperatureC, 1)) + ",";
  j += "\"tds\":" + (isnan(tdsPpm) ? String("null") : String(tdsPpm, 0)) + ",";
  j += "\"ph\":" + (isnan(phValue) ? String("null") : String(phValue, 2)) + ",";
  j += "\"ph_ok\":" + String(phOk() ? "true" : "false") + ",";
  j += "\"tds_ok\":" + String(tdsOk() ? "true" : "false") + ",";
  j += "\"tds_level\":\"" + String(tdsLevel(tdsPpm)) + "\"";
  j += "}";
  server.send(200, "application/json", j);
}

const char INDEX_HTML[] PROGMEM = R"HTML(<!doctype html>
<html lang="th"><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>เครื่องวัดคุณภาพน้ำ</title>
<style>
body{font-family:sans-serif;margin:0;padding:16px;background:#eef6fb;color:#123}
h1{font-size:22px;margin:0 0 12px}
.card{background:#fff;border-radius:12px;padding:16px;margin-bottom:12px;box-shadow:0 1px 4px #0002}
.v{font-size:40px;font-weight:bold}.u{font-size:16px;color:#567}
.ok{color:#1a8a3a}.bad{color:#c62828}
#sum{font-size:20px;font-weight:bold;text-align:center}
</style></head><body>
<h1>💧 เครื่องวัดคุณภาพน้ำ</h1>
<div class="card" id="sum">กำลังโหลด...</div>
<div class="card">pH<div><span class="v" id="ph">--</span></div><div id="phs"></div></div>
<div class="card">TDS<div><span class="v" id="tds">--</span> <span class="u">ppm</span></div><div id="tdss"></div></div>
<div class="card">อุณหภูมิ<div><span class="v" id="t">--</span> <span class="u">°C</span></div></div>
<script>
const lv={Excellent:"ดีมาก",Good:"ดี",Fair:"พอใช้",Poor:"ไม่ดี",Bad:"แย่","-":"-"};
async function load(){try{
 const d=await (await fetch('/data')).json();
 ph.textContent=d.ph??'--'; tds.textContent=d.tds??'--'; t.textContent=d.temperature??'--';
 phs.innerHTML=d.ph_ok?'<span class=ok>ปกติ (6.5–8.5)</span>':'<span class=bad>ผิดปกติ</span>';
 tdss.innerHTML='<span class='+(d.tds_ok?'ok':'bad')+'>'+lv[d.tds_level]+'</span>';
 const g=d.ph_ok&&d.tds_ok; sum.className='card '+(g?'ok':'bad');
 sum.textContent=g?'✅ น้ำมีคุณภาพดี':'⚠️ ควรตรวจสอบน้ำ';
}catch(e){sum.textContent='เชื่อมต่อไม่ได้';}}
load(); setInterval(load,2000);
</script></body></html>)HTML";

void handleRoot() { server.send_P(200, "text/html; charset=utf-8", INDEX_HTML); }

// ---------------- คาลิเบรตผ่าน Serial Monitor ----------------
void printHelp() {
  Serial.println(F("คำสั่ง: cal7 | cal4 | tds <ค่าppmจริง> | show | reset"));
}

void handleSerial() {
  if (!Serial.available()) return;
  String cmd = Serial.readStringUntil('\n');
  cmd.trim();
  if (cmd == "cal7") {
    phNeutralMv = readMilliVoltsMedian(PIN_PH) * PH_DIVIDER;
    prefs.putFloat("ph7", phNeutralMv);
    Serial.printf("บันทึก pH7 = %.0f mV\n", phNeutralMv);
  } else if (cmd == "cal4") {
    phAcidMv = readMilliVoltsMedian(PIN_PH) * PH_DIVIDER;
    prefs.putFloat("ph4", phAcidMv);
    Serial.printf("บันทึก pH4 = %.0f mV\n", phAcidMv);
  } else if (cmd.startsWith("tds ")) {
    float real = cmd.substring(4).toFloat();
    float measured = readTds(temperatureC) / tdsK;  // ค่าก่อนคูณ K
    if (real > 0 && measured > 0) {
      tdsK = real / measured;
      prefs.putFloat("tdsK", tdsK);
      Serial.printf("บันทึก TDS K = %.3f\n", tdsK);
    } else {
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

// ---------------- setup / loop ----------------
void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.println("\n=== เครื่องวัดคุณภาพน้ำ ===");

  analogReadResolution(12);
  analogSetPinAttenuation(PIN_TDS, ADC_11db);
  analogSetPinAttenuation(PIN_PH, ADC_11db);
  analogSetPinAttenuation(PIN_TEMP, ADC_11db);
  pinMode(PIN_BUTTON, INPUT_PULLUP);

  prefs.begin("wqm", false);
  phNeutralMv = prefs.getFloat("ph7", phNeutralMv);
  phAcidMv    = prefs.getFloat("ph4", phAcidMv);
  tdsK        = prefs.getFloat("tdsK", tdsK);

  initLcd();
  lcdLine(0, "Water Quality Meter");
  lcdLine(1, "Starting...");

  WiFi.mode(WIFI_AP);
  WiFi.softAP(AP_SSID, AP_PASS);
  server.on("/", handleRoot);
  server.on("/data", handleJson);
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
    temperatureC = readTemperature();
    tdsPpm  = readTds(temperatureC);
    phValue = readPh();
    drawPage();
    Serial.printf("T=%s C  TDS=%s ppm  pH=%s  | mV T=%.0f D=%.0f P=%.0f\n",
                  fmt(temperatureC, 1).c_str(), fmt(tdsPpm, 0).c_str(),
                  fmt(phValue, 2).c_str(), tempMv, tdsMv, phMv);
  }
}
