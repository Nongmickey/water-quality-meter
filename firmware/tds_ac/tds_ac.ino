/*
 * เครื่องวัด TDS แบบไม่ใช้บอร์ด TDS: ให้ ESP32 สร้างไฟสลับเอง (รุ่นเพิ่มความเสถียร)
 *
 * หลักการ
 *   หัววัดต่ออนุกรมกับตัวต้านทาน 4.7k ระหว่างขา D25 กับ D26
 *   ESP32 สลับขั้วไฟกลับไปกลับมาเร็วๆ (ไฟสลับ) เพื่อไม่ให้ขั้วโลหะเกิดประจุกั้นไฟ
 *   วัดแรงดันที่จุด X (D33) ทั้งสองจังหวะ แล้วคำนวณ
 *     ความต้านทานน้ำ  Rw = R_SERIES x V1 / V2
 *     ค่าการนำไฟฟ้า  EC = 1,000,000 / Rw  (ชดเชยอุณหภูมิเป็นค่าที่ 25 °C)
 *     TDS (ppm)       = EC25 x 0.5 x K    (K ได้จากการคาลิเบรต)
 *
 * เพิ่มความเสถียร
 *   - วัด 5 รอบ (รอบละ 500 ครั้งสลับขั้ว) แล้วใช้ค่ากลาง ตัดค่าที่กระโดดทิ้ง
 *   - อ่านอุณหภูมิน้ำจากหัววัด NTC (ถ้าต่อไว้) มาชดเชยอัตโนมัติ ไม่ได้ต่อ = ใช้ 25 °C
 *   - แก้ R_SERIES เป็นค่าที่วัดจริงด้วยมัลติมิเตอร์ จะแม่นขึ้น
 *
 * การใช้งาน
 *   กดปุ่มเขียว (สั้น)       = วัดค่า
 *   กดปุ่มเขียวค้าง 3 วินาที = คาลิเบรต: จุ่มหัววัดในน้ำที่รู้ค่า REF_PPM ก่อน
 *
 * ต่อสาย (บอร์ดขยาย ESP32) — ไม่ใช้บอร์ด TDS
 *   S ของ D25 -> ตัวต้านทาน 4.7k -> จุด X
 *   จุด X     -> S ของ D33  และ  -> หัววัด ขั้วที่ 1      (ห้ามมีตัวเก็บประจุที่จุด X)
 *   หัววัด ขั้วที่ 2 -> S ของ D26
 *   หัววัดอุณหภูมิ NTC (ถ้ามี): สาย 1 -> S ของ D32 และ D32 -> 4.7k -> 3V3, สาย 2 -> G
 *   ปุ่มเขียว: ขั้ว 23 -> S ของ D15, ขั้ว 24 -> G
 *   LCD: SDA -> D21, SCL -> D22, VCC -> VIN, GND -> G
 * ไลบรารี: "LiquidCrystal I2C" (by Frank de Brabander)
 */

#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Preferences.h>
#include <math.h>

// ---------- ตั้งค่า ----------
const int   PIN_EXC_A  = 25;      // ขาสร้างไฟ ฝั่งตัวต้านทาน
const int   PIN_EXC_B  = 26;      // ขาสร้างไฟ ฝั่งหัววัดขั้วที่ 2
const int   PIN_SENSE  = 33;      // อ่านแรงดันที่จุด X
const int   PIN_TEMP   = 32;      // หัววัดอุณหภูมิ NTC (ถ้าไม่ได้ต่อ จะใช้ 25 °C)
const int   PIN_BUTTON = 15;

const float R_SERIES   = 4700.0;  // ตัวต้านทานที่จุด X (โอห์ม) วัดจริงด้วยมัลติมิเตอร์แล้วแก้ตรงนี้
const float REF_PPM    = 500.0;   // ค่า TDS ของน้ำที่ใช้คาลิเบรต
const int   CYCLES     = 500;     // จำนวนครั้งสลับขั้วต่อ 1 รอบวัด
const int   ROUNDS     = 5;       // จำนวนรอบวัด แล้วใช้ค่ากลาง

const float NTC_SERIES = 4700.0;  // ตัวต้านทานที่ต่อกับ NTC
const float NTC_R25    = 10000.0; // NTC 10k ที่ 25 °C
const float NTC_BETA   = 3950.0;  // ค่า B ของ NTC

LiquidCrystal_I2C *lcd = nullptr;
Preferences prefs;
float cellK = 1.0;   // ค่าคงที่ของหัววัด (ได้จากการคาลิเบรต)

void lcdLine(int row, String s) {
  if (!lcd) return;
  while (s.length() < 20) s += ' ';
  lcd->setCursor(0, row);
  lcd->print(s.substring(0, 20));
}

void sortFloats(float *a, int n) {
  for (int i = 1; i < n; i++) {
    float v = a[i];
    int j = i - 1;
    while (j >= 0 && a[j] > v) { a[j + 1] = a[j]; j--; }
    a[j + 1] = v;
  }
}

