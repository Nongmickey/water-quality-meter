/*
 * ทดสอบเซนเซอร์ pH (โมดูล pH + หัววัด BNC) กับ ESP32
 *
 * การใช้งาน
 *   กดปุ่มเขียว (สั้น)        = วัด pH แสดงบนจอ
 *   กดค้าง 3 วินาทีแล้วปล่อย = คาลิเบรตจุด pH 6.86 (จุ่มหัววัดในน้ำยา pH 6.86 ก่อน)
 *   กดค้าง 6 วินาทีแล้วปล่อย = คาลิเบรตจุด pH 4.00 (จุ่มหัววัดในน้ำยา pH 4.00 ก่อน)
 *   ค่าคาลิเบรตเก็บในบอร์ด ไม่หายเมื่อถอดปลั๊ก
 *
 * ต่อสาย (บอร์ดขยาย ESP32)
 *   โมดูล pH  V+ -> VIN (5V)      G -> G
 *   โมดูล pH  Po -> 4.7k -> จุด P ;  จุด P -> 4.7k -> G ;  จุด P -> S ของ D35
 *   จุด P -> 10µF (+) และ 104 ลง G  (กรองสัญญาณ)
 *   ปุ่มเขียว: ขั้ว 23 -> S ของ D15, ขั้ว 24 -> G
 *   LCD: SDA -> D21, SCL -> D22, VCC -> VIN, GND -> G
 *   ขา To, Do ของโมดูล pH ไม่ใช้
 * ไลบรารี: "LiquidCrystal I2C" (by Frank de Brabander)
 */

#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Preferences.h>

const int   PIN_PH     = 35;     // จุด P (หลังตัวแบ่งแรงดัน)
const int   PIN_BUTTON = 15;
const float PH_DIVIDER = 2.0;    // ตัวแบ่งแรงดัน 4.7k + 4.7k: แรงดันจริง = ที่อ่านได้ x 2
const float CAL_HIGH   = 6.86;   // น้ำยาคาลิเบรตจุดที่ 1
const float CAL_LOW    = 4.00;   // น้ำยาคาลิเบรตจุดที่ 2

LiquidCrystal_I2C *lcd = nullptr;
Preferences prefs;
float mvHigh = 2500.0;   // แรงดันโมดูล (mV) ในน้ำยา pH 6.86 (ค่าเริ่มต้นโดยประมาณ)
float mvLow  = 3020.0;   // แรงดันโมดูล (mV) ในน้ำยา pH 4.00 (ค่าเริ่มต้นโดยประมาณ)

void lcdLine(int row, String s) {
  if (!lcd) return;
  while (s.length() < 20) s += ' ';
  lcd->setCursor(0, row);
  lcd->print(s.substring(0, 20));
}

// อ่านแรงดันที่ขา Po ของโมดูล (mV): อ่าน 31 ครั้ง ใช้ค่ากลาง แล้วคูณกลับตัวแบ่งแรงดัน
float readModuleMv() {
  int buf[31];
  for (int i = 0; i < 31; i++) {
    buf[i] = analogReadMilliVolts(PIN_PH);
    delay(10);
  }
  for (int i = 1; i < 31; i++) {
    int v = buf[i], j = i - 1;
    while (j >= 0 && buf[j] > v) { buf[j + 1] = buf[j]; j--; }
    buf[j + 1] = v;
  }
  return buf[15] * PH_DIVIDER;
}

float mvToPh(float mv) {
  float diff = mvHigh - mvLow;
  if (fabs(diff) < 50) diff = -520;   // คาลิเบรตสองจุดได้ค่าใกล้กันเกินไป (จุ่มน้ำยาผิด) ใช้ค่ามาตรฐานแทน
  float slope = (CAL_HIGH - CAL_LOW) / diff;   // pH ต่อ mV
  float ph = CAL_HIGH + slope * (mv - mvHigh);
  return constrain(ph, 0.0, 14.0);
}

void measure() {
  lcdLine(2, "Measuring...");
  float mv = readModuleMv();
  if (mv < 300) {   // แรงดันต่ำผิดปกติ = ไม่ได้ต่อโมดูล pH หรือสายหลุด
    lcdLine(1, "pH  : -");
    lcdLine(2, "Check pH module");
    lcdLine(3, "Po: " + String(mv / 1000.0, 3) + " V");
    Serial.printf("pH: -  (Po = %.0f mV, ตรวจสาย)\n", mv);
    return;
  }
  float ph = mvToPh(mv);
  lcdLine(1, "pH  : " + String(ph, 2));
  lcdLine(2, "Po  : " + String(mv / 1000.0, 3) + " V");
  lcdLine(3, "GREEN = measure");
  Serial.printf("pH: %.2f   Po: %.0f mV\n", ph, mv);
}

void calibrate(bool high) {
  float mv = readModuleMv();
  if (high) { mvHigh = mv; prefs.putFloat("mvHigh", mv); }
  else      { mvLow = mv;  prefs.putFloat("mvLow", mv); }
  lcdLine(1, "CAL pH " + String(high ? CAL_HIGH : CAL_LOW, 2) + " OK");
  lcdLine(2, "Po  : " + String(mv / 1000.0, 3) + " V");
  lcdLine(3, "Saved");
  Serial.printf("คาลิเบรต pH %.2f: Po = %.0f mV\n", high ? CAL_HIGH : CAL_LOW, mv);
}

void setup() {
  Serial.begin(115200);
  pinMode(PIN_BUTTON, INPUT_PULLUP);
  prefs.begin("ph", false);
  mvHigh = prefs.getFloat("mvHigh", mvHigh);
  mvLow  = prefs.getFloat("mvLow", mvLow);

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
  lcdLine(0, "pH METER");
  lcdLine(2, "Press GREEN button");
  lcdLine(3, "to measure");
}

void loop() {
  if (digitalRead(PIN_BUTTON) != LOW) return;

  unsigned long t0 = millis();
  int stage = 0;   // 0 = กดสั้น, 1 = ค้าง 3 วินาที, 2 = ค้าง 6 วินาที
  while (digitalRead(PIN_BUTTON) == LOW) {
    unsigned long held = millis() - t0;
    if (stage < 2 && held >= 6000) { stage = 2; lcdLine(1, "Release: CAL 4.00"); }
    else if (stage < 1 && held >= 3000) { stage = 1; lcdLine(1, "Release: CAL 6.86"); }
    delay(10);
  }

  if (stage == 0) measure();
  else calibrate(stage == 1);
  delay(50);
}
