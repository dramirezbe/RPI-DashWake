# 🚨 Embedded Linux Project – Monitoring System with Raspberry Pi and ESP32

## DashWake

- *🎓 National University of Colombia - Manizales Campus*
- *📚 Course: Embedded Linux Programming*
- *👨‍🏫 Professor:* Juan Bernardo Gomez Mendoza
- *📅 Delivery Date:* July 24, 2025
---

## 👥 Authors

- 👨‍💻 *David Ramirez Betancourth*
- 👨‍💻 *Leandro Zapata Marín*

Students of Electronic Engineering

---

This project implements an *embedded data acquisition and visualization system* using a *Raspberry Pi* and an *ESP32*, focused on monitoring an alarm system and environmental sensors.  
It provides a *web-based interface* to display the collected data and synchronizes the time via an NTP server.

---

## 🎯 Project Objective

Develop a robust and functional embedded system that enables:

- ✅ Monitoring of temperature, humidity, and air quality sensors via the ESP32  
- ✅ Detection and handling of a physical alarm button  
- ✅ Retrieval and storage of the current time using NTP request
- ✅ Real-time data visualization through a web interface served from the Raspberry Pi  
- ✅ Modular and scalable C-based architecture for easier maintenance and extensibility

---

## 🧠 How it works

🔹 The *ESP32* handles analog sensor readings via its ADC pins, including:

- 🌡 Temperature  
- 💧 Humidity  
- 🌫 Air Quality  

🔹 A *physical alarm button* is integrated and managed via the btn_handler module.

🔹 A C function uses *system* to make an *NTP system update each 10 minutes*, storing the result in a custom structure containing:

- A char[3] array for day, month, and year  
- An int[3] array for hour, minutes, and seconds

🔹 The collected information is displayed on a static web page (HTML/CSS/JS), hosted locally by the Raspberry Pi.

---

## 🗂 Repository Structure
- ├── Core/ # Embedded system logic
- │ ├── modules/ # Functional modules
- │ │ ├── btn_handler.c # Alarm button handler
- │ │ ├── btn_handler.h
- | | ├── cJSON.h # Headers compiled manually from cJSON original lib
- | | ├── cJSON.c
- │ │ ├── force_ntp_sync.c # NTP request via libcurl
- │ │ ├── force_ntp_sync.h
- │ ├── CMakeLists.txt # CMake build configuration
- │ └── main.c # Main execution logic
- ├── page/ # Web interface for data display
- │ ├── app.js # Frontend JavaScript logic
- │ ├── index.html # Main web page
- │ └── style.css # Page styling
- ├── .gitignore # Git ignored files
- ├── README.md # This file
- ├── build-core.sh # Build script for core
- ├── install-dashwake.sh # Installation/deployment script

---

## ⚙ Technologies Used

- 🐧 *Embedded Linux* on Raspberry Pi  
- 📡 *ESP32* for sensor acquisition    
- 🧰 *C / CMake*  
- 🖥 *Frontend*: HTML + CSS + JavaScript  

---

## 🚀 How to Run

1. 🔧 Build the core project:

```bash
./build-core.sh
./install-dashwake.sh
```