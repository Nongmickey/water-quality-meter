/*
 * สแกนทุกช่องอนาล็อก: หาว่าสาย A ของบอร์ด TDS เข้าช่องไหน และ ESP32 อ่านค่าได้มั้ย
 * แสดงแรงดัน (mV) ของช่องกลุ่ม ADC1 ทั้งหมดทุกครึ่งวินาที: D32 D33 D34 D35 VP(36) VN(39)
 *
 * วิธีใช้
 *   1) จุ่มหัววัด TDS ในน้ำเกลือ แล้วยกขึ้น สลับไปมา
 *      ช่องที่ค่าขึ้นลงตาม = ช่องที่สาย A ต่ออยู่จริง (ต้องตรงกับขาในโค้ดที่ใช้วัด)
 *   2) ถ้าไม่มีช่องไหนขึ้นเลย: เอามิเตอร์ (DC 20V) วัดขา A กับขา - ของบอร์ด TDS ตอนจุ่มน้ำเกลือ
 *      ได้ 0.5-2.3 V = บอร์ดดี ปัญหาอยู่ที่สาย/ช่องเสียบ ; ได้ 0 V = ปัญหาที่บอร์ดหรือหัววัด
 *   3) ทดสอบขา ESP32: เอาสายจัมเปอร์จาก 3V3 แตะ S ของช่องนั้น ต้องได้ประมาณ 3000 mV ขึ้นไป
 *      แตะ G ต้องได้ประมาณ 0-150 mV
 * ช่องที่ไม่ได้ต่ออะไร ค่าจะลอยเป็นตัวเลขสุ่ม เป็นเรื่องปกติ
 * ช่อง D32 (อุณหภูมิ) และ D35 (pH) ถ้าต่อเซนเซอร์อยู่ จะเห็นค่าของเซนเซอร์นั้น
 * ไลบรารี: "LiquidCrystal I2C" (by Frank de Brabander)
 */

#include <Wire.h>
#include <LiquidCrystal_I2C.h>

const int   PINS[6]  = {32, 33, 34, 35, 36, 39};
const char *NAMES[6] = {"32", "33", "34", "35", "VP", "VN"};

LiquidCrystal_I2C *lcd = nullptr;

void lcdLine(int row, String s) {
  if (!lcd) return;
  while (s.length() < 20) s += ' ';
  lcd->setCursor(0, row);
  lcd->print(s.substring(0, 20));
}

// อ่าน 16 ครั้ง เฉลี่ย (mV)
int readMv(int pin) {
  long sum = 0;
  for (int i = 0; i < 16; i++) sum += analogReadMilliVolts(pin);
  return sum / 16;
}

String pad4(int v) {
  String s = String(v);
  while (s.length() < 4) s = " " + s;
  return s;
}

void setup() {
  Serial.begin(115200);
  pinMode(25, INPUT);                              // ขาวงจร tds_ac เก่า (ถ้ายังต่ออยู่) ปล่อยลอย
  pinMode(26, INPUT);

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
  lcdLine(3, "Dip TDS in salt (mV)");
}

void loop() {
  int v[6];
  for (int i = 0; i < 6; i++) v[i] = readMv(PINS[i]);

  for (int row = 0; row < 3; row++) {
    int a = row * 2, b = row * 2 + 1;
    lcdLine(row, String(NAMES[a]) + ":" + pad4(v[a]) + "  " + NAMES[b] + ":" + pad4(v[b]));
  }
  for (int i = 0; i < 6; i++) Serial.printf("%s=%d  ", NAMES[i], v[i]);
  Serial.println();
  delay(500);
}
