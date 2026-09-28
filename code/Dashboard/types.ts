// Shared types for the water purification dashboard.
// Mirrors the Clovis pattern: typed telemetry payloads + threshold-aware metric config.

export type ContaminationClass =
  | "CLEAN"
  | "SILT"
  | "HEAVY_METAL"
  | "ACIDIC_DRAINAGE"
  | "UNKNOWN_LOW_CONFIDENCE";

export interface SensorReading {
  deviceId: string;          // e.g. "WPU-JHK-001"
  timestamp: number;         // epoch ms
  pH: number;
  tds_ppm: number;
  turbidity_ntu: number;
  orp_mv: number;
  heavyMetal_ppb: number;
  temperature_c: number;
  flow_lps: number;
  contaminationClass: ContaminationClass;
  batchRejected: boolean;
}

export type AlertSeverity = "info" | "warning" | "critical";

export interface Alert {
  id: string;
  deviceId: string;
  metric: keyof Pick<
    SensorReading,
    "pH" | "tds_ppm" | "turbidity_ntu" | "orp_mv" | "heavyMetal_ppb" | "temperature_c"
  > | "batch";
  message: string;
  severity: AlertSeverity;
  value?: number;
  timestamp: number;
}

export interface MetricThreshold {
  label: string;
  unit: string;
  safeMin: number;
  safeMax: number;
  warnMin: number;
  warnMax: number;
  // Below warnMin/above warnMax (but outside safe) => "warning"
  // Below/above warn bounds entirely => "critical"
}

export interface DeviceState {
  deviceId: string;
  latest: SensorReading | null;
  connected: boolean;
  lastSeen: number | null;
}
