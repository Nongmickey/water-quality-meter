/*
 * ดูค่า TDS แบบสด: อ่านค่าน้ำทุก 1 วินาที แสดงบนจอ LCD
 *
 * จุ่มหัววัดในน้ำ = แสดงค่าน้ำ (ppm)
 * ยังไม่ได้จุ่มน้ำ = แสดง 0 ppm
 * Serial Monitor (115200) พิมพ์ค่าทุกวินาทีด้วย
 *
 * ต่อสาย: TDS A -> 4.7k -> D34 (+10µF, 104 ลง GND) · LCD SDA D21, SCL D22
 * ไลบรารี: "LiquidCrystal I2C" (by Frank de Brabander)
 */

#include <Wire.h>
#include <LiquidCrystal_I2C.h>

const int   PIN_TDS    = 34;
const int   PIN_SDA    = 21;
const int   PIN_SCL    = 22;
const float TDS_MIN_MV = 10.0;   // ต่ำกว่านี้ = หัววัดไม่ได้จุ่มน้ำ -> 0 ppm

LiquidCrystal_I2C *lcd = nullptr;

// อ่านแรงดันจากเซนเซอร์ (ใช้ค่ากลางจาก 25 ครั้ง กันค่ากระโดด)
float readMilliVolts() {
  const int N = 25;
  int buf[N];
  for (int i = 0; i < N; i++) {
    buf[i] = analogReadMilliVolts(PIN_TDS);
    delayMicroseconds(800);
  }
  for (int i = 1; i < N; i++) {
    int v = buf[i], j = i - 1;
    while (j >= 0 && buf[j] > v) { buf[j + 1] = buf[j]; j--; }
    buf[j + 1] = v;
  }
  return buf[N / 2];
}

// แปลงแรงดันเป็นค่าน้ำ (ppm) ตามสูตรของโมดูล TDS
int readTds() {
  float mv = readMilliVolts();
  if (mv < TDS_MIN_MV) return 0;
  float v = mv / 1000.0;
  float tds = (133.42 * v * v * v - 255.86 * v * v + 857.39 * v) * 0.5;
  return tds < 0 ? 0 : (int)(tds + 0.5);
}

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
  lcdLine(0, "TDS WATER METER");
}

void loop() {
  int tds = readTds();
  lcdLine(2, "TDS : " + String(tds) + " ppm");
  Serial.println(tds);
  delay(1000);
}
