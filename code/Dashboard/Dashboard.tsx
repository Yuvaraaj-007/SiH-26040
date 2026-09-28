import React from "react";
import { useMqttTelemetry } from "./useMqttTelemetry";
import AlertsPanel from "./AlertsPanel";
import MetricCard from "./MetricCard";
import { getOverallStatus, STATUS_COLORS } from "./thresholds";

export default function Dashboard() {
  const { devices, alerts, connected, dismissAlert } = useMqttTelemetry();
  const deviceList = Object.values(devices);

  return (
    <div style={{ fontFamily: "system-ui, sans-serif", background: "#f7f9f9", minHeight: "100vh" }}>
      <AlertsPanel alerts={alerts} onDismiss={dismissAlert} />

      <div style={{ padding: "16px 20px" }}>
        <div style={{ display: "flex", justifyContent: "space-between", alignItems: "center" }}>
          <h2 style={{ margin: 0 }}>Water Purification &amp; Quality Monitoring</h2>
          <span
            style={{
              fontSize: 12,
              padding: "3px 10px",
              borderRadius: 12,
              background: connected ? "#e6f7ec" : "#fdeaea",
              color: connected ? "#1c6b3d" : "#9c2c18",
              fontWeight: 600,
            }}
          >
            {connected ? "● Broker connected" : "○ Broker disconnected"}
          </span>
        </div>

        {deviceList.length === 0 && (
          <p style={{ color: "#7c8f93", marginTop: 24 }}>
            Waiting for telemetry from deployed units...
          </p>
        )}

        {deviceList.map(({ deviceId, latest, lastSeen }) => {
          if (!latest) return null;
          const status = getOverallStatus(latest);
          const c = STATUS_COLORS[status];

          return (
            <div
              key={deviceId}
              style={{
                marginTop: 20,
                background: "white",
                borderRadius: 10,
                padding: 16,
                boxShadow: "0 1px 4px rgba(0,0,0,0.06)",
                borderLeft: `5px solid ${c.border}`,
              }}
            >
              <div style={{ display: "flex", justifyContent: "space-between", marginBottom: 10 }}>
                <div>
                  <strong>{deviceId}</strong>
                  <span style={{ marginLeft: 10, fontSize: 12, color: "#7c8f93" }}>
                    class: {latest.contaminationClass}
                  </span>
                </div>
                <span style={{ fontSize: 12, color: "#7c8f93" }}>
                  last seen: {lastSeen ? new Date(lastSeen).toLocaleTimeString() : "-"}
                </span>
              </div>

              <div style={{ display: "flex", gap: 10, flexWrap: "wrap" }}>
                <MetricCard metric="pH" value={latest.pH} />
                <MetricCard metric="tds_ppm" value={latest.tds_ppm} />
                <MetricCard metric="turbidity_ntu" value={latest.turbidity_ntu} />
                <MetricCard metric="orp_mv" value={latest.orp_mv} />
                <MetricCard metric="heavyMetal_ppb" value={latest.heavyMetal_ppb} />
                <MetricCard metric="temperature_c" value={latest.temperature_c} />
              </div>
            </div>
          );
        })}
      </div>
    </div>
  );
}
