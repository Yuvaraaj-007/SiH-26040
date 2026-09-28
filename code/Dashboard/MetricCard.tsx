import React from "react";
import { getMetricStatus, STATUS_COLORS, THRESHOLDS } from "./thresholds";

interface MetricCardProps {
  metric: keyof typeof THRESHOLDS;
  value: number;
}

export default function MetricCard({ metric, value }: MetricCardProps) {
  const cfg = THRESHOLDS[metric];
  const status = getMetricStatus(metric, value);
  const colors = STATUS_COLORS[status];

  return (
    <div
      style={{
        background: colors.bg,
        border: `1.5px solid ${colors.border}`,
        borderRadius: 8,
        padding: "10px 14px",
        minWidth: 140,
      }}
    >
      <div style={{ fontSize: 11, fontWeight: 600, color: colors.text, textTransform: "uppercase" }}>
        {cfg.label}
      </div>
      <div style={{ fontSize: 22, fontWeight: 700, color: colors.text }}>
        {value.toFixed(2)}
        <span style={{ fontSize: 12, fontWeight: 500, marginLeft: 4 }}>{cfg.unit}</span>
      </div>
      <div style={{ fontSize: 10, color: colors.text, opacity: 0.75 }}>
        safe: {cfg.safeMin}-{cfg.safeMax}{cfg.unit}
      </div>
    </div>
  );
}
