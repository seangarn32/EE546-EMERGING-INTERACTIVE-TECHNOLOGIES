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

void selectChannel(int s0, int s1, int s2, int s3, int channel) {
  int ch = channel - 1;  // Convert to 0-based index (if input is 1–16)
  digitalWrite(s0, ch & 0x01);           // Write least significant bit to S0
  digitalWrite(s1, (ch >> 1) & 0x01);    // Shift right by 1, write to S1
  digitalWrite(s2, (ch >> 2) & 0x01);    // Shift right by 2, write to S2
  digitalWrite(s3, (ch >> 3) & 0x01);    // Shift right by 3, write to S3
}

float readAverageADC() {
  float sum = 0;
  for (int i = 0; i < SAMPLES_PER_CHANNEL; i++) {
    sum += analogRead(ADC_INPUT) * (3.3 / 4095.0);  // Convert raw ADC to volts
    delayMicroseconds(10);
  }
  return sum / SAMPLES_PER_CHANNEL;
}

void loop() {
  float matrix[5][3];  // Create a 4x4 array to store the sensor voltage matrix

  for (int ch1 = 1; ch1 <= 5; ch1++) {  // Loop through rows 1 to 5 (MUX1)
    selectChannel(CD4067_2_S0, CD4067_2_S1, CD4067_2_S2, CD4067_2_S3, ch1);  // Select the current row
    delayMicroseconds(500);  // Allow signal to settle

    for (int ch2 = 1; ch2 <= 3; ch2++) {  // Loop through columns 1 to 3 (MUX2)
      selectChannel(CD4067_1_S0, CD4067_1_S1, CD4067_1_S2, CD4067_1_S3, ch2);  // Select the current column
      delayMicroseconds(500);  // Allow signal to settle

      float voltage = readAverageADC();  // Read average voltage from sensor
      matrix[ch1 - 1][ch2 - 1] = voltage;
    }
  }

  Serial.println("Matrix updated:");  // Output message

  for (int i = 0; i < 5; i++) {  // Loop through rows
    for (int j = 0; j < 3; j++) {  // Loop through columns
      Serial.print(matrix[i][j], 4);  // Print voltage with 4 decimal digits
      if (j < 2) Serial.print(",");  // Print comma except after last element
    }
    Serial.println();  // Move to next line after each row
  }

  delay(SCAN_DELAY);  // Wait before scanning matrix again
}


