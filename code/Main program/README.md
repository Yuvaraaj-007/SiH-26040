# Main Program – ESP32-S3 Firmware

Native ESP-IDF firmware in plain C with explicit FreeRTOS tasks.

## Tasks
| Task | Priority | Role |
|---|---|---|
| `sensor_task` | 5 | Polls ADS1115, LMP91000, DS18B20 and the flow pulse count every 2 s |
| `control_task` | 6 | Classifies the water, sets valves/pump/UV, runs the post-treatment safety check |
| `comm_task` | 4 | Telemetry output (placeholder for LoRa/GSM) |

Tasks communicate through two queues: sensor → control and control → comm.

## Classification and routing
| Class | Rule | Valve 1 | Valve 2 | UV |
|---|---|---|---|---|
| Acidic drainage | heavy metal > 50 ppb or ORP > 400 mV, and pH < 5.5 | on | on | on |
| Heavy metal | heavy metal > 50 ppb or ORP > 400 mV | on | on | on |
| Silt | turbidity > 50 NTU | on | off | on |
| Clean | pH 6.5–8.5, turbidity < 5 NTU, heavy metal < 10 ppb | off | off | on |
| Unknown / low confidence | anything else | on | on | on |

Unknown readings fall back to the most thorough treatment path.

**Safety check:** a batch passes only if pH is 6.5–8.5, turbidity is below 5 NTU and heavy metal is below 10 ppb. Otherwise it is logged as diverted and not stored.

## Pins
| Function | Pin |
|---|---|
| I2C SDA / SCL | GPIO8 / GPIO9 |
| DS18B20 | GPIO4 |
| Flow sensor | GPIO34 |
| Valve 1 / Valve 2 | GPIO5 / GPIO6 |
| Pump PWM | GPIO7 |
| UV relay | GPIO15 |

Adjust these for your board in `main.c`.

## Build and flash
Requires [ESP-IDF](https://docs.espressif.com/projects/esp-idf/) (v5.x recommended) and an ESP32-S3 board.

```bash
cd "code/Main program"
idf.py set-target esp32s3
idf.py build
idf.py -p <PORT> flash monitor
```

## Still to do
- Replace the rule-based `classify_water()` with TensorFlow Lite Micro inference
- Replace the placeholder sensor calibration (pH, TDS, turbidity, ORP, heavy-metal ppb) with real calibration curves
- Drive the reject-line solenoid on a failed batch
- Implement LoRa (SX1278, SPI) and GSM (SIM800L, UART) transmission in `comm_task`
- Add predictive filter-fouling alerts
