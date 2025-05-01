# Python Real-Time Heatmap for Pressure Sensor Matrix Scanner

This Python script is a **visualization tool** for the ESP32 + CD74HC4067 sensor matrix. It listens to serial data from the ESP32, parses 4×4 voltage matrices, and renders them as a live color-coded heatmap using Matplotlib.

---
## Required Libraries

Install required libraries via pip if not already available:

```bash
pip install matplotlib pyserial numpy
```
---

## 1. Import Required Libraries

```python
import matplotlib
import serial
import numpy as np
import matplotlib.pyplot as plt
import matplotlib.colors as mcolors
import threading
import queue
import time
import matplotlib.animation as animation

matplotlib.use('TkAgg')  # Use TkAgg backend for GUI
```

- `serial`: Read data from ESP32
- `numpy`: Store matrix as a 4x4 array
- `matplotlib`: Plot heatmap and manage animation
- `queue`: Share data safely between threads
- `threading`: Run background serial reading without freezing UI

---

## 2. Serial Port Setup

```python
SERIAL_PORT = 'COM3'     # Change this based on your ESP32 port
BAUD_RATE = 250000       # Match the ESP32 code
ser = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=0.05)
```

Connect to the ESP32 using the same port and baud rate defined in your ESP32 code.

---

## 3. Heatmap Initialization

```python
cmap = mcolors.LinearSegmentedColormap.from_list("custom", ["red", "yellow", "green"])
fig, ax = plt.subplots()
matrix = np.zeros((4, 4))  # Initialize matrix
heatmap = ax.imshow(matrix, cmap=cmap, vmin=0, vmax=3.3)
cbar = plt.colorbar(heatmap, label="Voltage (V)")
```

- `cmap`: Custom colormap from red to green
- `imshow()`: Displays the 2D voltage grid
- `vmin`/`vmax`: Fix scale to 0–3.3V for consistency

---

## 4. Set X and Y Axis Labels

```python
ax.set_xticks(np.arange(4))
ax.set_yticks(np.arange(4))
ax.set_xticklabels(["CH1", "CH2", "CH3", "CH4"])
ax.set_yticklabels(["CH1", "CH2", "CH3", "CH4"]) # Match the ESP32 code channels
plt.title("4x4 Matrix Voltage Visualization")
```

Adds clear labels to indicate sensor channels (row/column positions).

---

## 5. Add Text Labels to Each Cell

```python
cell_texts = [[ax.text(j, i, "", ha="center", va="center", color="black", fontsize=12)
               for j in range(4)] for i in range(4)]
```

Creates a 4×4 array of text objects so we can display the numeric voltage values inside each heatmap cell.

---

## 6. Setup Thread-Safe Queue

```python
data_queue = queue.Queue()
```

The `queue` is used to safely transfer new matrix data from the serial thread to the GUI.

---

## 7. Update Function for Heatmap Animation

```python
def update_plot(frame):
    if not data_queue.empty():
        new_matrix = data_queue.get()
        heatmap.set_data(new_matrix)

        for i in range(4):
            for j in range(4):
                cell_texts[i][j].set_text(f"{new_matrix[i][j]:.2f}")

    plt.draw()
```

- Called every 50 ms by the animation
- Updates heatmap color and text labels with the latest data
- Uses `.set_text()` to refresh voltage numbers

---

## 8. Serial Reading Thread

```python
def read_arduino_data():
    while True:
        try:
            line = ser.readline().decode('utf-8', errors='ignore').strip()
            if "Matrix updated:" in line:
                new_matrix = np.zeros((4, 4))
                for i in range(4):
                    row_data = ser.readline().decode('utf-8', errors='ignore').strip().split(",")
                    if len(row_data) == 4:
                        new_matrix[i] = [float(val) for val in row_data]

                data_queue.put(new_matrix)
        except Exception as e:
            print(f"Serial read error: {e}")
```

- Waits for `Matrix updated:` line as a trigger
- Then reads 4 more lines, each with 4 comma-separated voltages
- Parses into NumPy array and pushes to queue

---

## 9. Start Background Thread

```python
data_thread = threading.Thread(target=read_arduino_data, daemon=True)
data_thread.start()
```

Starts the serial reader thread in the background, so the GUI stays responsive.

---

## 10. Start Animation & Show GUI

```python
ani = animation.FuncAnimation(fig, update_plot, interval=50)
plt.show()
```

- Starts real-time animation loop
- `interval=50` → 20 frames per second
- `plt.show()` keeps the window open and reactive

---
