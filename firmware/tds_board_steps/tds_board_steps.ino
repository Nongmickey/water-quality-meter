/*
 * ทดสอบบอร์ด TDS ตัวเดิม (TDS Meter V1.0) ทีละขั้น — หาว่าเสียจากโรงงาน หรือจากการต่อผิด
 *
 * ทำตามที่จอบอกทีละขั้น ดูค่าวิ่ง + PASS/FAIL แล้วกดปุ่มเขียว = บันทึกผล ไปขั้นถัดไป
 *   1/5 NO PROBE   ถอดหัววัดออกจากช่องขาวของบอร์ด            ผ่านถ้า A < 100 mV
 *   2/5 SHORT      เสียบหัววัด เอาไขควงแตะเข็ม 2 อันพร้อมกัน   ผ่านถ้า A > 1500 mV
 *   3/5 RESISTOR   ถอดหัววัด เสียบตัวต้านทาน 4.7k คร่อม 2 ขาช่องขาว  ผ่านถ้า A อยู่ระหว่าง 200-2000 mV
 *   4/5 TAP WATER  เสียบหัววัด จุ่มน้ำประปา                      ผ่านถ้า A > 50 mV
 *   5/5 SALT WATER ล้างหัววัด จุ่มน้ำเกลือ                       ผ่านถ้า A สูงกว่าน้ำประปาชัดเจน
 *   RESULT         สรุปผล + บอกสาเหตุที่น่าจะเป็น                 กดปุ่มอีกครั้ง = เริ่มใหม่
 *
 * อ่านผล
 *   ขั้น 1-3 ไม่ผ่าน              = ชิปบนบอร์ดเสีย หรือสายต่อผิด (มักเกิดจากต่อไฟผิด)
 *   ขั้น 1-3 ผ่าน แต่ 4-5 ไม่ผ่าน = วงจรสร้างไฟสลับบนบอร์ดเสีย (มักเป็นจากโรงงาน)
 *
 * ต่อสาย (เหมือนภาพ wiring-tds)
 *   บอร์ด TDS  + -> 3V3    - -> G    A -> 4.7k -> S ของ D34 (ถ้าต่อขา A ไว้ที่ D33 ให้แก้ PIN_A เป็น 33)
 *   ปุ่มเขียว: ขั้ว 23 -> S ของ D15, ขั้ว 24 -> G
 *   LCD: SDA -> D21, SCL -> D22, VCC -> VIN, GND -> G
 *   สายวงจร tds_ac (D25 D26) ต่อค้างไว้ได้ โค้ดนี้ปล่อยขาลอยไว้ แต่หัววัดต้องเสียบที่บอร์ด TDS
 * ไลบรารี: "LiquidCrystal I2C" (by Frank de Brabander)
 */

#include <Wire.h>
#include <LiquidCrystal_I2C.h>

const int PIN_A      = 34;      // ขา A ของบอร์ด TDS (ผ่าน 4.7k)
const int PIN_BUTTON = 15;

const int STEPS = 5;
const char *TITLES[STEPS] = {"1/5 NO PROBE", "2/5 SHORT (screw)", "3/5 RESISTOR 4.7k",
                             "4/5 TAP WATER", "5/5 SALT WATER"};
const char *NAMES[STEPS]  = {"NO PROBE", "SHORT", "RESISTOR", "TAP", "SALT"};

LiquidCrystal_I2C *lcd = nullptr;
int   step = 0;                 // 0-4 = ขั้นทดสอบ, 5 = สรุปผล
bool  result[STEPS] = {false, false, false, false, false};
float tapMv = -1;               // ค่าตอนน้ำประปา ใช้เทียบกับน้ำเกลือ
float lastMv = 0;
unsigned long nextRun = 0;

void lcdLine(int row, String s) {
  if (!lcd) return;
  while (s.length() < 20) s += ' ';
  lcd->setCursor(0, row);
  lcd->print(s.substring(0, 20));
}

// อ่านขา A 31 ครั้ง ใช้ค่ากลาง (mV)
void measure() {
  int buf[31];
  for (int i = 0; i < 31; i++) {
    buf[i] = analogReadMilliVolts(PIN_A);
    delay(10);
  }
  for (int i = 1; i < 31; i++) {
    int v = buf[i], j = i - 1;
    while (j >= 0 && buf[j] > v) { buf[j + 1] = buf[j]; j--; }
    buf[j + 1] = v;
  }
  lastMv = buf[15];
}

// แปลงเป็น ppm ด้วยสูตร DFRobot (ที่ 25 °C) ไว้ดูประกอบ
float dfrPpm(float mv) {
  float v = mv / 1000.0;
  float tds = (133.42 * v * v * v - 255.86 * v * v + 857.39 * v) * 0.5;
  return tds < 0 ? 0 : tds;
}

// ขั้นนี้ผ่านหรือไม่ (ดูจากค่าล่าสุด)
bool pass(int s) {
  if (s == 0) return lastMv < 100;                           // ไม่มีหัววัด: ต้องได้ประมาณ 0
  if (s == 1) return lastMv > 1500;                          // ช็อตเข็ม: ต้องขึ้นสูง
  if (s == 2) return lastMv >= 200 && lastMv <= 2000;        // ตัวต้านทาน: ต้องได้ค่ากลางๆ
  if (s == 3) return lastMv > 50;                            // น้ำประปา: ต้องมีค่า
  return tapMv > 0 && lastMv > tapMv * 1.5 && lastMv - tapMv > 150;   // น้ำเกลือ > น้ำประปาชัดเจน
}

void showStep() {
  measure();
  bool ok = pass(step);
  String aTxt = "A   : " + String(lastMv, 0) + " mV";
  String pTxt = "DFR : " + String(dfrPpm(lastMv), 0) + " ppm";
  String verdict = ok ? String("PASS  GREEN=next") : String("FAIL  GREEN=next");
  if (step == 4 && tapMv < 0) verdict = "FAIL: redo step 4";

  lcdLine(1, aTxt);
  lcdLine(2, pTxt);
  lcdLine(3, verdict);
  Serial.println(String(NAMES[step]) + " | " + aTxt + " | " + pTxt + " | " + (ok ? "PASS" : "FAIL"));
}

const char *okText(bool b) { return b ? "OK" : "NG"; }

// ขั้นแรกที่ไม่ผ่าน บอกสาเหตุที่น่าจะเป็น
String hint() {
  if (!result[0]) return "Stuck high: damaged";
  if (!result[1]) return "No output: wiring?";
  if (!result[2]) return "Amplifier fault";
  if (!result[3] || !result[4]) return "AC fault (factory)";
  return "BOARD OK";
}

void showResult() {
  String l1 = "1:" + String(okText(result[0])) + " 2:" + okText(result[1]) + " 3:" + okText(result[2]);
  String l2 = "4:" + String(okText(result[3])) + " 5:" + okText(result[4]);
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
  pinMode(25, INPUT);                              // ขาของวงจร tds_ac ปล่อยลอย ไม่ให้รบกวน
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
        if (step == 3) tapMv = lastMv;
        Serial.printf("บันทึก %s: %s (A = %.0f mV)\n", NAMES[step], result[step] ? "PASS" : "FAIL", lastMv);
        step++;
      } else {
        step = 0;                                  // เริ่มใหม่
        tapMv = -1;
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
