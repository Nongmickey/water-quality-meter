/*
 * ทดสอบเซนเซอร์ TDS + จอ LCD + ปุ่ม
 *
 * กดปุ่มสีเขียว -> วัดค่า TDS ทุก 1 วินาที นาน 10 วินาที
 * แล้วแสดงค่าเฉลี่ย ค่าต่ำสุด ค่าสูงสุด และช่วงแกว่ง บนจอ
 * ทุกค่าที่วัดได้จะพิมพ์ออก Serial Monitor (115200) ด้วย เอาไปใส่ตารางในรายงานได้
 *
 * ต่อสาย: TDS A -> 4.7k -> D34 (+10µF, 104 ลง GND) · ปุ่ม D15/GND · LCD SDA D21, SCL D22
 * ไลบรารี: "LiquidCrystal I2C" (by Frank de Brabander)
 */

#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <math.h>

// ---------- ตั้งค่า ----------
const int   PIN_TDS     = 34;
const int   PIN_BUTTON  = 15;
const int   PIN_SDA     = 21;
const int   PIN_SCL     = 22;
const int   SECONDS     = 10;     // วัดกี่วินาที
const float WATER_TEMP  = 25.0;   // อุณหภูมิน้ำที่ใช้ชดเชย (°C) แก้ให้ตรงกับน้ำจริงได้
const float TDS_K       = 1.0;    // ตัวคูณคาลิเบรต
const float TDS_MIN_MV  = 10.0;   // ต่ำกว่านี้ = หัววัดไม่ได้จุ่มน้ำ

LiquidCrystal_I2C *lcd = nullptr;

// ---------- อ่านแรงดัน (ค่ากลางจาก 25 ครั้ง) ----------
float readMilliVolts() {
  const int N = 25;
  int buf[N];
  for (int i = 0; i < N; i++) {
    buf[i] = analogReadMilliVolts(PIN_TDS);
    delayMicroseconds(800);
  }
  for (int i = 1; i < N; i++) {
    int v = buf[i], j = i - 1;
    while (j >= 0 && buf[j] > v) { buf[j + 1] = buf[j]; j--; }
    buf[j + 1] = v;
  }
  return buf[N / 2];
}

// ---------- แปลงแรงดันเป็น ppm (คืน NAN ถ้าหัววัดไม่อยู่ในน้ำ) ----------
float readTds(float &mvOut) {
  float mv = readMilliVolts();
  mvOut = mv;
  if (mv < TDS_MIN_MV) return NAN;
  float v = (mv / 1000.0) / (1.0 + 0.02 * (WATER_TEMP - 25.0));
  float tds = (133.42 * v * v * v - 255.86 * v * v + 857.39 * v) * 0.5 * TDS_K;
  return tds < 0 ? 0 : tds;
}

// ---------- จอ ----------
void lcdLine(int row, String s) {
  if (!lcd) return;
  while (s.length() < 20) s += ' ';
  lcd->setCursor(0, row);
  lcd->print(s.substring(0, 20));
}

// ยังไม่ได้จุ่มหัววัดลงน้ำ = แสดง 0 ppm (ไม่นำไปคิดค่าเฉลี่ย)
String ppm(float v) { return (isnan(v) ? String("0") : String(v, 0)) + " ppm"; }

void initLcd() {
  Wire.begin(PIN_SDA, PIN_SCL);
  // โมดูล I2C ของจอใช้ที่อยู่ 0x20-0x27 (PCF8574) หรือ 0x38-0x3F (PCF8574A) ไล่หาทุกค่า
  const uint8_t candidates[] = {0x27, 0x3F, 0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26,
                                0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E};
  for (uint8_t a : candidates) {
    Wire.beginTransmission(a);
    if (Wire.endTransmission() == 0) {
      lcd = new LiquidCrystal_I2C(a, 20, 4);
      lcd->init();
      lcd->backlight();
      Serial.printf("พบจอ LCD ที่ 0x%02X\n", a);
      return;
    }
  }
  Serial.println("ไม่พบจอ LCD! ตรวจสาย SDA/SCL/VCC/GND");
}

// ---------- ปุ่ม (กดแล้วปล่อย = true) ----------
bool buttonPressed() {
  static bool last = HIGH;
  static unsigned long changedAt = 0;
  bool now = digitalRead(PIN_BUTTON);
  if (now != last && millis() - changedAt > 40) {
    changedAt = millis();
    last = now;
    if (now == HIGH) return true;   // ปล่อยปุ่ม
  }
  return false;
}

// ---------- วัด 10 วินาที ----------
void runTest() {
  float sum = 0, mn = 1e9, mx = -1e9;
  int n = 0;
  Serial.println("\n=== เริ่มวัด TDS ===");
  Serial.println("วินาที,TDS(ppm),แรงดัน(mV)");

  for (int i = 1; i <= SECONDS; i++) {
    unsigned long start = millis();
    float mv;
    float t = readTds(mv);
    if (!isnan(t)) {
      sum += t; n++;
      mn = min(mn, t); mx = max(mx, t);
    }
    Serial.printf("%d,%.0f,%.0f\n", i, isnan(t) ? 0.0f : t, mv);

    String bar = "";
    for (int k = 0; k < 20; k++) bar += (k < i * 20 / SECONDS) ? '#' : '.';
    lcdLine(0, "Measuring " + String(i) + "/" + String(SECONDS));
    lcdLine(1, "Now : " + ppm(t));
    lcdLine(2, "Avg : " + ppm(n ? sum / n : NAN));
    lcdLine(3, bar);

    while (millis() - start < 1000) delay(5);   // ให้ครบ 1 วินาทีพอดี
  }

  if (n == 0) {
    lcdLine(0, "RESULT: no reading");
    lcdLine(1, "Put probe in water");
    lcdLine(2, "");
    lcdLine(3, "Press GREEN again");
    Serial.println("ไม่มีค่าที่อ่านได้ (หัววัดไม่ได้จุ่มน้ำ?)");
    return;
  }

  float avg = sum / n;
  lcdLine(0, "RESULT " + String(n) + "/" + String(SECONDS) + " ok");
  lcdLine(1, "Avg : " + String(avg, 0) + " ppm");
  lcdLine(2, "Min " + String(mn, 0) + " Max " + String(mx, 0));
  lcdLine(3, "Range: " + String(mx - mn, 0) + " ppm");
  Serial.printf("ผล: เฉลี่ย %.1f ppm | ต่ำสุด %.0f | สูงสุด %.0f | แกว่ง %.0f ppm | อ่านได้ %d/%d ครั้ง\n",
                avg, mn, mx, mx - mn, n, SECONDS);
}

void setup() {
  Serial.begin(115200);
  delay(200);
  analogReadResolution(12);
  pinMode(PIN_BUTTON, INPUT_PULLUP);
  initLcd();
  lcdLine(0, "TDS TEST");
  lcdLine(3, "GREEN = measure 10s");
  Serial.println("กดปุ่มสีเขียวเพื่อเริ่มวัด 10 วินาที");
}

void loop() {
  static unsigned long lastLive = 0;
  static bool showingResult = false;

  if (buttonPressed()) {
    runTest();
    showingResult = true;
    return;
  }

  // ระหว่างรอ แสดงค่าสดทุกครึ่งวินาที (ถ้ายังไม่มีผลค้างบนจอ)
  if (!showingResult && millis() - lastLive > 500) {
    lastLive = millis();
    float mv;
    float t = readTds(mv);
    lcdLine(1, "Live: " + ppm(t));
    lcdLine(2, "mV  : " + String(mv, 0));
  }
}
