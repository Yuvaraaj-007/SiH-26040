import React from "react";
import { Alert } from "./types";
import { STATUS_COLORS } from "./thresholds";

interface AlertsPanelProps {
  alerts: Alert[];
  onDismiss: (id: string) => void;
}

const SEVERITY_TO_STATUS: Record<Alert["severity"], keyof typeof STATUS_COLORS> = {
  info: "safe",
  warning: "warning",
  critical: "critical",
};

export default function AlertsPanel({ alerts, onDismiss }: AlertsPanelProps) {
  if (alerts.length === 0) {
    return (
      <div
        style={{
          position: "sticky",
          top: 0,
          zIndex: 10,
          background: "#f0f4f2",
          padding: "8px 16px",
          fontSize: 13,
          color: "#4a5a5e",
        }}
      >
        No active alerts - all deployed units within safe range.
      </div>
    );
  }

  const worst = alerts.some((a) => a.severity === "critical")
    ? "critical"
    : alerts.some((a) => a.severity === "warning")
    ? "warning"
    : "info";
  const barColor = STATUS_COLORS[SEVERITY_TO_STATUS[worst as Alert["severity"]]];

  return (
    <div
      style={{
        position: "sticky",
        top: 0,
        zIndex: 10,
        background: barColor.bg,
        borderBottom: `2px solid ${barColor.border}`,
        padding: "6px 16px",
        maxHeight: 160,
        overflowY: "auto",
      }}
    >
      {alerts.map((a) => {
        const c = STATUS_COLORS[SEVERITY_TO_STATUS[a.severity]];
        return (
          <div
            key={a.id}
            style={{
              display: "flex",
              justifyContent: "space-between",
              alignItems: "center",
              padding: "4px 0",
              borderBottom: "1px solid rgba(0,0,0,0.05)",
            }}
          >
            <span style={{ fontSize: 13, color: c.text }}>
              <strong style={{ textTransform: "uppercase", marginRight: 6 }}>
                {a.severity}
              </strong>
              [{a.deviceId}] {a.message}
            </span>
            <button
              onClick={() => onDismiss(a.id)}
              style={{
                border: "none",
                background: "transparent",
                cursor: "pointer",
                color: c.text,
                fontSize: 12,
              }}
            >
              dismiss
            </button>
          </div>
        );
      })}
    </div>
  );
}
