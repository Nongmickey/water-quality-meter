/*
 * ดูค่า TDS แบบสด: วัดทุก 1 วินาทีไปเรื่อยๆ ไม่จับเวลา
 *
 * จอ LCD แสดง: ค่าตอนนี้ แรงดัน (mV) ค่าต่ำสุด/สูงสุด และวินาทีที่ผ่านไป
 * ยังไม่ได้จุ่มหัววัดลงน้ำ = แสดง 0 ppm (ไม่นำไปคิดค่าต่ำสุด/สูงสุด)
 * กดปุ่มสีเขียว = ล้างค่าต่ำสุด/สูงสุด แล้วเริ่มนับใหม่
 * Serial Monitor (115200) พิมพ์ทุกวินาที: วินาที,TDS(ppm),แรงดัน(mV)
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
const float WATER_TEMP  = 25.0;   // อุณหภูมิน้ำที่ใช้ชดเชย (°C)
const float TDS_K       = 1.0;    // ตัวคูณคาลิเบรต
const float TDS_MIN_MV  = 10.0;   // ต่ำกว่านี้ = หัววัดไม่ได้จุ่มน้ำ

LiquidCrystal_I2C *lcd = nullptr;

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

float readTds(float &mvOut) {
  float mv = readMilliVolts();
  mvOut = mv;
  if (mv < TDS_MIN_MV) return NAN;
  float v = (mv / 1000.0) / (1.0 + 0.02 * (WATER_TEMP - 25.0));
  float tds = (133.42 * v * v * v - 255.86 * v * v + 857.39 * v) * 0.5 * TDS_K;
  return tds < 0 ? 0 : tds;
}

void lcdLine(int row, String s) {
  if (!lcd) return;
  while (s.length() < 20) s += ' ';
  lcd->setCursor(0, row);
  lcd->print(s.substring(0, 20));
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

bool buttonPressed() {
  static bool last = HIGH;
  static unsigned long changedAt = 0;
  bool now = digitalRead(PIN_BUTTON);
  if (now != last && millis() - changedAt > 40) {
    changedAt = millis();
    last = now;
    if (now == HIGH) return true;
  }
  return false;
}

float mn = NAN, mx = NAN;
unsigned long startMs = 0;

void resetStats() {
  mn = NAN;
  mx = NAN;
  startMs = millis();
  Serial.println("\n--- เริ่มนับใหม่ ---");
  Serial.println("วินาที,TDS(ppm),แรงดัน(mV)");
}

void setup() {
  Serial.begin(115200);
  delay(200);
  analogReadResolution(12);
  pinMode(PIN_BUTTON, INPUT_PULLUP);
  initLcd();
  resetStats();
}

void loop() {
  static unsigned long lastRead = 0;

  if (buttonPressed()) {
    resetStats();
    lastRead = 0;   // อ่านค่าทันที
  }

  if (lastRead == 0 || millis() - lastRead >= 1000) {
    lastRead = millis();
    float mv;
    float t = readTds(mv);
    if (!isnan(t)) {
      mn = isnan(mn) ? t : min(mn, t);
      mx = isnan(mx) ? t : max(mx, t);
    }
    unsigned long sec = (millis() - startMs) / 1000;

    lcdLine(0, "TDS LIVE     " + String(sec) + " s");
    lcdLine(1, "Now : " + (isnan(t) ? String("0") : String(t, 0)) + " ppm");
    lcdLine(2, "mV  : " + String(mv, 0));
    lcdLine(3, isnan(mn) ? String("Min 0  Max 0")
                         : "Min " + String(mn, 0) + "  Max " + String(mx, 0));

    Serial.printf("%lu,%.0f,%.0f\n", sec, isnan(t) ? 0.0f : t, mv);
  }
}
