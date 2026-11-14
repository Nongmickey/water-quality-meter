/*
 * TDS meter (Total Dissolved Solids) for ESP32
 *
 * Based on the keyestudio KS0429 TDS Meter V1.0 test code and
 * https://randomnerdtutorials.com/esp32-tds-water-quality-sensor/
 *
 * Wiring:
 *   Sensor analog output -> TDS_PIN (GPIO 34)
 *   Sensor VCC           -> 3.3V
 *   Sensor GND           -> GND
 *   If the sensor is powered from 5V, its output can exceed 3.3V.
 *   Use a voltage divider on the signal line to protect the ESP32.
 *
 * Output: Serial Monitor at 115200 baud, one reading every PRINT_INTERVAL_MS.
 */

// ---------- Hardware ----------
const int TDS_PIN = 34;             // ADC1 pin. GPIO 27 (ADC2) cannot be read while Wi-Fi is on.
const float ADC_REFERENCE_V = 3.3;  // ESP32 reference voltage in volts
const int ADC_RESOLUTION = 4096;    // 12-bit ADC: readings are 0..4095

// ---------- Sampling ----------
const int SAMPLE_COUNT = 5;                   // readings used for the median filter (use an odd number)
const unsigned long SAMPLE_INTERVAL_MS = 40;  // time between ADC readings
const unsigned long PRINT_INTERVAL_MS = 800;  // time between printed results

// Water temperature used for compensation. Replace with a real sensor reading if you have one.
const float WATER_TEMPERATURE_C = 25.0;

// ---------- State ----------
int sampleBuffer[SAMPLE_COUNT];   // latest ADC readings (circular buffer)
int nextSampleIndex = 0;          // where the next reading will be stored
int storedSampleCount = 0;        // how many slots are filled so far, up to SAMPLE_COUNT

unsigned long lastSampleTime = 0;
unsigned long lastPrintTime = 0;

// ---------- Helpers ----------

// Returns the median of values[0..count-1] without changing the array.
int medianOf(const int values[], int count) {
  int sorted[SAMPLE_COUNT];
  for (int i = 0; i < count; i++) {
    sorted[i] = values[i];
  }

  // Insertion sort: simple and fast enough for a few values
  for (int i = 1; i < count; i++) {
    int current = sorted[i];
    int j = i - 1;
    while (j >= 0 && sorted[j] > current) {
      sorted[j + 1] = sorted[j];
      j--;
    }
    sorted[j + 1] = current;
  }

  if (count % 2 == 1) {
    return sorted[count / 2];
  }
  return (sorted[count / 2 - 1] + sorted[count / 2]) / 2;
}

// Converts a raw ADC reading (0..4095) to volts.
float adcToVolts(int adcValue) {
  return adcValue * ADC_REFERENCE_V / ADC_RESOLUTION;
}

// Converts sensor voltage to TDS in ppm, with temperature compensation.
float voltageToTds(float volts) {
  // Scale the voltage to what it would be at 25 °C
  float tempFactor = 1.0 + 0.02 * (WATER_TEMPERATURE_C - 25.0);
  float v = volts / tempFactor;

  // Empirical curve from the KS0429 test code
  float tds = (133.42 * v * v * v) - (255.86 * v * v) + (857.39 * v);
  tds *= 0.5;

  // A negative value is not physically possible
  if (tds < 0) {
    tds = 0;
  }
  return tds;
}

// Takes the median of the stored readings, converts it, and prints the result.
void printTdsReading() {
  int medianAdc = medianOf(sampleBuffer, SAMPLE_COUNT);
  float volts = adcToVolts(medianAdc);
  float tds = voltageToTds(volts);

  Serial.print("Voltage: ");
  Serial.print(volts, 3);
  Serial.print(" V   TDS: ");
  Serial.print(tds, 0);
  Serial.println(" ppm");
}

// ---------- Arduino entry points ----------

void setup() {
  Serial.begin(115200);
  pinMode(TDS_PIN, INPUT);
}

void loop() {
  unsigned long now = millis();

  // Take a new reading every SAMPLE_INTERVAL_MS
  if (now - lastSampleTime >= SAMPLE_INTERVAL_MS) {
    lastSampleTime = now;

    sampleBuffer[nextSampleIndex] = analogRead(TDS_PIN);
    nextSampleIndex = (nextSampleIndex + 1) % SAMPLE_COUNT;

    if (storedSampleCount < SAMPLE_COUNT) {
      storedSampleCount++;
    }
  }

  // Print a result every PRINT_INTERVAL_MS
  if (now - lastPrintTime >= PRINT_INTERVAL_MS) {
    lastPrintTime = now;

    // Skip printing until the buffer is full, so startup zeros don't skew the result
    if (storedSampleCount == SAMPLE_COUNT) {
      printTdsReading();
    }
  }
}
