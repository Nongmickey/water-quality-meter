/*
 * แสดงข้อความบนจอ LCD 20x4 (I2C)
 * แก้ข้อความได้ที่ LINE1 - LINE4 (บรรทัดละไม่เกิน 20 ตัว ใช้ได้เฉพาะภาษาอังกฤษ ตัวเลข และสัญลักษณ์)
 *
 * ต่อสาย
 *   LCD SDA -> S ของ D21
 *   LCD SCL -> S ของ D22
 *   LCD VCC -> VIN (5V)   ต้องเป็น 5V ถ้าใช้ 3V3 ตัวหนังสือจะจางจนมองไม่เห็น
 *   LCD GND -> G
 *
 * ถ้าไฟจอติดแต่ไม่เห็นตัวหนังสือ: หมุนตัวปรับสีฟ้าด้านหลังจอช้าๆ จนตัวหนังสือชัด
 * ไลบรารี: "LiquidCrystal I2C" (by Frank de Brabander)
 */

#include <Wire.h>
#include <LiquidCrystal_I2C.h>

const char *LINE1 = "Smart Water System";
const char *LINE2 = "Water Quality Meter";
const char *LINE3 = "pH / TDS / Temp";
const char *LINE4 = "Hello!";

LiquidCrystal_I2C *lcd = nullptr;

void setup() {
  Serial.begin(115200);
  Wire.begin(21, 22);

  // หาจอ: address ที่จอ LCD ใช้บ่อย
  const uint8_t addrs[] = {0x27, 0x3F, 0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26,
                           0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E};
  for (uint8_t a : addrs) {
    Wire.beginTransmission(a);
    if (Wire.endTransmission() == 0) {
      lcd = new LiquidCrystal_I2C(a, 20, 4);
      Serial.printf("เจอจอที่ 0x%02X\n", a);
      break;
    }
  }
  if (!lcd) {
    Serial.println("ไม่เจอจอ: เช็ก SDA -> D21, SCL -> D22, VCC -> VIN, GND -> G");
    return;
  }

  lcd->init();
  lcd->backlight();
  lcd->setCursor(0, 0); lcd->print(LINE1);
  lcd->setCursor(0, 1); lcd->print(LINE2);
  lcd->setCursor(0, 2); lcd->print(LINE3);
  lcd->setCursor(0, 3); lcd->print(LINE4);
  Serial.println("แสดงข้อความแล้ว ถ้าจอไม่เห็นตัวหนังสือ ให้หมุนตัวปรับสีฟ้าด้านหลังจอ");
}

void loop() {
}
