/*
 * วัด TDS เมื่อกดปุ่มสีเขียว แล้วแสดงค่าบนจอ LCD
 *
 * กดปุ่มสีเขียว -> วัด 1 วินาที (อ่าน 30 ครั้ง ใช้ค่ากลาง) -> แสดงค่า TDS บนจอค้างไว้
 * กดอีกครั้ง = วัดใหม่
 *
 * ต่อสาย
 *   TDS A  -> 4.7k -> ช่อง S ของ D34  (ถ้าเสียบ D33 ให้แก้ PIN_TDS เป็น 33)
 *   TDS +  -> 3V3     TDS - -> G
 *   ปุ่มเขียว ขั้ว 23 -> ช่อง S ของ D15, ขั้ว 24 -> G
 *   LCD SDA -> D21, SCL -> D22, VCC -> VIN, GND -> G
 * ไลบรารี: "LiquidCrystal I2C" (by Frank de Brabander)
 */

#include <Wire.h>
#include <LiquidCrystal_I2C.h>

const int   PIN_TDS     = 34;     // ขาที่ต่อสาย A ของบอร์ด TDS
const int   PIN_BUTTON  = 15;     // ปุ่มสีเขียว
const float TEMPERATURE = 25.0;   // อุณหภูมิน้ำ (°C) สำหรับชดเชยค่า

LiquidCrystal_I2C *lcd = nullptr;

void lcdLine(int row, String s) {
  if (!lcd) return;
  while (s.length() < 20) s += ' ';
  lcd->setCursor(0, row);
  lcd->print(s.substring(0, 20));
}

// อ่านแรงดัน 30 ครั้งใน 1 วินาที แล้วใช้ค่ากลาง (mV)
// แรงดันต่ำใช้ช่วงวัดละเอียดของ ESP32 จะอ่านน้ำที่สารละลายน้อยได้ดีขึ้น
int measureMilliVolts() {
  const int N = 30;
  int buf[N];
  analogSetPinAttenuation(PIN_TDS, ADC_11db);
  bool fine = analogReadMilliVolts(PIN_TDS) < 800;
  if (fine) analogSetPinAttenuation(PIN_TDS, ADC_0db);

  for (int i = 0; i < N; i++) {
    buf[i] = analogReadMilliVolts(PIN_TDS);
    delay(33);
  }
  for (int i = 1; i < N; i++) {            // เรียงค่าจากน้อยไปมาก
    int v = buf[i], j = i - 1;
    while (j >= 0 && buf[j] > v) { buf[j + 1] = buf[j]; j--; }
    buf[j + 1] = v;
  }
  return (buf[N / 2 - 1] + buf[N / 2]) / 2;  // ค่ากลาง
}

// แปลงแรงดันเป็น TDS (สูตรของ DFRobot)
float voltsToTds(float v) {
  v = v / (1.0 + 0.02 * (TEMPERATURE - 25.0));   // ชดเชยอุณหภูมิ
  float tds = (133.42 * v * v * v - 255.86 * v * v + 857.39 * v) * 0.5;
  return tds < 0 ? 0 : tds;
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
  lcdLine(0, "TDS METER");
  lcdLine(2, "Press GREEN button");
  lcdLine(3, "to measure");
}

void loop() {
  static bool lastButton = HIGH;
  bool button = digitalRead(PIN_BUTTON);

  if (lastButton == HIGH && button == LOW) {   // เพิ่งกดปุ่ม
    lcdLine(1, "");
    lcdLine(2, "Measuring...");
    lcdLine(3, "");

    int mv = measureMilliVolts();
    float tds = voltsToTds(mv / 1000.0);

    lcdLine(1, "TDS : " + String(tds, 0) + " ppm");
    lcdLine(2, "V   : " + String(mv / 1000.0, 3) + " V");
    lcdLine(3, "GREEN = measure new");

    Serial.print("TDS: ");
    Serial.print(tds, 0);
    Serial.print(" ppm   V: ");
    Serial.println(mv / 1000.0, 3);
  }
  lastButton = button;
  delay(30);   // กันปุ่มเด้ง
}
