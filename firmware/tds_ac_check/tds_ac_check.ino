/*
 * ตรวจวงจร TDS แบบ ESP32 สร้างไฟสลับ (ต่อสายเหมือน tds_ac) — ค่าวิ่งทุกครึ่งวินาที ไม่ต้องกดปุ่ม
 *
 * จอแสดง
 *   V1 : แรงดันที่จุด X จังหวะ 1 (คร่อมน้ำ)          หน่วย mV
 *   V2 : แรงดันที่จุด X จังหวะ 2 (คร่อมตัวต้านทาน)   หน่วย mV
 *   R  : ความต้านทานน้ำ = 4700 x V1 / V2
 *   idle : แรงดันที่จุด X ตอนปิดไฟทั้งสองขา (ต้องใกล้ 0)
 *
 * ผลที่ควรเห็น
 *   หัววัดอยู่ในอากาศ          : V1 สูง (ประมาณ 3000), V2 ประมาณ 0  -> R: open
 *   ตัวต้านทาน 4.7k แทนหัววัด  : V1 กับ V2 ใกล้กัน (ประมาณ 1600)    -> R ประมาณ 4700
 *   น้ำประปา                   : V1 ลดลง V2 เพิ่มขึ้น               -> R หลักพัน
 *   น้ำเกลือ                   : V1 ต่ำ V2 สูง                      -> R หลักร้อย
 *
 * ต่อสาย: S ของ D25 -> 4.7k -> จุด X ; จุด X -> S ของ D33 และ หัววัดขั้ว 1 ; หัววัดขั้ว 2 -> S ของ D26
 *         LCD: SDA -> D21, SCL -> D22, VCC -> VIN, GND -> G
 * ไลบรารี: "LiquidCrystal I2C" (by Frank de Brabander)
 */

#include <Wire.h>
#include <LiquidCrystal_I2C.h>

const int   PIN_EXC_A = 25;
const int   PIN_EXC_B = 26;
const int   PIN_SENSE = 33;
const float R_SERIES  = 4700.0;
const int   CYCLES    = 200;

LiquidCrystal_I2C *lcd = nullptr;

void lcdLine(int row, String s) {
  if (!lcd) return;
  while (s.length() < 20) s += ' ';
  lcd->setCursor(0, row);
  lcd->print(s.substring(0, 20));
}

void setup() {
  Serial.begin(115200);
  pinMode(PIN_EXC_A, OUTPUT);
  pinMode(PIN_EXC_B, OUTPUT);
  digitalWrite(PIN_EXC_A, LOW);
  digitalWrite(PIN_EXC_B, LOW);

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
  lcdLine(0, "TDS AC CHECK");
  Serial.println("idle_mV,V1_mV,V2_mV,R_ohm");
}

void loop() {
  int idle = analogReadMilliVolts(PIN_SENSE);      // ปิดไฟทั้งสองขา

  long sum1 = 0, sum2 = 0;
  for (int i = 0; i < CYCLES; i++) {
    digitalWrite(PIN_EXC_B, LOW);                  // จังหวะ 1: D25 -> 4.7k -> น้ำ -> D26
    digitalWrite(PIN_EXC_A, HIGH);
    delayMicroseconds(20);
    sum1 += analogReadMilliVolts(PIN_SENSE);
    digitalWrite(PIN_EXC_A, LOW);                  // จังหวะ 2: สลับขั้ว
    digitalWrite(PIN_EXC_B, HIGH);
    delayMicroseconds(20);
    sum2 += analogReadMilliVolts(PIN_SENSE);
  }
  digitalWrite(PIN_EXC_A, LOW);
  digitalWrite(PIN_EXC_B, LOW);

  float v1 = sum1 / (float)CYCLES;
  float v2 = sum2 / (float)CYCLES;

  lcdLine(1, "V1:" + String(v1, 0) + " V2:" + String(v2, 0));
  if (v2 < 15) {
    lcdLine(2, "R : open (no water)");
    Serial.printf("%d,%.0f,%.0f,open\n", idle, v1, v2);
  } else {
    float r = R_SERIES * v1 / v2;
    lcdLine(2, "R : " + String(r, 0) + " ohm");
    Serial.printf("%d,%.0f,%.0f,%.0f\n", idle, v1, v2, r);
  }
  lcdLine(3, "idle: " + String(idle) + " mV");
  delay(500);
}
