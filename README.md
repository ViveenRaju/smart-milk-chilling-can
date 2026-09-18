\# Smart Hybrid Milk-Chilling and Quality-Monitoring Can



\## SIH Project Prototype



A portable smart milk-chilling system designed to support rapid cooling and continuous quality monitoring during milk collection and transportation.



\## Overview



The system combines passive PCM-based cooling with short-duration Peltier-assisted cooling during the milk loading phase. An ESP32 collects temperature and pH data and sends telemetry to a Firebase Realtime Database. A web dashboard provides collection-centre personnel with real-time visibility of milk-can conditions.



\## Key Features



\- PCM-based passive cooling for extended temperature retention

\- Peltier-assisted cooling during the initial loading phase

\- ESP32-based temperature and pH monitoring

\- Firebase Realtime Database connectivity

\- Web-based collection-centre monitoring dashboard

\- Temperature history visualization

\- Time-temperature exposure monitoring

\- Milk condition classification as Safe, Watch, or Danger

\- Local and browser-based alerts

\- Can identification and QR-based linking interface



\## System Architecture



```text

&#x20;                SMART MILK CHILLING CAN

&#x20;                        │

&#x20;       ┌────────────────┼────────────────┐

&#x20;       │                │                │

&#x20;  Temperature        pH Sensor       Cooling System

&#x20;    Sensor                              │

&#x20;       │                         ┌──────┴──────┐

&#x20;       │                         │             │

&#x20;       │                      Peltier         PCM

&#x20;       │                      Loading       Transport

&#x20;       │                       Phase          Phase

&#x20;       │

&#x20;       └──────────────┬───────────────┐

&#x20;                      │

&#x20;                    ESP32

&#x20;                      │

&#x20;                      ▼

&#x20;             Firebase Realtime DB

&#x20;                      │

&#x20;                      ▼

&#x20;               Web Dashboard

&#x20;                      │

&#x20;            ┌─────────┼─────────┐

&#x20;            │         │         │

&#x20;         Temp        pH       Risk

&#x20;       Monitoring Monitoring Classification

