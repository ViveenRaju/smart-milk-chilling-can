# Smart Hybrid Milk-Chilling and Quality-Monitoring Can

🔗 [Live Demo](https://chilling-can.web.app/)

### SIH Project Prototype

A portable smart milk-chilling system designed to help maintain milk quality during collection and transportation by combining **PCM-based passive cooling**, **Peltier-assisted active cooling**, and **real-time quality monitoring**.

---

## Overview

Freshly collected milk can reach temperatures of around **30–35°C**, creating favorable conditions for microbial growth and quality deterioration.

The proposed system is designed to rapidly reduce milk temperature toward the **4–8°C safe range** and maintain suitable conditions during transportation.

The system integrates:

- Passive cooling using **Phase Change Material (PCM)**
- Active cooling using a **Peltier thermoelectric module**
- **ESP32-based monitoring and control**
- Real-time **temperature and pH monitoring**
- **Firebase Realtime Database** connectivity
- Web-based monitoring dashboard
- Temperature-exposure and milk-quality risk classification

---

## Key Features

### Hybrid Cooling

Combines PCM-based passive cooling with Peltier-assisted active cooling to reduce temperature efficiently while minimizing continuous power consumption.

### Temperature Monitoring

Continuously monitors milk temperature and identifies whether the milk remains within the target **4–8°C range**.

### pH-Based Quality Monitoring

Monitors milk pH to provide an additional indicator of freshness and potential quality deterioration.

### Smart Risk Classification

Uses temperature, pH, and temperature-exposure information to classify the milk condition and generate alerts.

### IoT Connectivity

ESP32 sends monitoring data to **Firebase Realtime Database**, allowing the web dashboard to display live information.

### Portable Design

Designed as a compact milk-chilling can suitable for milk collection and transportation in locations where conventional bulk milk chillers may not be practical.

---

## System Architecture

```text
                    SMART MILK CHILLING CAN
                              │
          ┌───────────────────┼───────────────────┐
          │                   │                   │
    Temperature           pH Sensor         Cooling System
       Sensor                                  │
                                               │
                                      ┌────────┴────────┐
                                      │                 │
                                   Peltier             PCM
                               Active Cooling    Passive Cooling
                                      │                 │
                                      └────────┬────────┘
                                               │
                                             ESP32
                                               │
                                               ▼
                                    Firebase Realtime DB
                                               │
                                               ▼
                                        Web Dashboard
                                               │
                           ┌───────────────────┼───────────────────┐
                           │                   │                   │
                      Temperature              pH              Risk Status
                       Monitoring          Monitoring          Classification
```

---

## Hardware Components

| Component | Purpose |
|---|---|
| ESP32 | Main controller and IoT communication |
| Temperature Sensor | Measures milk temperature |
| pH Sensor | Monitors milk pH |
| Peltier Module | Provides active cooling assistance |
| PCM | Provides passive cooling and thermal storage |
| Insulation | Reduces external heat transfer |
| Battery | Powers the electronics and active cooling |
| Milk Can | Main storage and transportation container |

---

## Cooling Concept

The system uses a hybrid cooling approach.

### PCM-Based Passive Cooling

PCM is used as the primary thermal storage medium. The PCM absorbs heat from the milk while undergoing a phase change, allowing cooling to continue without continuous electrical power.

The PCM tray can be removed and recharged before reuse.

### Peltier-Assisted Active Cooling

The Peltier system provides active cooling assistance during the initial cooling stage and when additional cooling is required.

The control strategy reduces unnecessary continuous operation of the Peltier module, helping lower energy consumption.

### Target Temperature

```text
Milk Temperature Target
        │
        ▼
   ┌───────────┐
   │   4–8°C   │
   └───────────┘
        │
        ▼
 Suitable range for
 chilled milk transport
```

---

## IoT and Monitoring

The ESP32 collects sensor readings and sends the data to Firebase Realtime Database.

```text
Temperature Sensor ──┐
                     │
pH Sensor ───────────┼──► ESP32 ──► Firebase ──► Web Dashboard
                     │
Cooling System ──────┘
```

The dashboard provides:

- Live temperature monitoring
- Live pH monitoring
- Milk condition classification
- Temperature exposure tracking
- Cooling status
- Alerts
- Can identification
- Historical temperature information

---

## Peltier Control Logic

The prototype uses temperature-based cooling control.

| Temperature Condition | Peltier Cooling |
|---|---:|
| > 20°C | 100% |
| > 12°C | 75% |
| > 8°C | 50% |
| ≤ 8°C | 0% |

The objective is to provide stronger cooling when the milk temperature is high and reduce active cooling as the target temperature is approached.

---

## Milk Quality Monitoring

The system combines multiple parameters rather than relying only on temperature.

### Parameters Monitored

- **Temperature**
- **pH**
- **Time spent above the target temperature**

These parameters are used by the dashboard to provide a simplified milk-quality risk assessment.

```text
Temperature
     │
     ├──────────────┐
     │              │
     ▼              │
    pH ─────────────┤
     │              │
     ▼              ▼
Temperature Exposure
     │
     └──────────► Risk Assessment
                       │
                       ▼
                Milk Condition
```

---

## Firebase Data Flow

```text
ESP32
  │
  │ Sensor Data
  ▼
Firebase Realtime Database
  │
  │ Real-Time Updates
  ▼
Web Dashboard
  │
  ├── Temperature
  ├── pH
  ├── Cooling Status
  ├── Exposure
  └── Risk Classification
```

---

## Repository Structure

```text
smart-milk-chilling-can/
│
├── .gitignore
├── README.md
│
├── firmware/
│   └── milk_monitor_esp32.ino
│
└── web-dashboard/
    └── index.html
```

---

## Firmware

The ESP32 firmware is located in:

```text
firmware/milk_monitor_esp32.ino
```

The firmware is responsible for:

- Connecting the ESP32 to Wi-Fi
- Reading temperature data
- Reading pH data
- Controlling the cooling system
- Sending sensor data to Firebase
- Supporting real-time monitoring

> **Security Note:** Public repository versions should use placeholders for Wi-Fi credentials and other sensitive configuration values. Do not commit private Wi-Fi passwords, authentication credentials, or other secrets.

---

## Web Dashboard

The monitoring interface is located in:

```text
web-dashboard/index.html
```

The dashboard provides a centralized interface for monitoring milk cans and viewing real-time sensor information.

### Dashboard Functions

- Live temperature display
- pH monitoring
- Milk condition status
- Cooling status
- Temperature exposure
- Risk classification
- Alerts
- Can monitoring
- Historical temperature visualization

---

## Data Flow

```text
┌──────────────────┐
│   Milk Can       │
│                  │
│ Temperature      │
│ pH               │
│ Cooling System   │
└────────┬─────────┘
         │
         ▼
┌──────────────────┐
│      ESP32       │
│ Data Acquisition │
│ Control          │
└────────┬─────────┘
         │
         ▼
┌──────────────────┐
│ Firebase         │
│ Realtime         │
│ Database         │
└────────┬─────────┘
         │
         ▼
┌──────────────────┐
│ Web Dashboard    │
│                  │
│ Temperature      │
│ pH               │
│ Risk             │
│ Alerts           │
└──────────────────┘
```

---

## Project Status

**Current Stage:** Prototype Development

### Implemented

- ESP32-based monitoring
- Temperature sensing
- pH monitoring
- Peltier control concept
- PCM-based passive cooling concept
- Firebase integration
- Web monitoring dashboard

### Future Development

- Physical prototype optimization
- Improved thermal insulation
- PCM thermal-performance optimization
- Battery and power optimization
- Solar-assisted charging
- Field testing with real milk
- Sensor calibration and validation
- Mobile-friendly monitoring interface
- Multi-can fleet monitoring

---

## Technologies Used

### Hardware

- ESP32
- Temperature Sensor
- pH Sensor
- Peltier Thermoelectric Module
- PCM
- Battery
- Insulated Milk Can

### Software

- Arduino / ESP32
- HTML
- CSS
- JavaScript
- Firebase Realtime Database

---

## Project Objective

The objective of the Smart Hybrid Milk-Chilling and Quality-Monitoring Can is to provide a **portable, energy-conscious, and intelligent milk transportation solution** that combines thermal management with real-time quality monitoring.

The system aims to help reduce milk quality deterioration during the critical period between **milk collection and chilling/processing**.

---

## Note

This repository contains the software and firmware components of the prototype. Hardware specifications, thermal calculations, experimental results, and future design improvements may be added as the prototype progresses.

---

### Smart Hybrid Milk-Chilling and Quality-Monitoring Can

**SIH Project Prototype**
