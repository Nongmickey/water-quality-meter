/*
 * ทดสอบ TDS ทีละขั้น (วงจร ESP32 สร้างไฟสลับ ต่อสายเหมือน tds_ac)
 *
 * ทำตามที่จอบอกทีละขั้น ดูค่าวิ่ง + PASS/FAIL แล้วกดปุ่มเขียว = บันทึกผล ไปขั้นถัดไป
 *   1/4 AIR           ยกหัววัดขึ้นจากน้ำ เช็ดให้แห้ง        ผ่านถ้า R = open (ไม่มีไฟไหล)
 *   2/4 4.7k RESISTOR ถอดหัววัด ต่อตัวต้านทาน 4.7k แทน    ผ่านถ้า R อยู่ระหว่าง 3800-5600
 *   3/4 TAP WATER     ใส่หัววัดกลับ จุ่มน้ำประปา             ผ่านถ้า มีไฟไหล (R ไม่ใช่ open)
 *   4/4 SALT WATER    ล้างหัววัด จุ่มน้ำเกลือ               ผ่านถ้า R ต่ำกว่าน้ำประปาชัดเจน (TDS สูงกว่า)
 *   RESULT            สรุปผล 4 ขั้น และบอกจุดที่น่าจะเสีย    กดปุ่มอีกครั้ง = เริ่มใหม่
 *
 * ต่อสาย: S ของ D25 -> 4.7k -> จุด X ; จุด X -> S ของ D33 และหัววัดขั้ว 1 ; หัววัดขั้ว 2 -> S ของ D26
 *         ปุ่มเขียว: ขั้ว 23 -> S ของ D15, ขั้ว 24 -> G
 *         LCD: SDA -> D21, SCL -> D22, VCC -> VIN, GND -> G
 * ไลบรารี: "LiquidCrystal I2C" (by Frank de Brabander)
 */

#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Preferences.h>

const int   PIN_EXC_A  = 25;      // ขาสร้างไฟ ฝั่งตัวต้านทาน
const int   PIN_EXC_B  = 26;      // ขาสร้างไฟ ฝั่งหัววัดขั้ว 2
const int   PIN_SENSE  = 33;      // จุด X
const int   PIN_BUTTON = 15;
const float R_SERIES   = 4700.0;
const int   CYCLES     = 300;

const int STEPS = 4;
const char *TITLES[STEPS] = {"1/4 AIR (dry probe)", "2/4 4.7k RESISTOR", "3/4 TAP WATER", "4/4 SALT WATER"};
const char *NAMES[STEPS]  = {"AIR", "RES", "TAP", "SALT"};

LiquidCrystal_I2C *lcd = nullptr;
float cellK = 1.0;               // ค่าคาลิเบรตจาก tds_ac (ถ้ามี)
int   step = 0;                  // 0-3 = ขั้นทดสอบ, 4 = สรุปผล
bool  result[STEPS] = {false, false, false, false};
float tapR = -1;                 // R ของน้ำประปา ใช้เทียบกับน้ำเกลือ
float lastV1 = 0, lastV2 = 0;
float lastR = -1;                // -1 = open (ไม่มีไฟไหล)
unsigned long nextRun = 0;

void lcdLine(int row, String s) {
  if (!lcd) return;
  while (s.length() < 20) s += ' ';
  lcd->setCursor(0, row);
  lcd->print(s.substring(0, 20));
}

// วัด 1 ครั้ง: ได้ V1 (คร่อมน้ำ) V2 (คร่อมตัวต้านทาน) และ R
void measure() {
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
  pinMode(PIN_EXC_A, INPUT);                       // วัดเสร็จ ปล่อยลอย
  pinMode(PIN_EXC_B, INPUT);

  lastV1 = sum1 / (float)CYCLES;
  lastV2 = sum2 / (float)CYCLES;
  lastR  = (lastV2 < 15) ? -1 : R_SERIES * lastV1 / lastV2;
}

