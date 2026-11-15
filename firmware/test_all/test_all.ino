/*
 * ทดสอบทีละอย่าง: จอ+ปุ่ม -> อุณหภูมิ -> pH -> TDS
 *
 * กดปุ่มเขียว = ไปทดสอบข้อถัดไป (ครบ 4 ข้อแล้ววนกลับข้อ 1)
 * แต่ละข้อแสดงค่าวิ่งทุก 1 วินาที
 *
 * ข้อ 1  LCD + ปุ่ม        : เห็นข้อความครบ 4 บรรทัด และกดปุ่มแล้วไปข้อ 2 ได้ = ผ่าน
 * ข้อ 2  อุณหภูมิ (D32)    : อากาศในห้องประมาณ 28-32 C, กำหัววัดแล้วค่าขึ้น = ผ่าน
 * ข้อ 3  pH (D35)          : น้ำประปาประมาณ 6.5-8, น้ำส้มสายชูค่าลดลง = ผ่าน
 * ข้อ 4  TDS (D25 D26 D33) : อากาศขึ้น Probe not in water, น้ำเกลือค่าสูงกว่าน้ำประปา = ผ่าน
 *
 * ใช้ค่าคาลิเบรตที่บันทึกไว้จากโค้ด ph_test และ tds_ac (ถ้ายังไม่เคยคาลิเบรต ใช้ค่าเริ่มต้น)
 * ทดสอบ pH กับ TDS ทีละแก้ว อย่าจุ่มหัววัดทั้งสองในน้ำเดียวกันพร้อมกัน
 *
 * ต่อสาย (เหมือนภาพ wiring-ph, wiring-tds-ac, wiring-temp)
 *   ปุ่มเขียว: ขั้ว 23 -> S ของ D15, ขั้ว 24 -> G
 *   LCD: SDA -> D21, SCL -> D22, VCC -> VIN, GND -> G
 *   อุณหภูมิ: 3V3 -> 4.7k -> จุด T -> S ของ D32 ; NTC ระหว่างจุด T กับ G
 *   pH: V+ -> VIN, G -> G, Po -> 4.7k -> จุด P -> S ของ D35 ; จุด P -> 4.7k -> G
 *   TDS: S ของ D25 -> 4.7k -> จุด X -> S ของ D33 และหัววัดขั้ว 1 ; หัววัดขั้ว 2 -> S ของ D26
 * ไลบรารี: "LiquidCrystal I2C" (by Frank de Brabander)
 */

#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Preferences.h>
#include <math.h>

// ---------- ขา ----------
const int PIN_BUTTON = 15;
const int PIN_TEMP   = 32;
const int PIN_PH     = 35;
const int PIN_EXC_A  = 25;   // TDS ขาสร้างไฟ ฝั่งตัวต้านทาน
const int PIN_EXC_B  = 26;   // TDS ขาสร้างไฟ ฝั่งหัววัดขั้ว 2
const int PIN_SENSE  = 33;   // TDS จุด X

// ---------- ค่าคงที่ ----------
const float NTC_SERIES = 4700.0;    // อุณหภูมิ
const float NTC_R25    = 10000.0;
const float NTC_BETA   = 3950.0;
const float PH_DIVIDER = 2.0;       // pH
const float CAL_HIGH   = 6.86;
const float CAL_LOW    = 4.00;
const float R_SERIES   = 4700.0;    // TDS
const int   CYCLES     = 200;

const int TESTS = 4;
const char *TITLES[TESTS] = {"1/4 LCD + BUTTON", "2/4 TEMP (D32)", "3/4 pH (D35)", "4/4 TDS D25 D26 D33"};

LiquidCrystal_I2C *lcd = nullptr;
uint8_t lcdAddr = 0;
int test = 0;
unsigned long nextRun = 0;

float mvHigh = 2500.0, mvLow = 3020.0;   // pH (โหลดจากที่คาลิเบรตไว้)
bool  phCal = false;
float cellK = 1.0;                       // TDS (โหลดจากที่คาลิเบรตไว้)

void lcdLine(int row, String s) {
  if (!lcd) return;
  while (s.length() < 20) s += ' ';
  lcd->setCursor(0, row);
  lcd->print(s.substring(0, 20));
}

void show(String l1, String l2, String l3) {
  lcdLine(1, l1);
  lcdLine(2, l2);
  lcdLine(3, l3);
  Serial.println(l1 + " | " + l2 + " | " + l3);
}

// อ่าน 31 ครั้ง ใช้ค่ากลาง (mV)
int readMedianMv(int pin, int gapMs) {
  int buf[31];
  for (int i = 0; i < 31; i++) {
    buf[i] = analogReadMilliVolts(pin);
    delay(gapMs);
  }
  for (int i = 1; i < 31; i++) {
    int v = buf[i], j = i - 1;
    while (j >= 0 && buf[j] > v) { buf[j + 1] = buf[j]; j--; }
    buf[j + 1] = v;
  }
  return buf[15];
}

// ---------- ข้อ 1: จอ + ปุ่ม ----------
void testLcd() {
  char addr[8];
  snprintf(addr, sizeof(addr), "0x%02X", lcdAddr);
  show(lcd ? "LCD addr: " + String(addr) : String("LCD not found"),
       "Time: " + String(millis() / 1000) + " s",
       "GREEN = next test");
}

