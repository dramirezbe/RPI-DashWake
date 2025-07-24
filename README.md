# 🌐 RPI-DashWake

RPI-DashWake is a modular dashboard system developed for *Raspberry Pi*, designed to provide user-friendly access to system features such as network control, sensor readings, and web-based interactions. It combines C-based hardware drivers, a backend server, and two web-based frontends (including a captive portal) to deliver a full-stack Linux application for embedded environments.

---

## 📁 Project Structure

```bash
RPI-DashWake/
├── Core/
│ ├── CMakeLists.txt
│ ├── main.c
│ └── Modules/
│ ├── btn_handler.c/h # Button input driver
│ ├── dht11Driver.c/h # DHT11 sensor driver (deprecated)
│ ├── webHandler.c/h # UART/web communication handler
├── backend/
│ ├── package.json
│ ├── pnpm-lock.yaml
│ └── server.js # Express server for backend APIs
├── captive_portal/
│ ├── public/
│ │ ├── favicon.ico
│ │ └── logo.png
│ ├── src/
│ │ ├── assets/
│ │ ├── pages/
│ │ ├── App.tsx
│ │ ├── main.tsx
│ │ └── ...
│ └── vite.config.ts
├── frontend/
│ ├── public/
│ ├── src/
│ ├── index.html
│ └── vite.config.ts
├── .gitignore
├── README.md
├── build-core.sh # Shell script to compile Core
└── install-dashwake.sh # Setup script for the full system
```

---

## ⚙ Features

- 🔌 *Modular C core* for sensor and button handling.
- 🌐 *Captive portal* for initial network configuration.
- 📡 *Backend server* for handling requests and interfacing with the system.
- 🖥 *Frontend dashboard* for user interaction and data visualization.
- 🧪 UART communication to external microcontrollers for extended sensor support.

---

## 🚀 Installation

Clone the repository:

```bash
git clone https://github.com/dramirezbe/RPI-DashWake.git
cd RPI-DashWake
```

❌ DHT11 Sensor Module
The dht11Driver.c module could not reliably read values from the DHT11 sensor on Raspberry Pi GPIOs. After multiple debugging attempts, it was determined that timing issues and kernel-level constraints on the Pi make the use of this sensor unreliable using software-level GPIO toggling.

Solution:
Sensor readings are now handled using an ESP32 or Arduino and transmitted to the Raspberry Pi via UART. This setup has proven more stable and reliable.

We have kept the original DHT11 module in the repo for reference and possible future fixes.