// ขั้นนี้ผ่านหรือไม่ (ดูจากค่าล่าสุด)
bool pass(int s) {
  if (s == 0) return lastR < 0;                         // อากาศ: ต้องไม่มีไฟไหล
  if (s == 1) return lastR >= 3800 && lastR <= 5600;    // ตัวต้านทาน 4.7k
  if (s == 2) return lastR > 0;                         // น้ำประปา: ต้องมีไฟไหล
  return lastR > 0 && tapR > 0 && lastR < tapR * 0.7;   // น้ำเกลือ: R ต่ำกว่าน้ำประปาชัดเจน
}

void showStep() {
  measure();
  String rTxt;
  if (lastR < 0) {
    rTxt = "R: open";
  } else {
    float tds = 1000000.0 / lastR * 0.5 * cellK;   // ที่ 25 °C
    rTxt = "R:" + String(lastR, 0) + " TDS:" + String(tds, 0);
  }
  String vTxt = "V1:" + String(lastV1, 0) + " V2:" + String(lastV2, 0);
  bool ok = pass(step);
  String verdict = ok ? String("PASS  GREEN=next") : String("FAIL  GREEN=next");
  if (step == 3 && tapR < 0) verdict = "FAIL: redo step 3";

  lcdLine(1, rTxt);
  lcdLine(2, vTxt);
  lcdLine(3, verdict);
  Serial.println(String(NAMES[step]) + " | " + rTxt + " | " + vTxt + " | " + (ok ? "PASS" : "FAIL"));
}

const char *okText(bool b) { return b ? "OK" : "NG"; }

// ขั้นแรกที่ไม่ผ่าน บอกจุดที่น่าจะเสีย
String hint() {
  if (!result[0]) return "Probe wet / short";
  if (!result[1]) return "Check D25 D26 D33";
  if (!result[2]) return "Probe contact bad";
  if (!result[3]) return "SALT not > TAP";
  return "ALL PASS";
}

void showResult() {
  String l1 = "AIR:" + String(okText(result[0])) + "  RES:" + okText(result[1]);
  String l2 = "TAP:" + String(okText(result[2])) + " SALT:" + okText(result[3]);
  lcdLine(0, "RESULT  GREEN=again");
  lcdLine(1, l1);
  lcdLine(2, l2);
  lcdLine(3, hint());
  Serial.println();
  Serial.println("=== RESULT ===");
  Serial.println(l1);
  Serial.println(l2);
  Serial.println(hint());
}

void startStep() {
  if (step >= STEPS) {
    showResult();
    return;
  }
  lcdLine(0, TITLES[step]);
  lcdLine(1, "");
  lcdLine(2, "Measuring...");
  lcdLine(3, "");
  Serial.println();
  Serial.println(String("=== ") + TITLES[step] + " ===");
  nextRun = 0;
}

void setup() {
  Serial.begin(115200);
  pinMode(PIN_BUTTON, INPUT_PULLUP);
  pinMode(PIN_EXC_A, INPUT);
  pinMode(PIN_EXC_B, INPUT);

  Preferences prefs;                               // โหลด K จาก tds_ac (อ่านอย่างเดียว)
  prefs.begin("tdsac", true);
  cellK = prefs.getFloat("cellK", 1.0);
  prefs.end();

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
  startStep();
}

void loop() {
  if (digitalRead(PIN_BUTTON) == LOW) {
    delay(30);                                     // กันปุ่มเด้ง
    if (digitalRead(PIN_BUTTON) == LOW) {
      while (digitalRead(PIN_BUTTON) == LOW) delay(10);   // รอปล่อยปุ่ม
      if (step < STEPS) {
        measure();                                 // วัดอีกครั้งแล้วบันทึกผลขั้นนี้
        result[step] = pass(step);
        if (step == 2) tapR = lastR;
        Serial.printf("บันทึก %s: %s (R = %.0f)\n", NAMES[step], result[step] ? "PASS" : "FAIL", lastR);
        step++;
      } else {
        step = 0;                                  // เริ่มใหม่
        tapR = -1;
      }
      startStep();
    }
  }

  if (step < STEPS && millis() >= nextRun) {
    nextRun = millis() + 500;
    showStep();
  }
  delay(10);
}