// ---------- อุณหภูมิน้ำ ----------
// คืน NAN ถ้าไม่ได้ต่อหัววัด NTC หรือค่าผิดปกติ
float readTemperature() {
  float buf[15];
  for (int i = 0; i < 15; i++) {
    buf[i] = analogReadMilliVolts(PIN_TEMP);
    delay(2);
  }
  sortFloats(buf, 15);
  float mv = buf[7];
  if (mv < 50 || mv > 3150) return NAN;            // สายขาด / ไม่ได้ต่อ / ช็อต
  float r = NTC_SERIES * mv / (3300.0 - mv);       // ความต้านทานของ NTC
  float t = 1.0 / (1.0 / 298.15 + log(r / NTC_R25) / NTC_BETA) - 273.15;
  if (t < 0 || t > 60) return NAN;
  return t;
}

// ---------- ความต้านทานของน้ำ ----------
// วัด 1 รอบ: คืนค่าโอห์ม หรือ -1 ถ้าหัววัดไม่ได้อยู่ในน้ำ
float measureOnce() {
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
  digitalWrite(PIN_EXC_A, LOW);                    // วัดเสร็จ ปิดไฟที่หัววัด
  digitalWrite(PIN_EXC_B, LOW);

  float v1 = sum1 / (float)CYCLES;                 // แรงดันคร่อมน้ำ
  float v2 = sum2 / (float)CYCLES;                 // แรงดันคร่อมตัวต้านทาน
  if (v2 < 15) return -1;                          // แทบไม่มีไฟไหลผ่านน้ำ
  return R_SERIES * v1 / v2;
}

// วัด 5 รอบ ใช้ค่ากลาง: คืนโอห์ม หรือ -1 ถ้าวัดได้ไม่ถึง 3 รอบ
float measureWaterOhms() {
  float vals[ROUNDS];
  int n = 0;
  for (int i = 0; i < ROUNDS; i++) {
    float r = measureOnce();
    if (r > 0) vals[n++] = r;
    delay(20);
  }
  if (n < 3) return -1;
  sortFloats(vals, n);
  return vals[n / 2];
}

// TDS ก่อนคูณ K (ชดเชยอุณหภูมิแล้ว)
float rawTds(float ohms, float tempC) {
  float ec = 1000000.0 / ohms;
  float ec25 = ec / (1.0 + 0.02 * (tempC - 25.0));
  return ec25 * 0.5;
}

String tempText(float t) { return isnan(t) ? String("25(set)") : String(t, 1); }

void showResult() {
  lcdLine(1, "");
  lcdLine(2, "Measuring...");
  lcdLine(3, "");
  float t = readTemperature();
  float tUse = isnan(t) ? 25.0 : t;
  float ohms = measureWaterOhms();

  if (ohms < 0) {
    lcdLine(1, "TDS : 0 ppm");
    lcdLine(2, "Probe not in water");
    lcdLine(3, "GREEN = measure");
    Serial.println("TDS: 0 ppm (probe not in water)");
    return;
  }
  float tds = rawTds(ohms, tUse) * cellK;
  lcdLine(1, "TDS : " + String(tds, 0) + " ppm");
  lcdLine(2, "R:" + String(ohms, 0) + " T:" + tempText(t) + "C");
  lcdLine(3, "GREEN = measure");
  Serial.printf("TDS: %.0f ppm   R: %.0f ohm   Temp: %.1f C   K=%.3f\n", tds, ohms, tUse, cellK);
}

void calibrate() {
  lcdLine(1, "CALIBRATING...");
  lcdLine(2, "Ref: " + String(REF_PPM, 0) + " ppm");
  lcdLine(3, "");
  float t = readTemperature();
  float tUse = isnan(t) ? 25.0 : t;
  float ohms = measureWaterOhms();
  if (ohms < 0) {
    lcdLine(1, "CAL FAILED");
    lcdLine(2, "Probe not in water");
    return;
  }
  cellK = REF_PPM / rawTds(ohms, tUse);
  prefs.putFloat("cellK", cellK);
  lcdLine(1, "CAL OK  K=" + String(cellK, 3));
  lcdLine(2, "Saved");
  Serial.printf("คาลิเบรตแล้ว K = %.3f\n", cellK);
}

void setup() {
  Serial.begin(115200);
  pinMode(PIN_EXC_A, OUTPUT);
  pinMode(PIN_EXC_B, OUTPUT);
  digitalWrite(PIN_EXC_A, LOW);
  digitalWrite(PIN_EXC_B, LOW);
  pinMode(PIN_BUTTON, INPUT_PULLUP);

  prefs.begin("tdsac", false);
  cellK = prefs.getFloat("cellK", 1.0);

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
  lcdLine(0, "TDS METER (AC)");
  lcdLine(2, "Press GREEN button");
  lcdLine(3, "to measure");
}

void loop() {
  if (digitalRead(PIN_BUTTON) == LOW) {
    unsigned long pressedAt = millis();
    while (digitalRead(PIN_BUTTON) == LOW && millis() - pressedAt < 3000) delay(10);
    if (millis() - pressedAt >= 3000) {
      calibrate();                                   // กดค้าง 3 วินาที
      while (digitalRead(PIN_BUTTON) == LOW) delay(10);
    } else {
      showResult();                                  // กดสั้น
    }
    delay(50);
  }
}
