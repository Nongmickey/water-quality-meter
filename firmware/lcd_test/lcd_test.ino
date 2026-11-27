/*
 * ทดสอบจอ LCD 20x4 (I2C)
 *
 * ต่อสาย
 *   LCD SDA -> S ของ D21
 *   LCD SCL -> S ของ D22
 *   LCD VCC -> VIN (5V)
 *   LCD GND -> G
 *
 * การทำงาน
 *   1) ค้นหาอุปกรณ์ I2C ทุกตำแหน่ง แล้วบอกใน Serial Monitor (115200) ว่าเจอที่ address ไหน
 *   2) กะพริบไฟพื้นหลัง 2 ครั้ง
 *   3) แสดงข้อความครบ 4 บรรทัด + ตัวนับวินาที
 *   4) ทุก 6 วินาที เติมช่องเต็มทั้งจอ 1 วินาที เช็กว่าทุกช่องติดครบ (ไม่มีช่องดับ)
 *
 * ถ้าไฟจอติดแต่ไม่เห็นตัวหนังสือ หรือเห็นเป็นแถบสี่เหลี่ยม: หมุนตัวปรับสีฟ้าด้านหลังจอ (ปรับความชัด)
 * ไลบรารี: "LiquidCrystal I2C" (by Frank de Brabander)
 */

#include <Wire.h>
#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C *lcd = nullptr;
uint8_t lcdAddr = 0;
unsigned long counter = 0;

void lcdLine(int row, String s) {
  if (!lcd) return;
  while (s.length() < 20) s += ' ';
  lcd->setCursor(0, row);
  lcd->print(s.substring(0, 20));
}

String hex2(uint8_t v) {
  const char *d = "0123456789ABCDEF";
  return String("0x") + d[v >> 4] + d[v & 0x0F];
}

void showText() {
  lcdLine(0, "LCD TEST  " + hex2(lcdAddr));
  lcdLine(1, "Line2 ABCDEFGHIJKLMN");
  lcdLine(2, "Line3 01234567890123");
  lcdLine(3, "Count: " + String(counter) + " s");
}

void fillBlocks() {
  String full = "";
  for (int i = 0; i < 20; i++) full += (char)255;   // ช่องเต็ม (ทุกจุดติด)
  for (int row = 0; row < 4; row++) {
    lcd->setCursor(0, row);
    lcd->print(full);
  }
}

void setup() {
  Serial.begin(115200);
  delay(500);
  Wire.begin(21, 22);

  Serial.println("ค้นหาอุปกรณ์ I2C ...");
  int found = 0;
  for (uint8_t a = 1; a < 127; a++) {
    Wire.beginTransmission(a);
    if (Wire.endTransmission() == 0) {
      Serial.println("  เจออุปกรณ์ที่ " + hex2(a));
      found++;
      // จอ LCD ใช้ address 0x20-0x27 หรือ 0x38-0x3F
      if (lcdAddr == 0 && ((a >= 0x20 && a <= 0x27) || (a >= 0x38 && a <= 0x3F))) lcdAddr = a;
    }
  }

  if (lcdAddr == 0) {
    Serial.println(found == 0 ? "ไม่เจออุปกรณ์ I2C เลย" : "เจออุปกรณ์ แต่ไม่ใช่ address ของจอ LCD");
    Serial.println("เช็ก: SDA -> D21, SCL -> D22 (ใช้ช่อง S), VCC -> VIN, GND -> G, สายสลับ SDA/SCL หรือเปล่า");
    return;
  }

  Serial.println("ใช้จอที่ " + hex2(lcdAddr));
  lcd = new LiquidCrystal_I2C(lcdAddr, 20, 4);
  lcd->init();
  for (int i = 0; i < 2; i++) {                    // กะพริบไฟพื้นหลัง
    lcd->noBacklight();
    delay(300);
    lcd->backlight();
    delay(300);
  }
  showText();
  Serial.println("ถ้าจอไม่เห็นตัวหนังสือ ให้หมุนตัวปรับสีฟ้าด้านหลังจอ");
}

void loop() {
  if (!lcd) {
    delay(1000);
    return;
  }
  delay(1000);
  counter++;
  if (counter % 6 == 0) {
    fillBlocks();                                  // เติมช่องเต็มทั้งจอ 1 วินาที
    delay(1000);
    counter++;
  }
  showText();
}
