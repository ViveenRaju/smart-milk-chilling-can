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

┌─────────────────────────────────────────────────────────────┐
│              SMART MILK CHILLING CAN                        │
└─────────────────────────────┬───────────────────────────────┘
                              │
              ┌───────────────┼───────────────┐
              │               │               │
        Temperature        pH Sensor     Cooling System
          Sensor                              │
              │                         ┌─────┴─────┐
              │                         │           │
              │                      Peltier       PCM
              │                    Initial         Passive
              │                    Cooling        Cooling
              │
              └───────────────┬───────────────┘
                              │
                           ESP32
                              │
                              ▼
                    Firebase Realtime DB
                              │
                              ▼
                       Web Dashboard
                              │
                 ┌────────────┼────────────┐
                 │            │            │
              Temperature     pH        Risk Status
               Monitoring  Monitoring  Classification

