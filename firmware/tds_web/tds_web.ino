/*
 * เครื่องวัด TDS ที่คำนวณค่าผ่านหน้าเว็บบนมือถือ
 *
 * ขั้นตอนการทำงาน
 *   1) ยังไม่มีมือถือต่อ WiFi      -> จอขึ้น NOT CONNECTED พร้อมชื่อ WiFi และรหัส
 *   2) มือถือต่อ WiFi แล้ว           -> จอบอกให้เปิดเว็บ 192.168.4.1
 *   3) เปิดเว็บแล้ว                  -> จอขึ้น "Waiting for water"
 *   4) จุ่มหัววัดลงน้ำ                -> ESP32 ส่งแรงดันดิบ (mV) ไปที่เว็บ
 *   5) เว็บคำนวณ TDS ด้วยสูตรอ้างอิงของ DFRobot (ผู้ผลิตเซนเซอร์)
 *      แล้วส่งค่ากลับมาแสดงบนจอ LCD
 *   ถ้าปิดเว็บหรือมือถือหลุด จอจะกลับไปขั้นที่ 1 หรือ 2 และไม่แสดงค่า
 *
 * ต่อสาย: TDS A -> 4.7k -> D34 (+10µF, 104 ลง GND) · LCD SDA D21, SCL D22
 * ไลบรารี: "LiquidCrystal I2C" (by Frank de Brabander)
 */

#include <WiFi.h>
#include <WebServer.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <math.h>

// ---------- ตั้งค่า ----------
const char *AP_SSID = "WaterMeter";
const char *AP_PASS = "12345678";      // อย่างน้อย 8 ตัวอักษร
const int   PIN_TDS = 34;
const int   PIN_SDA = 21;
const int   PIN_SCL = 22;
const float IN_WATER_MV = 8.0;         // แรงดันตั้งแต่ค่านี้ขึ้นไป = จุ่มหัววัดอยู่ในน้ำ
const float FINE_MAX_MV = 800.0;       // ต่ำกว่านี้ใช้ช่วงวัดละเอียดของ ESP32
const unsigned long WEB_TIMEOUT_MS = 3000;  // เว็บเงียบเกินนี้ = ถือว่าปิดเว็บแล้ว

WebServer server(80);
LiquidCrystal_I2C *lcd = nullptr;

float lastMv = 0;               // แรงดันล่าสุดจากเซนเซอร์
bool inWater = false;
uint32_t seq = 0;               // เลขลำดับการอ่านค่า
unsigned long lastPollMs = 0;   // เวลาที่เว็บมาขอข้อมูลครั้งล่าสุด
float webTds = NAN, webEc = NAN;
unsigned long webResultMs = 0;  // เวลาที่ได้ผลคำนวณจากเว็บครั้งล่าสุด

// ================= อ่านแรงดันจากเซนเซอร์ =================
// อ่าน 31 ครั้ง ตัดค่าที่กระโดดหัวท้ายทิ้ง แล้วเฉลี่ยตรงกลาง 15 ค่า
float readSmoothMv() {
  const int N = 31;
  int buf[N];
  for (int i = 0; i < N; i++) {
    buf[i] = analogReadMilliVolts(PIN_TDS);
    delayMicroseconds(500);
  }
  for (int i = 1; i < N; i++) {
    int v = buf[i], j = i - 1;
    while (j >= 0 && buf[j] > v) { buf[j + 1] = buf[j]; j--; }
    buf[j + 1] = v;
  }
  float sum = 0;
  for (int i = 8; i < 23; i++) sum += buf[i];
  return sum / 15.0;
}

// เลือกช่วงวัดอัตโนมัติ: แรงดันต่ำใช้ช่วงละเอียด แรงดันสูงใช้ช่วงกว้าง
float readMilliVolts() {
  analogSetPinAttenuation(PIN_TDS, ADC_11db);
  float mv = readSmoothMv();
  if (mv < FINE_MAX_MV) {
    analogSetPinAttenuation(PIN_TDS, ADC_0db);
    mv = readSmoothMv();
  }
  return mv;
}

// ================= จอ LCD =================
String shown[4];   // จำข้อความเดิมไว้ เขียนเฉพาะบรรทัดที่เปลี่ยน จอจะไม่กะพริบ

void lcdLine(int row, String s) {
  while (s.length() < 20) s += ' ';
  s = s.substring(0, 20);
  if (!lcd || shown[row] == s) return;
  shown[row] = s;
  lcd->setCursor(0, row);
  lcd->print(s);
}

void screen(const String &a, const String &b, const String &c, const String &d) {
  lcdLine(0, a); lcdLine(1, b); lcdLine(2, c); lcdLine(3, d);
}

void initLcd() {
  Wire.begin(PIN_SDA, PIN_SCL);
  const uint8_t candidates[] = {0x27, 0x3F, 0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26,
                                0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E};
  for (uint8_t a : candidates) {
    Wire.beginTransmission(a);
    if (Wire.endTransmission() == 0) {
      lcd = new LiquidCrystal_I2C(a, 20, 4);
      lcd->init();
      lcd->backlight();
      return;
    }
  }
  Serial.println("ไม่พบจอ LCD! ตรวจสาย SDA/SCL/VCC/GND");
}

