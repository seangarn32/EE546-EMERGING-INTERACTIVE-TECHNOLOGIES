# ESP32 + Dual CD74HC4067 Multiplexer Matrix Scan

This project demonstrates how to use an **ESP32** and two **CD74HC4067 16-channel analog multiplexers** to scan a 4×4 sensor matrix. It reads analog voltages at each intersection and prints a voltage matrix via serial.

---

## Table of Contents
- [Arduino IDE Setup and Driver Download](#arduino-ide-setup-and-driver-download)
- [Hardware Overview](#hardware-overview)
- [Core Idea](#core-idea)
- [Define GPIO Pins for Multiplexer Control](#define-gpio-pins-for-multiplexer-control)
- [Define ADC Input and Sampling Parameters](#define-adc-input-and-sampling-parameters)
- [Setup Function: Configure Pin Modes](#setup-function-configure-pin-modes)
- [Function: Select a Channel on a MUX](#function-select-a-channel-on-a-mux)
- [Function: Read and Average ADC Voltage](#function-read-and-average-adc-voltage)
- [Loop Function: Scan the 4×4 Matrix](#loop-function-scan-the-44-matrix)
- [Serial Output: Print Matrix Values](#serial-output-print-matrix-values)
- [Example Output](#example-output)
- [Notes](#notes)

---

## Arduino IDE Setup and Driver Download
- The ESP32-WROOM-32 is a powerful Wi-Fi and Bluetooth microcontroller module. The Arduino IDE provides a simple and user-friendly programming environment that allows developers to quickly write, upload, and debug code using Arduino-style syntax. By installing the ESP32 board support package in the Arduino IDE, users can easily access hardware features such as GPIO, Wi-Fi, and ADC.

- See: "How to Set Up ESP32-WROOM-32" by Samuel Adesola on Medium: https://samueladesola.medium.com/how-to-set-up-esp32-wroom-32-b2100060470c

- Choose **ESP32 Dev Module** from **Tools** - **Board** and also ensure the **Port** is connected to **COM?**.


<img src="https://github.com/user-attachments/assets/66b91360-febd-4be3-821f-a8979537f125" width="900" height="500">


- You may need to hold down the **boot** button on the board so the code will be uploaded. When 10%, 20%, 30%, ... appear, you can release the button.

---

## Hardware Overview

- **ESP  WROOM 32 Development Board**
- **CD74HC4067 x2** (MUX1 for columns, MUX2 for rows)
- **Analog pressure sensor matrix**
- **Example wiring**: 
  - `MUX1` S0–S3 → GPIO33, 25, 26, 27  
  - `MUX2` S0–S3 → GPIO21, 22, 23, 14  
  - Output signal → `ADC_INPUT = GPIO36`

---

## Core Idea

This project uses **row-column multiplexing** to access each sensor point individually with only one ADC pin.  
The code selects a row (via MUX2), then scans each column (via MUX1), and reads voltage at the intersection using `analogRead()`.

---

## **Define GPIO Pins for Multiplexer Control**

Each CD74HC4067 multiplexer has 4 address pins (S0–S3). We use two MUX breakout boards: one for **rows**, one for **columns**.

```cpp
#define CD4067_1_S0  33
#define CD4067_1_S1  25
#define CD4067_1_S2  26
#define CD4067_1_S3  27

#define CD4067_2_S0  21
#define CD4067_2_S1  22
#define CD4067_2_S2  23
#define CD4067_2_S3  14
```

>  `CD4067_1_` is for **columns**, `CD4067_2_` is for **rows**.  
You can modify these GPIO values depending on your ESP32 pin connections.

---

## **Define ADC Input and Sampling Parameters**

We read analog voltage values from the matrix using a single ADC channel. We will take the average of three sampled voltages as the effective value for one sensing point.

```cpp
#define ADC_INPUT 36               // ADC1_CH0 on ESP32
#define SAMPLES_PER_CHANNEL 3     // Take 3 readings per point
#define SCAN_DELAY 100            // Delay (ms) between matrix scans
```

>  `GPIO36` (ADC1 channel 0) is a commonly used analog pin on the ESP32.

---

## **Setup Function: Configure Pin Modes**

This function runs once at boot. It sets up serial communication and declares all MUX address pins as outputs.

```cpp
void setup() {
  Serial.begin(250000);  // High-speed debug output

  // Initialize all address pins as OUTPUT
  pinMode(CD4067_1_S0, OUTPUT);
  pinMode(CD4067_1_S1, OUTPUT);
  pinMode(CD4067_1_S2, OUTPUT);
  pinMode(CD4067_1_S3, OUTPUT);

  pinMode(CD4067_2_S0, OUTPUT);
  pinMode(CD4067_2_S1, OUTPUT);
  pinMode(CD4067_2_S2, OUTPUT);
  pinMode(CD4067_2_S3, OUTPUT);

  Serial.println("Initialization complete, starting channel scan...");
}
```

>  Always initialize GPIOs before using them to control hardware.

---

## **Function: Select a Channel on a MUX**

The **CD74HC4067** is a 16-channel analog/digital multiplexer. It allows one signal to be routed from one of **16 different input/output lines (channels 0–15)** to a single common pin (usually connected to ADC or signal output).

This function `selectChannel(...)` sets the address lines **S0 to S3** of the MUX to select a specific channel using **bitwise operations**.

### Function Definition

```cpp
void selectChannel(int s0, int s1, int s2, int s3, int channel) {
  int ch = channel - 1;  // Convert to 0-based index (if input is 1–16)
  digitalWrite(s0, ch & 0x01);           // Write least significant bit to S0
  digitalWrite(s1, (ch >> 1) & 0x01);    // Shift right by 1, write to S1
  digitalWrite(s2, (ch >> 2) & 0x01);    // Shift right by 2, write to S2
  digitalWrite(s3, (ch >> 3) & 0x01);    // Shift right by 3, write to S3
}
```

### How It Works

Each CD74HC4067 channel is selected using a **4-bit binary address**:
- S0 is the least significant bit (LSB)
- S3 is the most significant bit (MSB)

To select a channel, the binary representation of the channel number is sent to these pins.

If the `channel` input is from **1 to 16**, we first subtract 1 (`ch = channel - 1`) to match the chip's **0-based indexing** (0–15).

---

### Example 1: Select Channel 3 on MUX1

```cpp
selectChannel(33, 25, 26, 27, 3);
```

#### Step-by-step:
- `channel = 3` → `ch = 2` (after subtracting 1)
- `ch = 2` in binary = `0010`
  - S3 = 0 → `digitalWrite(27, 0)`
  - S2 = 0 → `digitalWrite(26, 0)`
  - S1 = 1 → `digitalWrite(25, 1)`
  - S0 = 0 → `digitalWrite(33, 0)`

---

### Example 2: Select Channel 14

```cpp
selectChannel(33, 25, 26, 27, 14);
```

- `channel = 14` → `ch = 13`
- `13` in binary = `1101`
  - S3 = 1 → `digitalWrite(27, 1)`
  - S2 = 1 → `digitalWrite(26, 1)`
  - S1 = 0 → `digitalWrite(25, 0)`
  - S0 = 1 → `digitalWrite(33, 1)`

---

### Why Use Bitwise Operations?

Bitwise operations (`&`, `>>`) efficiently extract individual bits from a binary number:
- `(ch >> n) & 0x01` gets the *n-th bit* (from LSB to MSB)
- Avoids conditional `if` statements and makes the code fast and compact

---

## **Function: Read and Average ADC Voltage**

To reduce noise, we read the analog input 3 times and return the average.

```cpp
float readAverageADC() {
  float sum = 0;
  for (int i = 0; i < SAMPLES_PER_CHANNEL; i++) {
    sum += analogRead(ADC_INPUT) * (3.3 / 4095.0);  // Convert raw ADC to volts
    delayMicroseconds(10);
  }
  return sum / SAMPLES_PER_CHANNEL;
}
```

>  Converts raw `analogRead()` value (0–4095) to real-world voltage (ADC has 12 bits)

---

## **Loop Function: Scan the 4×4 Matrix**

The main `loop()` controls the scanning logic:  
- Iterate over **rows** (via MUX2)
- For each row, iterate over **columns** (via MUX1)
- Read voltage at the intersection and store it in a matrix

```cpp
 float matrix[4][4];  // Create a 4x4 array to store the sensor voltage matrix

  for (int ch2 = 1; ch2 <= 4; ch2++) {  // Loop through rows 1 to 4 (MUX2)
    selectChannel(CD4067_2_S0, CD4067_2_S1, CD4067_2_S2, CD4067_2_S3, ch2);  // Select the current row
    delayMicroseconds(500);  // Allow signal to settle

    int ch1_list[] = {1, 2, 3, 4};  // Define column numbers to scan

    for (int i = 0; i < 4; i++) {  // Loop through columns 1 to 4 (MUX1)
      int ch1 = ch1_list[i];  // Get the column index
      selectChannel(CD4067_1_S0, CD4067_1_S1, CD4067_1_S2, CD4067_1_S3, ch1);  // Select the current column
      delayMicroseconds(500);  // Allow signal to settle

      float voltage = readAverageADC();  // Read average voltage from sensor
      matrix[i][ch2 - 1] = voltage;  // Store the voltage in matrix at [column][row]
    }
  }
```

>  Matrix is filled in `[column][row]` order to match sensor layout.

---

## **Serial Output: Print Matrix Values**

After scanning the matrix, we print all voltage values in a readable format.

```cpp
  Serial.println("Matrix updated:");  // Output message

  for (int i = 0; i < 4; i++) {  // Loop through rows
    for (int j = 0; j < 4; j++) {  // Loop through columns
      Serial.print(matrix[i][j], 4);  // Print voltage with 4 decimal digits
      if (j < 3) Serial.print(",");  // Print comma except after last element
    }
    Serial.println();  // Move to next line after each row
  }

  delay(SCAN_DELAY);  // Wait before scanning matrix again
}
```

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

- `selectChannel()` uses bitwise operations, including shifting and masking, to set the 4-bit address lines (S0–S3) of the multiplexer.
- `readAverageADC()` improves stability by averaging multiple ADC reads.
- Be sure your wiring matches the GPIO definitions.
- If your matrix is larger (e.g. 8x8), you can extend the logic with more MUX or higher scanning loops.

## Reference
Adesola, S. (n.d.). How to set up ESP32 WROOM-32. Medium. https://samueladesola.medium.com/how-to-set-up-esp32-wroom-32-b2100060470c

---
