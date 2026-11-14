/*
 * ทดสอบหัววัดอุณหภูมิ NTC 10k (B3950) กับ ESP32 — ค่าวิ่งทุก 1 วินาที ไม่ต้องกดปุ่ม
 *
 * หลักการ
 *   3V3 -> 4.7k -> จุด T -> NTC -> G   (ตัวแบ่งแรงดัน)
 *   น้ำร้อนขึ้น ความต้านทาน NTC ลดลง แรงดันที่จุด T จึงลดลง
 *     R_NTC  = 4700 x mV / (3300 - mV)
 *     T (°C) = 1 / (1/298.15 + ln(R_NTC / 10000) / 3950) - 273.15
 *
 * ต่อสาย (บอร์ดขยาย ESP32)
 *   3V3 -> ตัวต้านทาน 4.7k -> จุด T ;  จุด T -> S ของ D32
 *   NTC สาย 1 -> จุด T ;  NTC สาย 2 -> G   (สลับสายได้ ไม่มีขั้ว)
 *   จุด T -> 10µF (+) และ 104 ลง G  (กรองสัญญาณ)
 *   LCD: SDA -> D21, SCL -> D22, VCC -> VIN, GND -> G
 * ไลบรารี: "LiquidCrystal I2C" (by Frank de Brabander)
 */

#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <math.h>

const int   PIN_TEMP    = 32;
const float VCC_MV      = 3300.0;   // ไฟเลี้ยงตัวแบ่งแรงดัน (3V3) วัดจริงด้วยมิเตอร์แล้วแก้ได้
const float NTC_SERIES  = 4700.0;   // ตัวต้านทานที่ต่อกับ NTC (วัดจริงแล้วแก้ได้)
const float NTC_R25     = 10000.0;  // NTC 10k ที่ 25 °C
const float NTC_BETA    = 3950.0;   // ค่า B ของ NTC
const float TEMP_OFFSET = 0.0;      // ถ้าเทียบเทอร์โมมิเตอร์แล้วต่างคงที่ ใส่ผลต่างตรงนี้ (เช่น -0.8)

LiquidCrystal_I2C *lcd = nullptr;

void lcdLine(int row, String s) {
  if (!lcd) return;
  while (s.length() < 20) s += ' ';
  lcd->setCursor(0, row);
  lcd->print(s.substring(0, 20));
}

// อ่าน 31 ครั้ง ใช้ค่ากลาง (mV)
int readMv() {
  int buf[31];
  for (int i = 0; i < 31; i++) {
    buf[i] = analogReadMilliVolts(PIN_TEMP);
    delay(5);
  }
  for (int i = 1; i < 31; i++) {
    int v = buf[i], j = i - 1;
    while (j >= 0 && buf[j] > v) { buf[j + 1] = buf[j]; j--; }
    buf[j + 1] = v;
  }
  return buf[15];
}

void setup() {
  Serial.begin(115200);

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
  lcdLine(0, "TEMP METER (NTC)");
  Serial.println("D32_mV,R_ohm,temp_C");
}

void loop() {
  int mv = readMv();

  if (mv > 3000) {            // แรงดันเต็ม = สาย NTC หลุด / ไม่ได้ต่อ
    lcdLine(1, "Temp: -");
    lcdLine(2, "NTC not connected");
    lcdLine(3, "D32 : " + String(mv) + " mV");
    Serial.printf("%d,-,- (NTC not connected)\n", mv);
  } else if (mv < 100) {      // แรงดันเกือบ 0 = สายช็อต หรือไม่มีไฟ 3V3 / ไม่ได้ต่อ 4.7k
    lcdLine(1, "Temp: -");
    lcdLine(2, "Check 4.7k / 3V3");
    lcdLine(3, "D32 : " + String(mv) + " mV");
    Serial.printf("%d,-,- (check 4.7k / 3V3)\n", mv);
  } else {
    float r = NTC_SERIES * mv / (VCC_MV - mv);
    float t = 1.0 / (1.0 / 298.15 + log(r / NTC_R25) / NTC_BETA) - 273.15 + TEMP_OFFSET;
    lcdLine(1, "Temp: " + String(t, 1) + " C");
    lcdLine(2, "R   : " + String(r, 0) + " ohm");
    lcdLine(3, "D32 : " + String(mv) + " mV");
    Serial.printf("%d,%.0f,%.1f\n", mv, r, t);
  }
  delay(1000);
}