void updateScreen() {
  bool phone = WiFi.softAPgetStationNum() > 0;
  bool web = lastPollMs != 0 && millis() - lastPollMs < WEB_TIMEOUT_MS;

  if (!phone) {
    screen("NOT CONNECTED", "WiFi: " + String(AP_SSID), "Pass: " + String(AP_PASS), "Connect your phone");
  } else if (!web) {
    screen("PHONE CONNECTED", "Open web browser:", "http://192.168.4.1", "Waiting for web...");
  } else if (!inWater) {
    screen("TDS METER   [web OK]", "Waiting for water...", "Dip the TDS probe", "");
  } else if (isnan(webTds) || millis() - webResultMs > WEB_TIMEOUT_MS) {
    screen("TDS METER   [web OK]", "Calculating...", "", "mV  : " + String(lastMv, 0));
  } else {
    String t = webTds < 100 ? String(webTds, 1) : String(webTds, 0);
    screen("TDS (from phone)", "TDS : " + t + " ppm", "EC  : " + String(webEc, 0) + " uS/cm",
           "mV  : " + String(lastMv, 0));
  }
}

// ================= หน้าเว็บ =================
const char PAGE[] PROGMEM = R"PAGE(<!doctype html>
<html lang="th"><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>TDS Meter</title>
<style>
body{font-family:system-ui,sans-serif;margin:0;padding:16px;background:#eef4f5;color:#102a32}
.card{background:#fff;border:1px solid #d3e1e5;border-radius:12px;padding:16px;margin-bottom:12px}
h1{font-size:20px;margin:0 0 12px}
.big{font-size:52px;font-weight:700;font-variant-numeric:tabular-nums;line-height:1.1}
.unit{font-size:18px;color:#587480}
.muted{color:#587480;font-size:14px}
.row{display:flex;gap:8px;align-items:center;flex-wrap:wrap;margin-top:8px}
input{font:inherit;padding:8px;border:1px solid #d3e1e5;border-radius:8px;width:110px}
button{font:inherit;padding:8px 14px;border-radius:8px;border:1px solid #0b7480;background:#0b7480;color:#fff}
button.ghost{background:#fff;color:#0b7480}
.dot{display:inline-block;width:10px;height:10px;border-radius:50%;background:#b8352a;margin-right:6px}
.dot.on{background:#1d7f48}
code{font-size:13px}
</style></head><body>
<h1>เครื่องวัด TDS</h1>
<div class="card"><span class="dot" id="dot"></span><span id="status">กำลังเชื่อมต่อ...</span></div>
<div class="card">
  <div class="muted" id="hint">จุ่มหัววัดลงน้ำ</div>
  <div><span class="big" id="tds">-</span> <span class="unit">ppm</span></div>
  <div class="muted">EC <b id="ec">-</b> µS/cm · แรงดันจากเซนเซอร์ <b id="mv">-</b> mV</div>
</div>
<div class="card">
  <b>อุณหภูมิน้ำ</b> <span class="muted">(ใช้ชดเชยค่า)</span>
  <div class="row"><input id="temp" type="number" step="0.1"> °C <button class="ghost" id="saveTemp">บันทึก</button></div>
</div>
<div class="card">
  <b>คาลิเบรต</b> <span class="muted">ค่าตัวคูณ K ตอนนี้ = <b id="k">1.000</b></span>
  <p class="muted" style="margin:6px 0">จุ่มหัววัดในน้ำที่รู้ค่า TDS (เช่น วัดด้วยปากกา TDS) ใส่ค่าแล้วกดคาลิเบรต</p>
  <div class="row"><input id="ref" type="number" placeholder="ค่าจริง ppm"> <button id="cal">คาลิเบรต</button> <button class="ghost" id="reset">ล้างค่า</button></div>
  <div class="muted" id="calMsg"></div>
</div>
<div class="card muted">
  <b style="color:#102a32">สูตรที่ใช้คำนวณ</b><br>
  อ้างอิงไลบรารี GravityTDS ของ DFRobot ผู้ผลิตเซนเซอร์ TDS (github.com/DFRobot/GravityTDS)<br>
  <code>EC = (133.42V³ − 255.86V² + 857.39V) × K</code><br>
  <code>EC25 = EC ÷ (1 + 0.02 × (T − 25))</code><br>
  <code>TDS = EC25 × 0.5</code><br>
  V = แรงดันเป็นโวลต์ · T = อุณหภูมิน้ำ °C · K = ค่าจากการคาลิเบรต
</div>
<script>
const $ = id => document.getElementById(id);
const get = (k, d) => { try { const v = localStorage.getItem(k); return v === null ? d : parseFloat(v); } catch (e) { return d; } };
const put = (k, v) => { try { localStorage.setItem(k, v); } catch (e) {} };
let K = get('tdsK', 1), T = get('waterTemp', 25), last = null;
$('temp').value = T; $('k').textContent = K.toFixed(3);

// สูตรอ้างอิงจาก DFRobot GravityTDS
function calc(mv, k) {
  const v = mv / 1000;
  const ec = (133.42 * v * v * v - 255.86 * v * v + 857.39 * v) * k;
  const ec25 = ec / (1 + 0.02 * (T - 25));
  return { ec: ec25, tds: ec25 * 0.5 };
}
const fmt = t => t < 100 ? t.toFixed(1) : t.toFixed(0);

async function tick() {
  try {
    const r = await (await fetch('/api/raw', { cache: 'no-store' })).json();
    $('dot').className = 'dot on'; $('status').textContent = 'เชื่อมต่อเครื่องวัดแล้ว';
    $('mv').textContent = r.mv.toFixed(0);
    if (!r.in_water) {
      last = null;
      $('hint').textContent = 'จุ่มหัววัดลงน้ำ'; $('tds').textContent = '-'; $('ec').textContent = '-';
    } else {
      last = r;
      const c = calc(r.mv, K);
      $('hint').textContent = 'กำลังวัด (อัปเดตทุก 1 วินาที)';
      $('tds').textContent = fmt(c.tds); $('ec').textContent = c.ec.toFixed(0);
      await fetch('/api/result?seq=' + r.seq + '&tds=' + c.tds.toFixed(1) + '&ec=' + c.ec.toFixed(0), { method: 'POST' });
    }
  } catch (e) {
    $('dot').className = 'dot'; $('status').textContent = 'ขาดการเชื่อมต่อ ตรวจว่าต่อ WiFi WaterMeter อยู่';
  }
  setTimeout(tick, 1000);
}

$('saveTemp').onclick = () => { const v = parseFloat($('temp').value); if (v > 0 && v < 60) { T = v; put('waterTemp', v); } };
$('cal').onclick = () => {
  const ref = parseFloat($('ref').value);
  if (!last) { $('calMsg').textContent = 'จุ่มหัววัดลงน้ำก่อน'; return; }
  if (!(ref > 0)) { $('calMsg').textContent = 'ใส่ค่า TDS จริงก่อน'; return; }
  const k = ref / calc(last.mv, 1).tds;
  if (k < 0.25 || k > 4) { $('calMsg').textContent = 'ค่า K = ' + k.toFixed(2) + ' ผิดปกติ (ต้องอยู่ระหว่าง 0.25-4) ตรวจหัววัดหรือค่าที่ใส่'; return; }
  K = k; put('tdsK', k); $('k').textContent = k.toFixed(3);
  $('calMsg').textContent = 'คาลิเบรตแล้ว ค่า K ถูกเก็บในมือถือเครื่องนี้';
};
$('reset').onclick = () => { K = 1; put('tdsK', 1); $('k').textContent = '1.000'; $('calMsg').textContent = 'ล้างค่าคาลิเบรตแล้ว'; };
tick();
</script></body></html>)PAGE";

// ================= Web API =================
void handlePage() { server.send_P(200, "text/html; charset=utf-8", PAGE); }

void handleRaw() {
  lastPollMs = millis();
  String j = "{\"seq\":" + String(seq) + ",\"mv\":" + String(lastMv, 1) +
             ",\"in_water\":" + String(inWater ? "true" : "false") + "}";
  server.sendHeader("Cache-Control", "no-store");
  server.send(200, "application/json", j);
}

void handleResult() {
  lastPollMs = millis();
  if (server.hasArg("tds") && server.hasArg("ec")) {
    webTds = server.arg("tds").toFloat();
    webEc = server.arg("ec").toFloat();
    webResultMs = millis();
    Serial.printf("ค่าจากเว็บ: TDS %.1f ppm, EC %.0f uS/cm (แรงดัน %.0f mV)\n", webTds, webEc, lastMv);
  }
  server.send(200, "application/json", "{\"ok\":true}");
}

// ================= setup / loop =================
void setup() {
  Serial.begin(115200);
  analogReadResolution(12);
  initLcd();

  WiFi.mode(WIFI_AP);
  WiFi.softAP(AP_SSID, AP_PASS);
  server.enableCORS(true);
  server.on("/", HTTP_GET, handlePage);
  server.on("/api/raw", HTTP_GET, handleRaw);
  server.on("/api/result", HTTP_POST, handleResult);
  server.begin();
  Serial.printf("WiFi: %s  รหัส: %s  เว็บ: http://%s\n", AP_SSID, AP_PASS, WiFi.softAPIP().toString().c_str());
}

void loop() {
  static unsigned long lastRead = 0, lastDraw = 0;
  server.handleClient();

  if (millis() - lastRead >= 1000) {   // อ่านเซนเซอร์ทุก 1 วินาที
    lastRead = millis();
    lastMv = readMilliVolts();
    bool nowInWater = lastMv >= IN_WATER_MV;
    if (!nowInWater) webTds = NAN;      // ยกหัววัดขึ้น = ล้างค่าเก่า
    inWater = nowInWater;
    seq++;
  }
  if (millis() - lastDraw >= 250) {     // วาดจอ 4 ครั้งต่อวินาที
    lastDraw = millis();
    updateScreen();
  }
}
