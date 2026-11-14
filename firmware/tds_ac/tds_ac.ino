/*
 * เครื่องวัด TDS แบบไม่ใช้บอร์ด TDS: ให้ ESP32 สร้างไฟสลับเอง
 *
 * หลักการ
 *   ต่อหัววัด (ขั้วโลหะ 2 ขั้ว) อนุกรมกับตัวต้านทาน 4.7k ระหว่างขา D25 กับ D26
 *   ESP32 สลับขั้วไฟ D25/D26 กลับไปกลับมาเร็วๆ (ไฟสลับ) เพื่อไม่ให้ขั้วโลหะเกิดประจุกั้นไฟ
 *   วัดแรงดันที่จุดต่อระหว่างตัวต้านทานกับหัววัด (D33) ทั้งสองจังหวะ แล้วคำนวณ
 *     ความต้านทานน้ำ  Rw = 4700 x V1 / V2
 *     ค่าการนำไฟฟ้า  EC (uS/cm) = 1,000,000 / Rw x CELL_K
 *     TDS (ppm)       = EC25 x 0.5   (ใช้ตัวคูณเดียวกับ DFRobot)
 *
 * การใช้งาน
 *   กดปุ่มเขียว (สั้น)       = วัดค่า
 *   กดปุ่มเขียวค้าง 3 วินาที = คาลิเบรต: จุ่มหัววัดในน้ำที่รู้ค่า REF_PPM ก่อน
 *
 * ต่อสาย (บอร์ดขยาย ESP32) — ไม่ใช้บอร์ด TDS
 *   S ของ D25 -> ตัวต้านทาน 4.7k -> จุด X
 *   จุด X     -> S ของ D33  และ  -> หัววัด ขั้วที่ 1
 *   หัววัด ขั้วที่ 2 -> S ของ D26
 *   (จุด X ห้ามมีตัวเก็บประจุ)
 *   ปุ่มเขียว: ขั้ว 23 -> S ของ D15, ขั้ว 24 -> G
 *   LCD: SDA -> D21, SCL -> D22, VCC -> VIN, GND -> G
 * ไลบรารี: "LiquidCrystal I2C" (by Frank de Brabander)
 */

#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Preferences.h>

const int   PIN_EXC_A  = 25;     // ขาสร้างไฟ ฝั่งตัวต้านทาน
const int   PIN_EXC_B  = 26;     // ขาสร้างไฟ ฝั่งหัววัดขั้วที่ 2
const int   PIN_SENSE  = 33;     // อ่านแรงดันที่จุด X
const int   PIN_BUTTON = 15;
const float R_SERIES   = 4700.0; // ตัวต้านทานที่ต่ออนุกรม (โอห์ม)
const float TEMPERATURE = 25.0;  // อุณหภูมิน้ำ (°C)
const float REF_PPM    = 500.0;  // ค่า TDS ของน้ำที่ใช้คาลิเบรต (เช่น น้ำเกลือ 500 mg/L ที่ผสมเอง)
const int   CYCLES     = 200;    // จำนวนรอบสลับขั้วต่อการวัด 1 ครั้ง

LiquidCrystal_I2C *lcd = nullptr;
Preferences prefs;
float cellK = 1.0;   // ค่าคงที่ของหัววัด (ได้จากการคาลิเบรต)

void lcdLine(int row, String s) {
  if (!lcd) return;
  while (s.length() < 20) s += ' ';
  lcd->setCursor(0, row);
  lcd->print(s.substring(0, 20));
}

