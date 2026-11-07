// ===== ตั้งค่าเครื่องวัดคุณภาพน้ำ =====
// แก้ค่าในไฟล์นี้ได้เลย แล้วกด Upload ใหม่
#pragma once

// ================= ขาที่ใช้ =================
// ขาอนาล็อกต้องอยู่กลุ่ม ADC1 (32-39) เพราะ ADC2 ใช้ไม่ได้ตอนเปิด WiFi
const int PIN_TDS    = 34;  // TDS: ขา A
const int PIN_PH     = 35;  // pH: ขา Po ผ่านตัวแบ่งแรงดัน 10k/20k
const int PIN_TEMP   = 32;  // NTC 10k + ตัวต้านทาน 10k ไป 3V3
const int PIN_BUTTON = 15;  // ปุ่มเขียว (อีกขาต่อ GND)
const int PIN_SDA    = 21;
const int PIN_SCL    = 22;

// ================= ค่าวงจร =================
const float VREF_MV         = 3300.0;   // ไฟเลี้ยงตัวแบ่งแรงดัน NTC (3V3)
const float NTC_SERIES_OHM  = 10000.0;  // ตัวต้านทานที่ต่อกับ NTC
const float NTC_NOMINAL_OHM = 10000.0;  // NTC 10k ที่ 25°C
const float NTC_BETA        = 3950.0;   // ค่า B ของ NTC
const float PH_DIVIDER      = 1.5;      // (10k + 20k) / 20k

// ค่าที่ใช้ตัดสินว่า "เซนเซอร์ไม่ได้ต่อ"
const float NOISE_SPREAD_MV = 250.0;    // สัญญาณแกว่งเกินนี้ = สายลอย
const float TDS_MIN_MV      = 10.0;     // ต่ำกว่านี้ = หัววัดไม่ได้จุ่มน้ำ
const float PH_MIN_MV       = 150.0;    // ที่ขา ESP32, ต่ำกว่านี้ = ไม่ได้ต่อโมดูล pH
const float ADC_MAX_MV      = 3150.0;   // ใกล้เพดาน ADC = ค่าเชื่อไม่ได้

// ================= WiFi (ESP32 ปล่อย WiFi เอง) =================
const char *AP_SSID = "WaterMeter";
const char *AP_PASS = "12345678";        // อย่างน้อย 8 ตัวอักษร
