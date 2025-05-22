#include "BluetoothSerial.h"

// CD4067 multiplexer control pins
#define CD4067_2_S0  33
#define CD4067_2_S1  25
#define CD4067_2_S2  26
#define CD4067_2_S3  27

#define CD4067_1_S0  21
#define CD4067_1_S1  22
#define CD4067_1_S2  23
#define CD4067_1_S3  14

#define ADC_INPUT 36               // ADC1_CH0 on ESP32
#define SAMPLES_PER_CHANNEL 3     // Take 3 readings per point
#define SCAN_DELAY 100            // Delay (ms) between matrix scans

BluetoothSerial SerialBT;  // Bluetooth Serial object

void setup() {
  Serial.begin(250000);             // USB serial debug
  SerialBT.begin("ESP32_BT_Matrix");  // Bluetooth device name
  Serial.println("Bluetooth device is ready to pair");
  SerialBT.println("Bluetooth device is ready to pair");

  // Initialize multiplexer control pins
  pinMode(CD4067_1_S0, OUTPUT);
  pinMode(CD4067_1_S1, OUTPUT);
  pinMode(CD4067_1_S2, OUTPUT);
  pinMode(CD4067_1_S3, OUTPUT);

  pinMode(CD4067_2_S0, OUTPUT);
  pinMode(CD4067_2_S1, OUTPUT);
  pinMode(CD4067_2_S2, OUTPUT);
  pinMode(CD4067_2_S3, OUTPUT);

  Serial.println("Initialization complete, starting channel scan...");
  SerialBT.println("Initialization complete, starting channel scan...");
}

void selectChannel(int s0, int s1, int s2, int s3, int channel) {
  int ch = channel - 1;
  digitalWrite(s0, ch & 0x01);
  digitalWrite(s1, (ch >> 1) & 0x01);
  digitalWrite(s2, (ch >> 2) & 0x01);
  digitalWrite(s3, (ch >> 3) & 0x01);
}

float readAverageADC() {
  float sum = 0;
  for (int i = 0; i < SAMPLES_PER_CHANNEL; i++) {
    sum += analogRead(ADC_INPUT) * (3.3 / 4095.0);
    delayMicroseconds(10);
  }
  return sum / SAMPLES_PER_CHANNEL;
}

void loop() {
  float matrix[5][3];

  for (int ch1 = 1; ch1 <= 5; ch1++) {
    selectChannel(CD4067_2_S0, CD4067_2_S1, CD4067_2_S2, CD4067_2_S3, ch1);
    delayMicroseconds(500);

    for (int ch2 = 1; ch2 <= 3; ch2++) {
      selectChannel(CD4067_1_S0, CD4067_1_S1, CD4067_1_S2, CD4067_1_S3, ch2);
      delayMicroseconds(500);

      float voltage = readAverageADC();
      matrix[ch1 - 1][ch2 - 1] = voltage;
    }
  }

  Serial.println("Matrix updated:");
  SerialBT.println("Matrix updated:");

  for (int i = 0; i < 5; i++) {
    String row = "";
    for (int j = 0; j < 3; j++) {
      row += String(matrix[i][j], 4);
      if (j < 2) row += ",";
    }
    Serial.println(row);
    SerialBT.println(row);
  }

  delay(SCAN_DELAY);
}
