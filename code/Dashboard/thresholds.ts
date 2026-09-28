import { MetricThreshold, SensorReading } from "./types";

// Safe / warn bands per metric. Tune these to your actual calibration and
// the BIS/WHO drinking-water limits you cite in the report.
export const THRESHOLDS: Record<
  "pH" | "tds_ppm" | "turbidity_ntu" | "orp_mv" | "heavyMetal_ppb" | "temperature_c",
  MetricThreshold
> = {
  pH: { label: "pH", unit: "", safeMin: 6.5, safeMax: 8.5, warnMin: 5.5, warnMax: 9.5 },
  tds_ppm: { label: "TDS", unit: "ppm", safeMin: 0, safeMax: 500, warnMin: 0, warnMax: 1000 },
  turbidity_ntu: { label: "Turbidity", unit: "NTU", safeMin: 0, safeMax: 5, warnMin: 0, warnMax: 50 },
  orp_mv: { label: "ORP", unit: "mV", safeMin: -50, safeMax: 400, warnMin: -100, warnMax: 600 },
  heavyMetal_ppb: { label: "Heavy metals", unit: "ppb", safeMin: 0, safeMax: 10, warnMin: 0, warnMax: 50 },
  temperature_c: { label: "Temperature", unit: "°C", safeMin: 5, safeMax: 35, warnMin: 0, warnMax: 45 },
};

export type MetricStatus = "safe" | "warning" | "critical";

export function getMetricStatus(
  metric: keyof typeof THRESHOLDS,
  value: number
): MetricStatus {
  const t = THRESHOLDS[metric];
  if (value >= t.safeMin && value <= t.safeMax) return "safe";
  if (value >= t.warnMin && value <= t.warnMax) return "warning";
  return "critical";
}

// Overall device status = worst status across all metrics (mirrors the
// Clovis board-health-modal "worst metric wins" coloring rule).
export function getOverallStatus(reading: SensorReading): MetricStatus {
  const metrics: (keyof typeof THRESHOLDS)[] = [
    "pH",
    "tds_ppm",
    "turbidity_ntu",
    "orp_mv",
    "heavyMetal_ppb",
    "temperature_c",
  ];
  let worst: MetricStatus = "safe";
  for (const m of metrics) {
    const s = getMetricStatus(m, reading[m]);
    if (s === "critical") return "critical";
    if (s === "warning") worst = "warning";
  }
  return worst;
}

export const STATUS_COLORS: Record<MetricStatus, { bg: string; border: string; text: string }> = {
  safe: { bg: "#e6f7ec", border: "#2e9e5b", text: "#1c6b3d" },
  warning: { bg: "#fff6e0", border: "#d99a1e", text: "#8a6110" },
  critical: { bg: "#fdeaea", border: "#d9482e", text: "#9c2c18" },
};
