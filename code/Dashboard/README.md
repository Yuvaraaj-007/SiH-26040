# Dashboard

React + TypeScript dashboard for village-level monitoring. It subscribes to MQTT telemetry from deployed units and shows readings, status and alerts.

## Files
| File | Purpose |
|---|---|
| `Dashboard.tsx` | Main page: broker status and one card group per device |
| `MetricCard.tsx` | Card for a single metric, colored by status |
| `AlertsPanel.tsx` | List of alerts with dismiss |
| `useMqttTelemetry.ts` | Hook that connects to the broker, tracks devices and derives alerts |
| `thresholds.ts` | Safe/warning bands and status colors |
| `types.ts` | Shared types (`SensorReading`, `Alert`, `DeviceState`, ...) |

## Metrics and thresholds
| Metric | Safe range | Warning range |
|---|---|---|
| pH | 6.5–8.5 | 5.5–9.5 |
| TDS (ppm) | 0–500 | 0–1000 |
| Turbidity (NTU) | 0–5 | 0–50 |
| ORP (mV) | −50–400 | −100–600 |
| Heavy metals (ppb) | 0–10 | 0–50 |
| Temperature (°C) | 5–35 | 0–45 |

Outside the warning range is critical. A device's overall status is its worst metric. A rejected batch always raises a critical alert.

## MQTT topics
- `water/<deviceId>/telemetry` – JSON `SensorReading`
- `water/<deviceId>/alert` – JSON `Alert`

## Setup
1. Install dependencies: `react` and `mqtt`.
2. Set `BROKER_URL` in `useMqttTelemetry.ts` to your broker's WebSocket endpoint (e.g. a Mosquitto WebSocket listener).
3. Render `Dashboard` from your app entry point.

This folder holds the components only; add it to a React/TypeScript project (e.g. Vite) to run it.
