# Source Code

| Folder | Description |
|---|---|
| [`Main program/`](Main%20program) | ESP32-S3 firmware: sensing, classification, treatment control, safety check |
| [`Dashboard/`](Dashboard) | React + TypeScript dashboard that shows live telemetry and alerts over MQTT |

## Data flow
```
Sensors → ESP32-S3 firmware → (LoRa / GSM) → MQTT broker → Dashboard
```
The firmware and dashboard share the same set of contamination classes: `CLEAN`, `SILT`, `HEAVY_METAL`, `ACIDIC_DRAINAGE`, `UNKNOWN_LOW_CONFIDENCE`.
