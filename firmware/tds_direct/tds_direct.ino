/*
 * ทดสอบบอร์ด TDS ต่อตรงเข้า ESP32 (ไม่มีตัวต้านทาน / ตัวเก็บประจุกรองสัญญาณ)
 * ค่าวิ่งทุก 1 วินาที (อ่านค่าดิบครั้งเดียว ไม่กรอง) + กดปุ่มเขียว = เฉลี่ย 10 วินาที
 *
 * ต่อสาย
 *   บอร์ด TDS  + -> 3V3    - -> G    A -> S ของ D33 (ต่อตรง)
 *   หัววัดเสียบช่องขาว 2 ขาของบอร์ด TDS
 *   ปุ่มเขียว: ขั้ว 23 -> S ของ D15, ขั้ว 24 -> G
 *   LCD: SDA -> D21, SCL -> D22, VCC -> VIN, GND -> G
 * บอร์ด TDS ใช้ไฟ 3V3 ขา A ออกสูงสุดประมาณ 2.3 V ต่อตรงเข้า ESP32 ได้ปลอดภัย
 *
 * ค่าที่ควรเห็น
 *   หัววัดอยู่ในอากาศ : <60 ppm (A ประมาณ 142 mV = ไฟจริง 0 V ESP32 อ่านต่ำกว่านี้ไม่ได้)
 *   น้ำประปา          : ประมาณ 100-300 ppm
 *   น้ำเกลือ          : สูงกว่าน้ำประปามาก
 * ไม่มีตัวกรอง ค่าจะแกว่งขึ้นลงเล็กน้อยเป็นปกติ
 * ไลบรารี: "LiquidCrystal I2C" (by Frank de Brabander)
 */

#include <Wire.h>
#include <LiquidCrystal_I2C.h>

const int   PIN_TDS     = 33;     // ขา A ของบอร์ด TDS (ช่องอื่นที่ใช้ได้: 34, 36 (VP), 39 (VN))
const int   PIN_BUTTON  = 15;
const float TEMPERATURE = 25.0;   // อุณหภูมิน้ำ (°C) สำหรับชดเชยค่า
const float TDS_K       = 1.0;    // ค่าปรับเทียบ: ถ้ารู้ค่าจริง K = ค่าจริง / ค่าที่วัดได้
const float FLOOR_MV    = 150.0;  // ต่ำกว่านี้ = ไฟจริง 0 V

LiquidCrystal_I2C *lcd = nullptr;
String avgText = "GREEN = avg 10s";

void lcdLine(int row, String s) {
  if (!lcd) return;
  while (s.length() < 20) s += ' ';
  lcd->setCursor(0, row);
  lcd->print(s.substring(0, 20));
}

// แปลงแรงดันเป็น TDS (สูตร DFRobot)
float tdsFromMv(float mv) {
  if (mv < FLOOR_MV) return 0;
  float v = (mv / 1000.0) / (1.0 + 0.02 * (TEMPERATURE - 25.0));
  float tds = (133.42 * v * v * v - 255.86 * v * v + 857.39 * v) * 0.5 * TDS_K;
  return tds < 0 ? 0 : tds;
}

String tdsText(float tds) {
  return tds < 60 ? String("<60 ppm") : String(tds, 0) + " ppm";
}

void showLive() {
  int mv = analogReadMilliVolts(PIN_TDS);          // อ่านค่าดิบ 1 ครั้ง ไม่กรอง
  float tds = tdsFromMv(mv);
  lcdLine(1, "TDS : " + tdsText(tds));
  lcdLine(2, "A   : " + String(mv) + " mV");
  Serial.printf("A: %d mV   TDS: %.0f ppm\n", mv, tds);
}

void measureAvg() {
  const int N = 10;
  long sum = 0;
  for (int i = 0; i < N; i++) {
    lcdLine(3, "Measuring... " + String(N - i) + "s");
    sum += analogReadMilliVolts(PIN_TDS);
    delay(1000);
  }
  float mv = sum / (float)N;
  float tds = tdsFromMv(mv);
  avgText = "AVG10s: " + tdsText(tds);
  lcdLine(3, avgText);
  Serial.printf("เฉลี่ย 10 วินาที: A %.0f mV  TDS %.0f ppm\n", mv, tds);
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
  lcdLine(0, "TDS DIRECT (D" + String(PIN_TDS) + ")");
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