// ---------- ข้อ 2: อุณหภูมิ ----------
// คืนอุณหภูมิ (°C) หรือ NAN ถ้าสายหลุด/ช็อต ; mv = แรงดันที่ D32
float readTemp(int &mv) {
  mv = readMedianMv(PIN_TEMP, 2);
  if (mv > 3000 || mv < 100) return NAN;
  float r = NTC_SERIES * mv / (3300.0 - mv);
  return 1.0 / (1.0 / 298.15 + log(r / NTC_R25) / NTC_BETA) - 273.15;
}

void testTemp() {
  int mv;
  float t = readTemp(mv);
  String mvTxt = "D32 : " + String(mv) + " mV";
  if (mv > 3000)     show("Temp: -", "NTC not connected", mvTxt);
  else if (mv < 100) show("Temp: -", "Check 4.7k / 3V3", mvTxt);
  else {
    float r = NTC_SERIES * mv / (3300.0 - mv);
    show("Temp: " + String(t, 1) + " C", "R   : " + String(r, 0) + " ohm", mvTxt);
  }
}

// ---------- ข้อ 3: pH ----------
void testPh() {
  float mv = readMedianMv(PIN_PH, 3) * PH_DIVIDER;   // แรงดันที่ขา Po
  String poTxt = "Po  : " + String(mv / 1000.0, 3) + " V";
  if (mv < 300) {
    show("pH  : -", "Check pH module", poTxt);
    return;
  }
  float diff = mvHigh - mvLow;
  if (fabs(diff) < 50) diff = -520;
  float ph = CAL_HIGH + (CAL_HIGH - CAL_LOW) / diff * (mv - mvHigh);
  ph = constrain(ph, 0.0, 14.0);
  show("pH  : " + String(ph, 2), poTxt, phCal ? "Calibrated" : "Not calibrated");
}

// ---------- ข้อ 4: TDS (ESP32 สร้างไฟสลับเอง) ----------
void testTds() {
  int tmv;
  float t = readTemp(tmv);
  float tUse = isnan(t) ? 25.0 : t;

  pinMode(PIN_EXC_A, OUTPUT);
  pinMode(PIN_EXC_B, OUTPUT);
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
  digitalWrite(PIN_EXC_A, LOW);
  digitalWrite(PIN_EXC_B, LOW);
  pinMode(PIN_EXC_A, INPUT);                       // วัดเสร็จ ปล่อยลอย ไม่ให้หัววัด TDS ไปรบกวน pH
  pinMode(PIN_EXC_B, INPUT);

  float v1 = sum1 / (float)CYCLES;
  float v2 = sum2 / (float)CYCLES;
  String vTxt = "V1:" + String(v1, 0) + " V2:" + String(v2, 0);
  if (v2 < 15) {
    show("TDS : 0 ppm", "Probe not in water", vTxt);
    return;
  }
  float rw   = R_SERIES * v1 / v2;
  float ec25 = (1000000.0 / rw) / (1.0 + 0.02 * (tUse - 25.0));
  float tds  = ec25 * 0.5 * cellK;
  String tTxt = isnan(t) ? String("25(set)") : String(t, 1);
  show("TDS : " + String(tds, 0) + " ppm", "R:" + String(rw, 0) + " T:" + tTxt + "C", vTxt);
}

void startTest() {
  lcdLine(0, TITLES[test]);
  lcdLine(1, "");
  lcdLine(2, "Measuring...");
  lcdLine(3, "");
  Serial.println();
  Serial.println(String("=== ") + TITLES[test] + " ===");
  nextRun = 0;
}

void setup() {
  Serial.begin(115200);
  pinMode(PIN_BUTTON, INPUT_PULLUP);
  pinMode(PIN_EXC_A, INPUT);
  pinMode(PIN_EXC_B, INPUT);

  // โหลดค่าคาลิเบรตที่บันทึกไว้ (อ่านอย่างเดียว ไม่แก้)
  Preferences prefs;
  prefs.begin("ph", true);
  float h = prefs.getFloat("mvHigh", NAN);
  float l = prefs.getFloat("mvLow", NAN);
  prefs.end();
  if (!isnan(h)) mvHigh = h;
  if (!isnan(l)) mvLow = l;
  phCal = !isnan(h) && !isnan(l);

  prefs.begin("tdsac", true);
  float k = prefs.getFloat("cellK", NAN);
  prefs.end();
  if (!isnan(k)) cellK = k;

  Wire.begin(21, 22);
  const uint8_t addrs[] = {0x27, 0x3F, 0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26,
                           0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E};
  for (uint8_t a : addrs) {
    Wire.beginTransmission(a);
    if (Wire.endTransmission() == 0) {
      lcdAddr = a;
      lcd = new LiquidCrystal_I2C(a, 20, 4);
      lcd->init();
      lcd->backlight();
      break;
    }
  }
  Serial.printf("pH cal: %s  (6.86 = %.0f mV, 4.00 = %.0f mV)   TDS K = %.3f\n",
                phCal ? "yes" : "no", mvHigh, mvLow, cellK);
  startTest();
}

void loop() {
  if (digitalRead(PIN_BUTTON) == LOW) {
    delay(30);                                     // กันปุ่มเด้ง
    if (digitalRead(PIN_BUTTON) == LOW) {
      while (digitalRead(PIN_BUTTON) == LOW) delay(10);   // รอปล่อยปุ่ม
      test = (test + 1) % TESTS;
      startTest();
    }
  }

  if (millis() >= nextRun) {
    nextRun = millis() + 1000;
    if (test == 0)      testLcd();
    else if (test == 1) testTemp();
    else if (test == 2) testPh();
    else                testTds();
  }
  delay(10);
}