// วัดความต้านทานของน้ำระหว่างขั้วหัววัด (โอห์ม) คืนค่าติดลบถ้าหัววัดไม่ได้อยู่ในน้ำ
float measureWaterOhms() {
  long sum1 = 0, sum2 = 0;
  for (int i = 0; i < CYCLES; i++) {
    // จังหวะ 1: ไฟไหล D25 -> 4.7k -> น้ำ -> D26
    digitalWrite(PIN_EXC_B, LOW);
    digitalWrite(PIN_EXC_A, HIGH);
    delayMicroseconds(20);
    sum1 += analogReadMilliVolts(PIN_SENSE);
    // จังหวะ 2: สลับขั้ว ไฟไหล D26 -> น้ำ -> 4.7k -> D25
    digitalWrite(PIN_EXC_A, LOW);
    digitalWrite(PIN_EXC_B, HIGH);
    delayMicroseconds(20);
    sum2 += analogReadMilliVolts(PIN_SENSE);
  }
  digitalWrite(PIN_EXC_A, LOW);   // วัดเสร็จ ปิดไฟที่หัววัด
  digitalWrite(PIN_EXC_B, LOW);

  float v1 = sum1 / (float)CYCLES;   // แรงดันคร่อมน้ำ
  float v2 = sum2 / (float)CYCLES;   // แรงดันคร่อมตัวต้านทาน
  Serial.printf("V1=%.0f mV  V2=%.0f mV\n", v1, v2);
  if (v2 < 15) return -1;            // แทบไม่มีไฟไหลผ่านน้ำ = ไม่ได้จุ่มน้ำ
  return R_SERIES * v1 / v2;
}

// แปลงความต้านทานน้ำเป็น TDS (ppm) โดยยังไม่คูณค่าคงที่หัววัด
float rawTds(float ohms) {
  float ec = 1000000.0 / ohms;                          // uS (ยังไม่คูณ K)
  float ec25 = ec / (1.0 + 0.02 * (TEMPERATURE - 25.0)); // ชดเชยอุณหภูมิ
  return ec25 * 0.5;
}

void showResult() {
  lcdLine(2, "Measuring...");
  float ohms = measureWaterOhms();
  if (ohms < 0) {
    lcdLine(1, "TDS : 0 ppm");
    lcdLine(2, "Probe not in water");
    lcdLine(3, "GREEN = measure");
    Serial.println("TDS: 0 ppm (probe not in water)");
    return;
  }
  float tds = rawTds(ohms) * cellK;
  lcdLine(1, "TDS : " + String(tds, 0) + " ppm");
  lcdLine(2, "R   : " + String(ohms, 0) + " ohm");
  lcdLine(3, "GREEN = measure");
  Serial.printf("TDS: %.0f ppm   R: %.0f ohm   K=%.3f\n", tds, ohms, cellK);
}

void calibrate() {
  lcdLine(1, "CALIBRATING...");
  lcdLine(2, "Ref: " + String(REF_PPM, 0) + " ppm");
  lcdLine(3, "");
  float ohms = measureWaterOhms();
  if (ohms < 0) {
    lcdLine(1, "CAL FAILED");
    lcdLine(2, "Probe not in water");
    return;
  }
  cellK = REF_PPM / rawTds(ohms);
  prefs.putFloat("cellK", cellK);
  lcdLine(1, "CAL OK  K=" + String(cellK, 3));
  lcdLine(2, "Saved");
  Serial.printf("คาลิเบรตแล้ว K = %.3f\n", cellK);
}

void setup() {
  Serial.begin(115200);
  pinMode(PIN_EXC_A, OUTPUT);
  pinMode(PIN_EXC_B, OUTPUT);
  digitalWrite(PIN_EXC_A, LOW);
  digitalWrite(PIN_EXC_B, LOW);
  pinMode(PIN_BUTTON, INPUT_PULLUP);

  prefs.begin("tdsac", false);
  cellK = prefs.getFloat("cellK", 1.0);

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
  lcdLine(0, "TDS METER (AC)");
  lcdLine(2, "Press GREEN button");
  lcdLine(3, "to measure");
}

void loop() {
  if (digitalRead(PIN_BUTTON) == LOW) {
    unsigned long pressedAt = millis();
    while (digitalRead(PIN_BUTTON) == LOW && millis() - pressedAt < 3000) delay(10);
    if (millis() - pressedAt >= 3000) {
      calibrate();                                   // กดค้าง 3 วินาที
      while (digitalRead(PIN_BUTTON) == LOW) delay(10);
    } else {
      showResult();                                  // กดสั้น
    }
    delay(50);
  }
}
