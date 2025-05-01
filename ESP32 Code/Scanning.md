# ESP32 + Dual CD74HC4067 Multiplexer Matrix Scan

This project demonstrates how to use an **ESP32** and two **CD74HC4067 16-channel analog multiplexers** to scan a 4×4 sensor matrix. It reads analog voltages at each intersection and prints a voltage matrix via serial.

---

## Hardware Overview

- **ESP32**
- **CD74HC4067 x2** (MUX1 for columns, MUX2 for rows)
- **Analog sensor matrix** (e.g., pressure or capacitive sensors)
- **Wiring**:  
  - `MUX1` S0–S3 → GPIO33, 25, 26, 27  
  - `MUX2` S0–S3 → GPIO21, 22, 23, 14  
  - Output signal → `ADC_INPUT = GPIO36`

---

## Core Idea

This project uses **row-column multiplexing** to access each sensor point individually with only one ADC pin.  
The code selects a row (via MUX2), then scans each column (via MUX1), and reads voltage at the intersection using `analogRead()`.

---

## Full Code with Comments

```cpp
// Define control pins for the first CD74HC4067 multiplexer (MUX1)
#define CD4067_1_S0  33  // Address line S0 of MUX1
#define CD4067_1_S1  25  // Address line S1 of MUX1
#define CD4067_1_S2  26  // Address line S2 of MUX1
#define CD4067_1_S3  27  // Address line S3 of MUX1

// Define control pins for the second CD74HC4067 multiplexer (MUX2)
#define CD4067_2_S0  21  // Address line S0 of MUX2
#define CD4067_2_S1  22  // Address line S1 of MUX2
#define CD4067_2_S2  23  // Address line S2 of MUX2
#define CD4067_2_S3  14  // Address line S3 of MUX2

// Define the analog input pin (connected to sensor matrix)
#define ADC_INPUT 36     // GPIO36 (ADC1 channel 0)

// Define how many samples to average per reading
#define SAMPLES_PER_CHANNEL 3  // Average 3 samples for stability

// Delay between matrix scans
#define SCAN_DELAY 100  // 100 ms

void setup() {
  Serial.begin(250000);  // Start serial monitor

  // Initialize address pins for MUX1
  pinMode(CD4067_1_S0, OUTPUT);
  pinMode(CD4067_1_S1, OUTPUT);
  pinMode(CD4067_1_S2, OUTPUT);
  pinMode(CD4067_1_S3, OUTPUT);

  // Initialize address pins for MUX2
  pinMode(CD4067_2_S0, OUTPUT);
  pinMode(CD4067_2_S1, OUTPUT);
  pinMode(CD4067_2_S2, OUTPUT);
  pinMode(CD4067_2_S3, OUTPUT);

  Serial.println("Initialization complete, starting channel scan...");
}

void loop() {
  float matrix[4][4];  // 4x4 matrix to store voltages

  for (int ch2 = 1; ch2 <= 4; ch2++) {  // Select row via MUX2
    selectChannel(CD4067_2_S0, CD4067_2_S1, CD4067_2_S2, CD4067_2_S3, ch2);
    delayMicroseconds(500);  // Stabilization delay

    int ch1_list[] = {1, 2, 3, 4};  // Column indices

    for (int i = 0; i < 4; i++) {  // Loop through columns via MUX1
      int ch1 = ch1_list[i];
      selectChannel(CD4067_1_S0, CD4067_1_S1, CD4067_1_S2, CD4067_1_S3, ch1);
      delayMicroseconds(500);  // Stabilization delay

      float voltage = readAverageADC();  // Read average voltage
      matrix[i][ch2 - 1] = voltage;      // Store in matrix
    }
  }

  Serial.println("Matrix updated:");
  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 4; j++) {
      Serial.print(matrix[i][j], 4);  // Print 4 decimal places
      if (j < 3) Serial.print(",");
    }
    Serial.println();
  }

  delay(SCAN_DELAY);  // Wait before next scan
}

// Selects a specific channel on CD74HC4067
void selectChannel(int s0, int s1, int s2, int s3, int channel) {
  int ch = channel - 1;  // Convert to 0-based index
  digitalWrite(s0, ch & 0x01);
  digitalWrite(s1, (ch >> 1) & 0x01);
  digitalWrite(s2, (ch >> 2) & 0x01);
  digitalWrite(s3, (ch >> 3) & 0x01);
}

// Read ADC multiple times and return the average voltage
float readAverageADC() {
  float sum = 0;
  for (int i = 0; i < SAMPLES_PER_CHANNEL; i++) {
    sum += analogRead(ADC_INPUT) * (3.3 / 4095.0);  // Convert raw value to voltage
    delayMicroseconds(10);  // Small delay between samples
  }
  return sum / SAMPLES_PER_CHANNEL;
}
```

---

## Example Output

```
Matrix updated:
0.3452,0.4280,0.9125,1.2031
0.2231,0.3785,0.8347,1.1010
0.2150,0.3999,0.8524,1.0903
0.1908,0.4103,0.8701,1.0765
```

Each value represents the **voltage** measured at a sensor intersection.

---

## Notes

- `selectChannel()` uses bit masking to control CD74HC4067's 4-bit address.
- `readAverageADC()` improves stability by averaging multiple ADC reads.
- Be sure your wiring matches the GPIO definitions.
- If your matrix is larger (e.g. 8x8), you can extend the logic with more MUX or higher scanning loops.
