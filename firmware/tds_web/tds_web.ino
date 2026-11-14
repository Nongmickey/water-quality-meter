/*
 * เครื่องวัด TDS ที่คำนวณค่าผ่านหน้าเว็บบนมือถือ
 *
 * ขั้นตอนการทำงาน
 *   1) ยังไม่มีมือถือต่อ WiFi      -> จอขึ้น NOT CONNECTED พร้อมชื่อ WiFi และรหัส
 *   2) มือถือต่อ WiFi แล้ว           -> จอบอกให้เปิดเว็บ 192.168.4.1
 *   3) เปิดเว็บแล้ว                  -> จอขึ้น "Waiting for water"
 *   4) จุ่มหัววัดลงน้ำ                -> ESP32 ส่งแรงดันดิบ (mV) ไปที่เว็บ
 *   5) เว็บคำนวณ TDS ด้วยสูตรอ้างอิงของ DFRobot (ผู้ผลิตเซนเซอร์) 2 แบบ:
 *      ค่าจริงตามสูตรมาตรฐาน (K = 1) และค่าที่คาลิเบรตแล้ว
 *      แล้วส่งค่าที่ดีที่สุด (คาลิเบรตแล้วถ้ามี) + ผลสรุปคุณภาพน้ำ กลับมาแสดงบนจอ LCD
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
String webSrc = "STD";          // STD = สูตรมาตรฐาน K=1, CAL = คาลิเบรตแล้ว
int webQuality = -1;            // ระดับคุณภาพน้ำที่เว็บสรุปมา (0-5)
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

String qualityText(int q) {
  switch (q) {
    case 0: return "LOW MINERAL";
    case 1: return "EXCELLENT";
    case 2: return "GOOD";
    case 3: return "FAIR";
    case 4: return "POOR";
    case 5: return "BAD";
  }
  return "-";
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
    screen("TDS [" + webSrc + "] from phone", "TDS : " + t + " ppm", "EC  : " + String(webEc, 0) + " uS/cm",
           "Water: " + qualityText(webQuality));
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
.two{display:grid;grid-template-columns:1fr 1fr;gap:8px;margin-top:6px}
.col{border:1px solid #d3e1e5;border-radius:10px;padding:10px}
.col.sel{border:2px solid #0b7480;background:#f2fafa}
.mid{font-size:32px;font-weight:700;font-variant-numeric:tabular-nums}
.verdict{font-size:24px;font-weight:700;padding:10px 12px;border-radius:10px;margin:8px 0;background:#e6eced;color:#587480}
.verdict.good{background:#dcf1e4;color:#1d7f48}.verdict.warn{background:#f6ead2;color:#9a6512}.verdict.bad{background:#f8dfdb;color:#b8352a}
.bands{width:100%;border-collapse:collapse;font-size:13px;margin-top:8px}
.bands td{padding:4px 6px;border-top:1px solid #e3ecee}
.bands tr.on td{background:#f2fafa;color:#102a32;font-weight:700}
</style></head><body>
<h1>เครื่องวัด TDS</h1>
<div class="card"><span class="dot" id="dot"></span><span id="status">กำลังเชื่อมต่อ...</span></div>
<div class="card">
  <div class="muted" id="hint">จุ่มหัววัดลงน้ำ</div>
  <div class="two">
    <div class="col" id="colStd"><div class="muted">ค่าจริงจากเซนเซอร์<br>สูตรมาตรฐาน (K = 1)</div>
      <span class="mid" id="tdsStd">-</span> <span class="unit">ppm</span></div>
    <div class="col" id="colCal"><div class="muted">ค่าที่คาลิเบรตแล้ว<br><span id="kShow">ยังไม่คาลิเบรต</span></div>
      <span class="mid" id="tdsCal">-</span> <span class="unit">ppm</span></div>
  </div>
  <div class="muted" style="margin-top:8px">ส่งไปแสดงบนจอเครื่อง: <b id="sent">-</b></div>
  <div class="muted">EC <b id="ec">-</b> µS/cm · แรงดันจากเซนเซอร์ <b id="mv">-</b> mV</div>
</div>
<div class="card">
  <b>สรุปคุณภาพน้ำ</b>
  <div class="verdict" id="verdict">รอวัดน้ำ</div>
  <div class="muted" id="why"></div>
  <table class="bands muted">
    <tr id="b0"><td>ต่ำกว่า 50 ppm</td><td>แร่ธาตุน้อยมาก (ดื่มได้)</td></tr>
    <tr id="b1"><td>50 – 300 ppm</td><td>ดีมาก</td></tr>
    <tr id="b2"><td>300 – 600 ppm</td><td>ดี</td></tr>
    <tr id="b3"><td>600 – 900 ppm</td><td>พอใช้</td></tr>
    <tr id="b4"><td>900 – 1200 ppm</td><td>ไม่ดี</td></tr>
    <tr id="b5"><td>มากกว่า 1200 ppm</td><td>แย่ ไม่ควรดื่ม</td></tr>
  </table>
  <p class="muted" style="margin:8px 0 0">เกณฑ์รสชาติน้ำดื่มของ WHO · เกณฑ์น้ำดื่มบรรจุขวดของไทยกำหนด TDS ไม่เกิน 500 ppm · TDS อย่างเดียวบอกไม่ได้ว่ามีเชื้อโรคหรือโลหะหนักหรือไม่</p>
</div>
<div class="card">
  <b>อุณหภูมิน้ำ</b> <span class="muted">(ใช้ชดเชยค่า)</span>
  <div class="row"><input id="temp" type="number" step="0.1"> °C <button class="ghost" id="saveTemp">บันทึก</button></div>
</div>
<div class="card">
  <b>คาลิเบรต</b> <span class="muted">ค่าตัวคูณ K ตอนนี้ = <b id="k">1.000</b></span>
  <p class="muted" style="margin:6px 0">ตามไลบรารี GravityTDS ของ DFRobot: จุ่มหัววัดในน้ำยามาตรฐาน <b>1413 µS/cm (= 707 ppm)</b> ที่ 25 °C แล้วกดปุ่มด้านล่าง</p>
  <div class="row"><button id="cal707">คาลิเบรตด้วยน้ำยามาตรฐาน 707 ppm</button></div>
  <p class="muted" style="margin:10px 0 0">หรือใช้น้ำที่รู้ค่าอื่น (เช่น วัดด้วยปากกา TDS) ใส่ค่าแล้วกดคาลิเบรต</p>
  <div class="row"><input id="ref" type="number" placeholder="ค่าจริง ppm"> <button id="cal">คาลิเบรต</button> <button class="ghost" id="reset">ใช้ค่ามาตรฐาน K = 1</button></div>
  <div class="muted" id="calMsg"></div>
  <table class="muted" style="margin-top:10px;border-collapse:collapse;font-size:13px">
    <tr><td style="padding:2px 8px 2px 0">ค่า K เริ่มต้น (ยังไม่คาลิเบรต)</td><td><b>1.0</b></td></tr>
    <tr><td style="padding:2px 8px 2px 0">น้ำยามาตรฐานที่แนะนำ</td><td><b>707 ppm (1413 µS/cm) @ 25 °C</b></td></tr>
    <tr><td style="padding:2px 8px 2px 0">ตัวคูณ TDS (TdsFactor)</td><td><b>0.5</b> (TDS = EC ÷ 2)</td></tr>
    <tr><td style="padding:2px 8px 2px 0">ชดเชยอุณหภูมิ</td><td><b>2% ต่อ °C</b> เทียบที่ 25 °C</td></tr>
    <tr><td style="padding:2px 8px 2px 0">ค่า K ที่ยอมรับได้</td><td><b>0.25 – 4.0</b></td></tr>
  </table>
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
let K = get('tdsK', 1), T = get('waterTemp', 25), calibrated = get('tdsCal', 0) === 1, last = null;
$('temp').value = T;
function showK() {
  $('k').textContent = K.toFixed(3);
  $('kShow').textContent = calibrated ? 'K = ' + K.toFixed(3) : 'ยังไม่คาลิเบรต';
}
showK();

// เกณฑ์คุณภาพน้ำจาก TDS
const BANDS = [
  { max: 50, cls: 'good', text: 'น้ำดี (แร่ธาตุน้อยมาก)', why: 'น้ำบริสุทธิ์มาก เช่น น้ำ RO น้ำกลั่น น้ำฝน ดื่มได้ แต่แทบไม่มีแร่ธาตุ' },
  { max: 300, cls: 'good', text: 'น้ำดีมาก', why: 'ต่ำกว่า 300 ppm ระดับที่ WHO ถือว่ารสชาติดีมาก และผ่านเกณฑ์น้ำดื่มบรรจุขวดของไทย' },
  { max: 600, cls: 'good', text: 'น้ำดี', why: 'WHO ถือว่าอยู่ในระดับดี ถ้าเกิน 500 ppm จะเกินเกณฑ์น้ำดื่มบรรจุขวดของไทย' },
  { max: 900, cls: 'warn', text: 'พอใช้', why: 'แร่ธาตุค่อนข้างมาก อาจมีรสกร่อยหรือทำให้เกิดคราบตะกรัน' },
  { max: 1200, cls: 'bad', text: 'น้ำไม่ดี', why: 'สารละลายสูง ไม่เหมาะดื่ม ควรกรองด้วยเครื่องกรอง RO ก่อน' },
  { max: Infinity, cls: 'bad', text: 'น้ำไม่ดี', why: 'สารละลายสูงมาก ไม่ควรดื่ม' }
];
function quality(tds) { const i = BANDS.findIndex(b => tds < b.max); return i < 0 ? 5 : i; }
function showVerdict(q, tds) {
  for (let i = 0; i < BANDS.length; i++) $('b' + i).className = i === q ? 'on' : '';
  if (q < 0) { $('verdict').className = 'verdict'; $('verdict').textContent = 'รอวัดน้ำ'; $('why').textContent = ''; return; }
  $('verdict').className = 'verdict ' + BANDS[q].cls;
  $('verdict').textContent = BANDS[q].text + ' (' + fmt(tds) + ' ppm)';
  $('why').textContent = BANDS[q].why + (tds >= 1000 ? ' · ค่าเกินช่วงของเซนเซอร์ (1000 ppm) ค่าจริงอาจสูงกว่านี้' : '');
}

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
      $('hint').textContent = 'จุ่มหัววัดลงน้ำ';
      for (const id of ['tdsStd', 'tdsCal', 'ec', 'sent']) $(id).textContent = '-';
      $('colStd').className = 'col'; $('colCal').className = 'col';
      showVerdict(-1);
    } else {
      last = r;
      const std = calc(r.mv, 1);                       // ค่าจริงตามสูตรมาตรฐาน
      const cal = calibrated ? calc(r.mv, K) : null;   // ค่าที่คาลิเบรตแล้ว
      const best = cal || std;                         // ค่าที่ดีที่สุด ส่งไปที่จอ
      const src = cal ? 'CAL' : 'STD';
      const q = quality(best.tds);
      $('hint').textContent = 'กำลังวัด (อัปเดตทุก 1 วินาที)';
      $('tdsStd').textContent = fmt(std.tds);
      $('tdsCal').textContent = cal ? fmt(cal.tds) : '-';
      $('colStd').className = cal ? 'col' : 'col sel';
      $('colCal').className = cal ? 'col sel' : 'col';
      $('ec').textContent = best.ec.toFixed(0);
      $('sent').textContent = fmt(best.tds) + ' ppm (' + (cal ? 'ค่าที่คาลิเบรตแล้ว' : 'สูตรมาตรฐาน ยังไม่คาลิเบรต') + ')';
      showVerdict(q, best.tds);
      await fetch('/api/result?seq=' + r.seq + '&tds=' + best.tds.toFixed(1) + '&ec=' + best.ec.toFixed(0) +
                  '&src=' + src + '&q=' + q, { method: 'POST' });
    }
  } catch (e) {
    $('dot').className = 'dot'; $('status').textContent = 'ขาดการเชื่อมต่อ ตรวจว่าต่อ WiFi WaterMeter อยู่';
  }
  setTimeout(tick, 1000);
}

$('saveTemp').onclick = () => { const v = parseFloat($('temp').value); if (v > 0 && v < 60) { T = v; put('waterTemp', v); } };
function calibrate(ref) {
  if (!last) { $('calMsg').textContent = 'จุ่มหัววัดลงน้ำก่อน'; return; }
  if (!(ref > 0)) { $('calMsg').textContent = 'ใส่ค่า TDS จริงก่อน'; return; }
  const k = ref / calc(last.mv, 1).tds;
  if (k < 0.25 || k > 4) { $('calMsg').textContent = 'ค่า K = ' + k.toFixed(2) + ' ผิดปกติ (ต้องอยู่ระหว่าง 0.25-4) ตรวจหัววัดหรือค่าที่ใส่'; return; }
  K = k; calibrated = true; put('tdsK', k); put('tdsCal', 1); showK();
  $('calMsg').textContent = 'คาลิเบรตด้วย ' + ref + ' ppm แล้ว ค่า K ถูกเก็บในมือถือเครื่องนี้';
}
$('cal').onclick = () => calibrate(parseFloat($('ref').value));
$('cal707').onclick = () => calibrate(707);
$('reset').onclick = () => { K = 1; calibrated = false; put('tdsK', 1); put('tdsCal', 0); showK(); $('calMsg').textContent = 'กลับไปใช้ค่ามาตรฐานของไลบรารี K = 1.0'; };
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
    webSrc = server.arg("src") == "CAL" ? "CAL" : "STD";
    webQuality = server.hasArg("q") ? server.arg("q").toInt() : -1;
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
