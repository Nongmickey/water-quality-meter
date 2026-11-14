/*
 * ดูค่าดิบจากเซนเซอร์ TDS (ไม่คำนวณ ไม่กรอง)
 *
 * อ่านขา D34 ครั้งเดียวทุก 1 วินาที แล้วแสดง 2 ค่า:
 *   ADC = ตัวเลขดิบจากตัวแปลงสัญญาณของ ESP32 (0-4095)
 *   mV  = แรงดันที่ขา D34 หน่วยมิลลิโวลต์ (ESP32 แปลงจากค่า ADC ให้)
 * น้ำที่มีสารละลายมาก ค่าจะสูง · ไม่ได้จุ่มน้ำ ค่าจะใกล้ 0
 *
 * ต่อสาย: TDS A -> 4.7k -> D34 (+10µF, 104 ลง GND) · LCD SDA D21, SCL D22
 * ไลบรารี: "LiquidCrystal I2C" (by Frank de Brabander)
 */

#include <Wire.h>
#include <LiquidCrystal_I2C.h>

const int PIN_TDS = 34;
const int PIN_SDA = 21;
const int PIN_SCL = 22;

LiquidCrystal_I2C *lcd = nullptr;

void lcdLine(int row, String s) {
  if (!lcd) return;
  while (s.length() < 20) s += ' ';
  lcd->setCursor(0, row);
  lcd->print(s.substring(0, 20));
}

void setup() {
  Serial.begin(115200);
  analogReadResolution(12);

  Wire.begin(PIN_SDA, PIN_SCL);
  const uint8_t candidates[] = {0x27, 0x3F, 0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26,
                                0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E};
  for (uint8_t a : candidates) {
    Wire.beginTransmission(a);
    if (Wire.endTransmission() == 0) {
      lcd = new LiquidCrystal_I2C(a, 20, 4);
      lcd->init();
      lcd->backlight();
      break;
    }
  }
  lcdLine(0, "TDS RAW");
  Serial.println("ADC,mV");
}

void loop() {
  int adc = analogRead(PIN_TDS);           // ค่าดิบ 0-4095
  int mv  = analogReadMilliVolts(PIN_TDS); // แรงดันที่ขา (mV)

  lcdLine(2, "ADC : " + String(adc));
  lcdLine(3, "mV  : " + String(mv));
  Serial.print(adc);
  Serial.print(",");
  Serial.println(mv);

  delay(1000);
}
