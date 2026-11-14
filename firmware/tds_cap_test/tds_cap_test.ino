/*
 * ทดสอบ 1: เช็กว่าวงจรไฟสลับ (AC) ในบอร์ด TDS ทำงานหรือไม่
 *
 * วิธีทดสอบ
 *   1) ถอดหัววัดออกจากช่องขาวบนบอร์ด TDS
 *   2) เปิดโค้ดนี้ ค่าบนจอควรใกล้ 0
 *   3) เอาตัวเก็บประจุ 10µF แตะคร่อมขาสองขาในช่องขาว ค้างไว้ 5 วินาที
 *      - ค่า "n" (ตอนนี้) ขึ้นสูงและค้างอยู่ตลอดที่แตะ   = วงจร AC ทำงาน (บอร์ดปกติ)
 *      - ค่า "n" กระโดดขึ้นแป๊บเดียวแล้วตกกลับเป็น 0     = วงจร AC เสีย (บอร์ดมีแต่ไฟตรง)
 *        (ค่า "m" = ค่าสูงสุดที่เคยเห็น ช่วยให้เห็นการกระโดดแป๊บเดียว)
 *   4) ลองเทียบกับตัวต้านทาน 4.7k: ค่าต้องขึ้นและค้างอยู่ทั้งสองกรณี
 *   กดปุ่มสีเขียว = ล้างค่าสูงสุด (m) เริ่มทดสอบใหม่
 *
 * อ่านทั้งขา D34 และ D33 พร้อมกัน ใช้ได้ไม่ว่าสาย A จะเสียบช่องไหน
 * ไลบรารี: "LiquidCrystal I2C" (by Frank de Brabander)
 */

#include <Wire.h>
#include <LiquidCrystal_I2C.h>

const int PIN_A = 34;
const int PIN_B = 33;
const int PIN_BUTTON = 15;

LiquidCrystal_I2C *lcd = nullptr;
int maxA = 0, maxB = 0;

void lcdLine(int row, String s) {
  if (!lcd) return;
  while (s.length() < 20) s += ' ';
  lcd->setCursor(0, row);
  lcd->print(s.substring(0, 20));
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
  lcdLine(0, "CAP TEST  btn=reset");
  Serial.println("D34_now,D34_max,D33_now,D33_max");
}

void loop() {
  if (digitalRead(PIN_BUTTON) == LOW) {   // กดปุ่ม = ล้างค่าสูงสุด
    maxA = 0;
    maxB = 0;
  }

  int a = analogReadMilliVolts(PIN_A);
  int b = analogReadMilliVolts(PIN_B);
  if (a > maxA) maxA = a;
  if (b > maxB) maxB = b;

  lcdLine(1, "D34 n:" + String(a) + " m:" + String(maxA));
  lcdLine(2, "D33 n:" + String(b) + " m:" + String(maxB));
  lcdLine(3, "n=now  m=max (mV)");

  Serial.print(a); Serial.print(",");
  Serial.print(maxA); Serial.print(",");
  Serial.print(b); Serial.print(",");
  Serial.println(maxB);

  delay(200);   // อัปเดต 5 ครั้งต่อวินาที เห็นการกระโดดแป๊บเดียวได้
}
