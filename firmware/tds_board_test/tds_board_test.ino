/*
 * ทดสอบบอร์ด TDS (TDS Meter V1.0) — ค่าวิ่งทุก 1 วินาที + กดปุ่มเขียว = วัดเฉลี่ย 10 วินาที
 *
 * ต่อสาย (เหมือนภาพ wiring-tds)
 *   บอร์ด TDS  + -> 3V3    - -> G    A -> 4.7k -> S ของ D34   (D34 -> 10µF + 104 ลง G)
 *   หัววัดเสียบช่องขาว 2 ขาของบอร์ด TDS
 *   หัววัดอุณหภูมิ NTC (ถ้ามี): 3V3 -> 4.7k -> D32, NTC ระหว่าง D32 กับ G (ไม่ได้ต่อ = ใช้ 25 °C)
 *   ปุ่มเขียว: ขั้ว 23 -> S ของ D15, ขั้ว 24 -> G
 *   LCD: SDA -> D21, SCL -> D22, VCC -> VIN, GND -> G
 *
 * ค่าที่ควรเห็น
 *   หัววัดอยู่ในอากาศ : TDS <60 ppm   (A: 0 V)
 *   น้ำประปา          : ประมาณ 100-300 ppm
 *   น้ำเกลือ          : สูงกว่าน้ำประปามาก
 * หมายเหตุ: ESP32 อ่านแรงดันต่ำสุดได้ประมาณ 142 mV (= 0 V จริง) จึงวัด TDS ต่ำกว่าประมาณ 60 ppm ไม่ได้
 * ไลบรารี: "LiquidCrystal I2C" (by Frank de Brabander)
 */

#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <math.h>

const int   PIN_TDS    = 34;      // ขา A ของบอร์ด TDS (ผ่าน 4.7k)
const int   PIN_TEMP   = 32;      // NTC (ถ้าไม่ได้ต่อ จะใช้ 25 °C)
const int   PIN_BUTTON = 15;
const float TDS_K      = 1.0;     // ค่าปรับเทียบ: ถ้ารู้ค่าจริง K = ค่าจริง / ค่าที่วัดได้
const float FLOOR_MV   = 150.0;   // ต่ำกว่านี้ = ไฟจริง 0 V (ESP32 อ่านต่ำสุดได้ประมาณ 142 mV)
const float NTC_SERIES = 4700.0;
const float NTC_R25    = 10000.0;
const float NTC_BETA   = 3950.0;

LiquidCrystal_I2C *lcd = nullptr;
String avgText = "GREEN = avg 10s";

void lcdLine(int row, String s) {
  if (!lcd) return;
  while (s.length() < 20) s += ' ';
  lcd->setCursor(0, row);
  lcd->print(s.substring(0, 20));
}

// อ่าน 31 ครั้ง ใช้ค่ากลาง (mV)
int readMedianMv(int pin) {
  int buf[31];
  for (int i = 0; i < 31; i++) {
    buf[i] = analogReadMilliVolts(pin);
    delay(3);
  }
  for (int i = 1; i < 31; i++) {
    int v = buf[i], j = i - 1;
    while (j >= 0 && buf[j] > v) { buf[j + 1] = buf[j]; j--; }
    buf[j + 1] = v;
  }
  return buf[15];
}

// อุณหภูมิน้ำ (°C) หรือ NAN ถ้าไม่ได้ต่อ NTC
float readTemp() {
  int mv = readMedianMv(PIN_TEMP);
  if (mv > 3000 || mv < 100) return NAN;
  float r = NTC_SERIES * mv / (3300.0 - mv);
  float t = 1.0 / (1.0 / 298.15 + log(r / NTC_R25) / NTC_BETA) - 273.15;
  return (t < 0 || t > 60) ? NAN : t;
}

// แปลงแรงดันเป็น TDS (สูตร DFRobot) คืน 0 ถ้าต่ำกว่าที่ ESP32 อ่านได้
float tdsFromMv(float mv, float tempC) {
  if (mv < FLOOR_MV) return 0;
  float v = (mv / 1000.0) / (1.0 + 0.02 * (tempC - 25.0));   // ชดเชยอุณหภูมิ
  float tds = (133.42 * v * v * v - 255.86 * v * v + 857.39 * v) * 0.5 * TDS_K;
  return tds < 0 ? 0 : tds;
}

String tdsText(float tds) {
  return tds < 60 ? String("<60 ppm") : String(tds, 0) + " ppm";
}

void showLive() {
  float t = readTemp();
  float tUse = isnan(t) ? 25.0 : t;
  int mv = readMedianMv(PIN_TDS);
  float tds = tdsFromMv(mv, tUse);

  lcdLine(0, "TDS TEST T:" + (isnan(t) ? String("25(set)") : String(t, 1) + "C"));
  lcdLine(1, "TDS : " + tdsText(tds));
  lcdLine(2, mv < FLOOR_MV ? String("A   : 0 V (min)") : "A   : " + String(mv / 1000.0, 3) + " V");
  Serial.printf("TDS: %.0f ppm   A: %d mV   T: %.1f C\n", tds, mv, tUse);
}

void measureAvg() {
  const int N = 10;
  float sum = 0;
  for (int i = 0; i < N; i++) {
    lcdLine(3, "Measuring... " + String(N - i) + "s");
    float t = readTemp();
    sum += tdsFromMv(readMedianMv(PIN_TDS), isnan(t) ? 25.0 : t);
    delay(880);                                    // รวมเวลาอ่านค่า ประมาณ 1 วินาทีต่อครั้ง
  }
  float avg = sum / N;
  avgText = "AVG10s: " + tdsText(avg);
  lcdLine(3, avgText);
  Serial.printf("เฉลี่ย 10 วินาที: %.0f ppm\n", avg);
}

void setup() {
  Serial.begin(115200);
  pinMode(PIN_BUTTON, INPUT_PULLUP);

  Wire.begin(21, 22);
  const uint8_t addrs[] = {0x27, 0x3F, 0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26,
                           0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E};
  for (uint8_t a : addrs) {
    Wire.beginTransmission(a);
    if (Wire.endTransmission() == 0) {
      lcd = new LiquidCrystal_I2C(a, 20, 4);
      lcd->init();
      lcd->backlight();
      break;
    }
  }
  lcdLine(3, avgText);
}

void loop() {
  static unsigned long next = 0;

  if (digitalRead(PIN_BUTTON) == LOW) {
    delay(30);                                     // กันปุ่มเด้ง
    if (digitalRead(PIN_BUTTON) == LOW) {
      while (digitalRead(PIN_BUTTON) == LOW) delay(10);   // รอปล่อยปุ่ม
      measureAvg();
      next = millis() + 1000;
    }
  }

  if (millis() >= next) {
    next = millis() + 1000;
    showLive();
  }
  delay(10);
}
