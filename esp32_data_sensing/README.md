# 🌡⛽ DHT11 & MQ-7 Sensor Data Logger with ESP32

This module reads temperature 🌡, humidity 💧, and carbon monoxide levels 🏭 using the *DHT11* and *MQ-7* sensors connected to an *ESP32* board. The sensor values are printed over the Serial Monitor for real-time monitoring.

---

## 📁 File

- dht11_mq7_data_logger.ino – Main Arduino sketch for the project.

---

## 🔧 Hardware Requirements

- 🧠 *ESP32* microcontroller
- 🌡 *DHT11* Temperature and Humidity sensor
- 🏭 *MQ-7* Carbon Monoxide (CO) sensor
- 🔌 Jumper wires
- 🪛 Breadboard or PCB (optional)
- 💻 USB cable for programming the ESP32

---

## 📌 Pin Configuration

| Component | ESP32 Pin |
|-----------|-----------|
| DHT11     | GPIO 27   |
| MQ-7 (Analog) | GPIO 35 (ADC1_CH7) |

---

## 🧠 How It Works

1. Initializes the DHT11 sensor on GPIO 27.
2. Reads temperature (°C) and humidity (%) every second.
3. Reads analog values from the MQ-7 sensor on GPIO 35 to estimate CO levels.
4. Prints all values in a structured format.

---

## ⚠ Notes

- The *MQ-7 sensor* requires a warm-up time (~24–48 hours for calibration) for reliable CO detection.
- The *DHT11* has limited accuracy and a slow update rate (recommended ≥1s delay between reads).
- Ensure the ESP32 analog pin supports ADC (e.g., GPIO 35 is ADC1_CH7 and works well).
- Use a pull-up resistor on the DHT11 data line if necessary.

---

## 🚀 Getting Started

1. Open dht11_mq7_data_catch.ino in the Arduino IDE.
2. Select the correct *ESP32 board* and *COM port*.
3. Install the *DHT sensor library* (from Adafruit or Arduino Library Manager).
4. Upload the code and open the *Serial Monitor* at 9600 baud.

---

## 👨‍💻 Authors

Developed by:
- *Leandro Zapata Marín*  
- *David Ramírez Betancourt*  
🎓 Students of *Electronic Engineering* at the *National University of Colombia*