/*
 * เครื่องวัด TDS: แสดงค่าน้ำ (ppm) ทุก 1 วินาที
 * วัดได้ตั้งแต่น้ำที่มีสารละลายน้อยมาก (น้ำ RO น้ำกลั่น) จนถึง 1000 ppm
 *
 * - ไม่ตัดค่าเป็น 0 แล้ว: น้ำสารละลายน้อยก็แสดงค่าจริง
 * - น้ำที่ค่าต่ำ (ต่ำกว่า 100 ppm) แสดงทศนิยม 1 ตำแหน่ง ให้เห็นความต่างเล็กๆ
 * - สลับช่วงวัดของ ESP32 อัตโนมัติ: แรงดันต่ำใช้ช่วงละเอียด (0-950 mV)
 *   แรงดันสูงใช้ช่วงกว้าง (0-2500 mV)
 *
 * ต่อสาย: TDS A -> 4.7k -> D34 (+10µF, 104 ลง GND) · LCD SDA D21, SCL D22
 * ไลบรารี: "LiquidCrystal I2C" (by Frank de Brabander)
 */

#include <Wire.h>
#include <LiquidCrystal_I2C.h>

const int   PIN_TDS  = 34;
const int   PIN_SDA  = 21;
const int   PIN_SCL  = 22;
const float TDS_MAX  = 1000.0;   // ค่าสูงสุดที่เซนเซอร์วัดได้
const float FINE_MAX_MV = 800.0; // ต่ำกว่านี้ใช้ช่วงวัดละเอียด

LiquidCrystal_I2C *lcd = nullptr;

// อ่าน 31 ครั้ง ตัดค่าที่กระโดดหัวท้ายทิ้ง แล้วเฉลี่ยตรงกลาง 15 ค่า
// การเฉลี่ยทำให้เห็นความต่างที่เล็กกว่า 1 mV (สำคัญมากกับน้ำสารละลายน้อย)
float readSmoothMv() {
  const int N = 31;
  int buf[N];
  for (int i = 0; i < N; i++) {
    buf[i] = analogReadMilliVolts(PIN_TDS);
    delayMicroseconds(500);
  }
  for (int i = 1; i < N; i++) {
    int v = buf[i], j = i - 1;
    while (j >= 0 && buf[j] > v) { buf[j + 1] = buf[j]; j--; }
    buf[j + 1] = v;
  }
  float sum = 0;
  for (int i = 8; i < 23; i++) sum += buf[i];
  return sum / 15.0;
}

// เลือกช่วงวัดอัตโนมัติ แล้วคืนแรงดันที่ขา D34 (mV)
float readMilliVolts() {
  analogSetPinAttenuation(PIN_TDS, ADC_11db);   // ช่วงกว้าง ดูก่อนว่าแรงดันประมาณเท่าไร
  float mv = readSmoothMv();
  if (mv < FINE_MAX_MV) {
    analogSetPinAttenuation(PIN_TDS, ADC_0db);  // ช่วงละเอียด สำหรับน้ำสารละลายน้อย
    mv = readSmoothMv();
  }
  return mv;
}

// แปลงแรงดันเป็นค่าน้ำ (ppm) ตามสูตรของโมดูล TDS
float readTds() {
  float v = readMilliVolts() / 1000.0;
  float tds = (133.42 * v * v * v - 255.86 * v * v + 857.39 * v) * 0.5;
  return tds < 0 ? 0 : tds;
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
  float tds = readTds();
  String text = tds < 100 ? String(tds, 1) : String(tds, 0);   // ค่าต่ำแสดงทศนิยม

  lcdLine(2, "TDS : " + text + " ppm");
  lcdLine(3, tds >= TDS_MAX ? "Over sensor range!" : "");
  Serial.println(text);

  delay(1000);
}
