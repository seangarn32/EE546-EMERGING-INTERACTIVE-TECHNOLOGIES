# Python Real-Time Heatmap for Pressure Sensor Matrix Scanner

This Python script is a **visualization tool** for the ESP32 + CD74HC4067 sensor matrix. It listens to serial data from the ESP32, parses 4×4 voltage matrices, and renders them as a live color-coded heatmap using Matplotlib.

---

## Table of Contents

- [1. Required Libraries](#1-required-libraries)
- [2. Import Required Libraries](#2-import-required-libraries)
- [3. Serial Port Setup](#3-serial-port-setup)
- [4. Heatmap Initialization](#4-heatmap-initialization)
- [5. Set X and Y Axis Labels](#5-set-x-and-y-axis-labels)
- [6. Add Text Labels to Each Cell](#6-add-text-labels-to-each-cell)
- [7. Setup Thread-Safe Queue](#7-setup-thread-safe-queue)
- [8. Update Function for Heatmap Animation](#8-update-function-for-heatmap-animation)
- [9. Serial Reading Thread](#9-serial-reading-thread)
- [10. Start Background Thread](#10-start-background-thread)
- [11. Start Animation & Show GUI](#11-start-animation--show-gui)

---


## 1. Required Libraries

Install required libraries via pip if not already available:

```bash
pip install matplotlib pyserial numpy
```
---

## 2. Import Required Libraries

```
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

## 3. Serial Port Setup

```
SERIAL_PORT = 'COM3'     # Change this based on your ESP32 port
BAUD_RATE = 250000       # Match the ESP32 code
ser = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=0.05)
```

Connect to the ESP32 using the same port and baud rate defined in your ESP32 code.

---

## 4. Heatmap Initialization

```
cmap = mcolors.LinearSegmentedColormap.from_list("custom", ["red", "yellow", "green"])  # Create a red-yellow-green gradient color map
fig, ax = plt.subplots()                   # Create the plot figure and axes
matrix = np.zeros((4, 4))                  # Initialize an empty 4x4 matrix
heatmap = ax.imshow(matrix, cmap=cmap, vmin=0, vmax=3.3)  # Plot the heatmap with defined colormap and voltage range
cbar = plt.colorbar(heatmap, label="Voltage (V)")         # Add a colorbar legend to the side

```

- `cmap`: Custom colormap from red to green
- `imshow()`: Displays the 2D voltage grid
- `vmin`/`vmax`: Fix scale to 0–3.3V for consistency

---

## 5. Set X and Y Axis Labels

```
ax.set_xticks(np.arange(4))                             # Set X-axis ticks from 0 to 3
ax.set_yticks(np.arange(4))                             # Set Y-axis ticks from 0 to 3
ax.set_xticklabels(["CH1", "CH2", "CH3", "CH4"])        # Label columns as CH1–CH4
ax.set_yticklabels(["CH1", "CH2", "CH3", "CH4"])        # Label rows as CH1–CH4
plt.title("4x4 Matrix Voltage Visualization")           # Add a title to the plot
```

Adds clear labels to indicate sensor channels (row/column positions).

---

## 6. Add Text Labels to Each Cell

```
cell_texts = [[ax.text(j, i, "", ha="center", va="center", color="black", fontsize=12)
               for j in range(4)] for i in range(4)]
```

Creates a 4×4 array of text objects so we can display the numeric voltage values inside each heatmap cell.

---

## 7. Setup Thread-Safe Queue

```
data_queue = queue.Queue()
```

The `queue` is used to safely transfer new matrix data from the serial thread to the GUI.

---

## 8. Update Function for Heatmap Animation

```
def update_plot(frame):                               # This function runs every 50 ms from the animation timer
    if not data_queue.empty():                        # If new matrix data is available
        new_matrix = data_queue.get()                 # Retrieve the newest matrix from the queue
        heatmap.set_data(new_matrix)                  # Update heatmap color data

        for i in range(4):                            # Loop over rows
            for j in range(4):                        # Loop over columns
                cell_texts[i][j].set_text(f"{new_matrix[i][j]:.2f}")  # Update cell text with 2 decimal places

    plt.draw()                                        # Redraw the figure
```

- Called every 50 ms by the animation
- Updates heatmap color and text labels with the latest data
- Uses `.set_text()` to refresh voltage numbers

---

## 9. Serial Reading Thread

```
def read_arduino_data():                                           # Define a background function for continuous serial reading
    while True:                                                    # Infinite loop
        try:
            line = ser.readline().decode('utf-8', errors='ignore').strip()  # Read a line from serial, decode it
            if "Matrix updated:" in line:                          # If the trigger line is received
                new_matrix = np.zeros((4, 4))                      # Initialize new matrix to fill

                for i in range(4):                                 # Expect 4 lines of row data
                    row_data = ser.readline().decode('utf-8', errors='ignore').strip().split(",")  # Read and split by comma
                    if len(row_data) == 4:                         # Ensure exactly 4 values per row
                        new_matrix[i] = [float(val) for val in row_data]  # Convert strings to floats and store

                data_queue.put(new_matrix)                         # Push the matrix into the data queue
        except Exception as e:                                     # If there's an error, print it
            print(f"Serial read error: {e}")                       # Print error to console
```

- Waits for `Matrix updated:` line as a trigger
- Then reads 4 more lines, each with 4 comma-separated voltages
- Parses into NumPy array and pushes to queue

---

## 10. Start Background Thread

```
data_thread = threading.Thread(target=read_arduino_data, daemon=True)  # Create a daemon thread that runs serial reading
data_thread.start()                                                    # Start the thread
```

Starts the serial reader thread in the background, so the GUI stays responsive.

---

## 11. Start Animation & Show GUI

```
ani = animation.FuncAnimation(fig, update_plot, interval=50)  # Start the animation; update every 50ms (20Hz)
plt.show()                                                     # Show the matplotlib GUI window
```

- Starts real-time animation loop
- `interval=50` → 20 frames per second
- `plt.show()` keeps the window open and reactive

---
