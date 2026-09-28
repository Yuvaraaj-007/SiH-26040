# Smart Water Purification & Quality Monitoring System

**Smart India Hackathon 2026 · Team_7 · SiH-26040**

An off-grid water purification unit that senses water quality, classifies the type of contamination on-device, and routes water only through the treatment stages it actually needs. Designed for mining-affected areas, where contamination (acidic drainage, heavy metals, suspended silt) varies by source and season.

## Problem with conventional units
- Fixed treatment pipeline regardless of contamination type
- No real-time water-safety visibility for the community
- Heavy-metal runoff from mining often goes undetected
- High downtime, with no early warning of filter fouling

## Our solution
| Feature | What it does |
|---|---|
| On-device classification | Classifies contamination type (clean, silt, heavy metal, acidic drainage), not just threshold breach |
| Adaptive routing | Activates only the treatment stages needed per batch |
| Electrochemical heavy-metal sensing | LMP91000 AFE with a 3-electrode cell, without lab-grade equipment |
| Closed-loop verification | Treated water is re-checked; a failed batch is diverted, not stored |
| Village telemetry | LoRa to a village-level dashboard, with GSM/SMS fallback |
| Predictive fouling alerts | Filter-fouling warnings from treatment-efficiency trend |

## How it works
1. **Sense** – pH, TDS, turbidity, ORP, heavy metals, temperature and flow.
2. **Classify** – the contamination type is determined on the ESP32-S3.
3. **Treat** – valves, pump and UV are set for that contamination type.
4. **Verify** – the treated water is re-tested before storage.
5. **Report** – readings and alerts go to the dashboard.

## Prototype
- Sensor cluster + control board in a sealed IP65 enclosure
- Treatment train: sediment → carbon → ion-exchange/electrocoagulation → UV
- Solar + LiFePO4 battery for off-grid operation
- LoRa telemetry, with GSM/SMS fallback

## Repository structure
| Folder | Contents |
|---|---|
| [`PPT/`](PPT) | Hackathon presentation |
| [`Smart_water_purifier/`](Smart_water_purifier) | Hardware design documentation and schematics overview |
| [`code/`](code) | Firmware and dashboard source |
| [`code/Main program/`](code/Main%20program) | ESP32-S3 firmware (ESP-IDF, C, FreeRTOS) |
| [`code/Dashboard/`](code/Dashboard) | React + TypeScript monitoring dashboard (MQTT) |

## Status
The classifier in the firmware is currently rule-based. The TensorFlow Lite Micro model, LoRa/GSM transmission, the reject-line solenoid and sensor calibration are still to be completed (see [`code/Main program`](code/Main%20program)).

## Team
Team_7 – Smart India Hackathon 2026
