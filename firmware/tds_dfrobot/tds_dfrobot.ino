/*
 * โค้ดตัวอย่าง TDS ของ DFRobot (เดิมเขียนให้ Arduino UNO) แก้ให้ใช้กับ ESP32 + บอร์ดขยาย
 *
 * สิ่งที่แก้จากโค้ดเดิม
 *   1) ขาเซนเซอร์: A1 (UNO)  ->  GPIO34 = ช่อง S ของ D34 บนบอร์ดขยาย
 *   2) อ่านค่า:  analogRead() * 5.0 / 1024  ->  analogReadMilliVolts() / 1000
 *      ESP32 อ่านละเอียด 12 บิต (0-4095) และรับแรงดันได้ไม่เกิน 3.3V
 *      analogReadMilliVolts() ให้ค่าแรงดันที่ ESP32 คาลิเบรตมาจากโรงงานแล้ว แม่นกว่าคิดจาก VREF เอง
 *   3) เพิ่มการแสดงผลบนจอ LCD 20x4 (I2C)
 *   ส่วนอื่นเหมือนโค้ดเดิมทั้งหมด: เก็บค่าทุก 40 ms ใช้ค่ากลางจาก 30 ค่า แสดงผลทุก 800 ms
 *   ชดเชยอุณหภูมิ และสูตรแปลงแรงดันเป็น TDS
 *
 * ต่อสาย (บอร์ดขยาย ESP32)
 *   TDS  -  -> ช่อง G (ช่องไหนก็ได้)
 *   TDS  +  -> 3V3
 *   TDS  A  -> 4.7k -> ช่อง S ของ D34  (10µF + 104 คร่อม D34 กับ G)
 *   LCD  SDA -> ช่อง S ของ D21, SCL -> ช่อง S ของ D22, VCC -> VIN, GND -> G
 *   ห้ามใช้ช่อง V บนบอร์ดขยาย
 *
 * ไลบรารี: "LiquidCrystal I2C" (by Frank de Brabander)
 */

#include <Wire.h>
#include <LiquidCrystal_I2C.h>

#define TdsSensorPin 34      // เดิม A1 -> ESP32 ใช้ GPIO34 (ช่อง S ของ D34)
#define SCOUNT 30            // จำนวนค่าที่เก็บไว้หาค่ากลาง (เหมือนเดิม)
#define LCD_SDA 21
#define LCD_SCL 22

int analogBuffer[SCOUNT];    // เก็บแรงดัน (mV) ที่อ่านได้
int analogBufferTemp[SCOUNT];
int analogBufferIndex = 0, copyIndex = 0;
float averageVoltage = 0, tdsValue = 0, temperature = 25;   // อุณหภูมิน้ำ (°C) แก้ให้ตรงกับน้ำจริงได้

LiquidCrystal_I2C *lcd = nullptr;

int getMedianNum(int bArray[], int iFilterLen);

void lcdLine(int row, String s) {
  if (!lcd) return;
  while (s.length() < 20) s += ' ';
  lcd->setCursor(0, row);
  lcd->print(s.substring(0, 20));
}

void setup()
{
  Serial.begin(115200);
  pinMode(TdsSensorPin, INPUT);
  analogReadResolution(12);  // ESP32: 12 บิต (0-4095)

  // หาจอ LCD (ที่อยู่ I2C ที่เป็นไปได้ทั้งหมด)
  Wire.begin(LCD_SDA, LCD_SCL);
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
  lcdLine(0, "TDS (DFRobot code)");
}

void loop()
{
  static unsigned long analogSampleTimepoint = millis();
  if (millis() - analogSampleTimepoint > 40U) // ทุก 40 ms อ่านค่าเก็บไว้ในบัฟเฟอร์
  {
    analogSampleTimepoint = millis();
    analogBuffer[analogBufferIndex] = analogReadMilliVolts(TdsSensorPin); // เดิม analogRead()
    analogBufferIndex++;
    if (analogBufferIndex == SCOUNT)
      analogBufferIndex = 0;
  }
  static unsigned long printTimepoint = millis();
  if (millis() - printTimepoint > 800U)        // ทุก 800 ms คำนวณและแสดงผล
  {
    printTimepoint = millis();
    for (copyIndex = 0; copyIndex < SCOUNT; copyIndex++)
      analogBufferTemp[copyIndex] = analogBuffer[copyIndex];
    // เดิม: getMedianNum(...) * (float)VREF / 1024.0  ->  ESP32: ค่ากลาง (mV) / 1000 = โวลต์
    averageVoltage = getMedianNum(analogBufferTemp, SCOUNT) / 1000.0;
    float compensationCoefficient = 1.0 + 0.02 * (temperature - 25.0); // ชดเชยอุณหภูมิ
    float compensationVolatge = averageVoltage / compensationCoefficient;
    tdsValue = (133.42 * compensationVolatge * compensationVolatge * compensationVolatge
              - 255.86 * compensationVolatge * compensationVolatge
              + 857.39 * compensationVolatge) * 0.5; // แปลงแรงดันเป็น TDS

    Serial.print("TDS Value:");
    Serial.print(tdsValue, 0);
    Serial.println("ppm");

    lcdLine(2, "TDS : " + String(tdsValue, 0) + " ppm");
    lcdLine(3, "V   : " + String(averageVoltage, 3) + " V");
  }
}

int getMedianNum(int bArray[], int iFilterLen)
{
  int bTab[iFilterLen];
  for (byte i = 0; i < iFilterLen; i++)
    bTab[i] = bArray[i];
  int i, j, bTemp;
  for (j = 0; j < iFilterLen - 1; j++)
  {
    for (i = 0; i < iFilterLen - j - 1; i++)
    {
      if (bTab[i] > bTab[i + 1])
      {
        bTemp = bTab[i];
        bTab[i] = bTab[i + 1];
        bTab[i + 1] = bTemp;
      }
    }
  }
  if ((iFilterLen & 1) > 0)
    bTemp = bTab[(iFilterLen - 1) / 2];
  else
    bTemp = (bTab[iFilterLen / 2] + bTab[iFilterLen / 2 - 1]) / 2;
  return bTemp;
}
