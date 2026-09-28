import { useEffect, useRef, useState, useCallback } from "react";
import mqtt, { MqttClient } from "mqtt";
import { Alert, DeviceState, SensorReading } from "./types";
import { getOverallStatus } from "./thresholds";

// Same telemetry-first architecture as Clovis: units publish retained
// telemetry over MQTT; the dashboard subscribes directly (or via a
// mqtt2prom-style bridge if you prefer polling Prometheus instead).
//
// Topic layout:
//   water/<deviceId>/telemetry   -> JSON SensorReading
//   water/<deviceId>/alert       -> JSON Alert

const BROKER_URL = "wss://your-broker-host:8081/mqtt"; // mosquitto websockets listener

interface TelemetryState {
  devices: Record<string, DeviceState>;
  alerts: Alert[];
  connected: boolean;
}

function deriveAlertsFromReading(reading: SensorReading): Alert[] {
  const alerts: Alert[] = [];
  const status = getOverallStatus(reading);

  if (reading.batchRejected) {
    alerts.push({
      id: `${reading.deviceId}-${reading.timestamp}-reject`,
      deviceId: reading.deviceId,
      metric: "batch",
      message: `Batch failed post-treatment check on ${reading.deviceId} - diverted, not stored.`,
      severity: "critical",
      timestamp: reading.timestamp,
    });
  }

  if (status === "critical") {
    alerts.push({
      id: `${reading.deviceId}-${reading.timestamp}-critical`,
      deviceId: reading.deviceId,
      metric: "batch",
      message: `${reading.deviceId} reporting unsafe water quality (class: ${reading.contaminationClass}).`,
      severity: "critical",
      timestamp: reading.timestamp,
    });
  } else if (status === "warning") {
    alerts.push({
      id: `${reading.deviceId}-${reading.timestamp}-warning`,
      deviceId: reading.deviceId,
      metric: "batch",
      message: `${reading.deviceId} readings trending outside safe range.`,
      severity: "warning",
      timestamp: reading.timestamp,
    });
  }

  return alerts;
}

export function useMqttTelemetry() {
  const [state, setState] = useState<TelemetryState>({
    devices: {},
    alerts: [],
    connected: false,
  });
  const clientRef = useRef<MqttClient | null>(null);

  useEffect(() => {
    const client = mqtt.connect(BROKER_URL, {
      reconnectPeriod: 3000,
      clean: true,
    });
    clientRef.current = client;

    client.on("connect", () => {
      setState((s) => ({ ...s, connected: true }));
      client.subscribe("water/+/telemetry");
      client.subscribe("water/+/alert");
    });

    client.on("close", () => setState((s) => ({ ...s, connected: false })));

    client.on("message", (topic, payload) => {
      const parts = topic.split("/"); // water/<deviceId>/<kind>
      const deviceId = parts[1];
      const kind = parts[2];

      try {
        const data = JSON.parse(payload.toString());

        if (kind === "telemetry") {
          const reading = data as SensorReading;
          const derivedAlerts = deriveAlertsFromReading(reading);

          setState((s) => ({
            ...s,
            devices: {
              ...s.devices,
              [deviceId]: {
                deviceId,
                latest: reading,
                connected: true,
                lastSeen: Date.now(),
              },
            },
            alerts: derivedAlerts.length
              ? [...derivedAlerts, ...s.alerts].slice(0, 50) // cap history
              : s.alerts,
          }));
        }

        if (kind === "alert") {
          const alert = data as Alert;
          setState((s) => ({ ...s, alerts: [alert, ...s.alerts].slice(0, 50) }));
        }
      } catch (err) {
        console.error("Failed to parse MQTT payload", topic, err);
      }
    });

    return () => {
      client.end(true);
    };
  }, []);

  const dismissAlert = useCallback((id: string) => {
    setState((s) => ({ ...s, alerts: s.alerts.filter((a) => a.id !== id) }));
  }, []);

  return { ...state, dismissAlert };
}
