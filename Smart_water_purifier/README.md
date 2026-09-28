# Smart Water Purifier – Hardware

Hardware design of the control unit.

## Files
| File | Description |
|---|---|
| `Water_purify_hardware_documentation.docx` | MCU selection, sensor selection, sensor-to-MCU wiring and actuator control |
| `Water_purify_schematics.docx` | Overview of the schematics: power section, sensor interfaces and actuator outputs |

## Design summary
- **MCU:** ESP32-S3 (sensor hub and control unit; I2C, 1-Wire, GPIO interrupts, PWM)
- **ADC:** ADS1115 (16-bit, I2C `0x48`) for the analog water-quality probes, chosen over the ESP32-S3 internal ADC for lower noise and better linearity
- **Electrochemical AFE:** LMP91000 (I2C `0x4D`) with a 3-electrode cell for heavy-metal sensing
- **Temperature:** DS18B20 (1-Wire, 4.7 kΩ pull-up)
- **Flow:** YF-S201 Hall-effect sensor, pulse counted by interrupt (5 V supply)
- **Actuators:** two valves via MOSFET with flyback diodes, PWM-driven pump, relay-switched UV ballast

## Pin map
| Signal | ESP32-S3 pin |
|---|---|
| I2C SDA / SCL (ADS1115 + LMP91000) | GPIO8 / GPIO9 |
| DS18B20 data | GPIO4 |
| YF-S201 flow pulse | GPIO34 |
| Valve 1 / Valve 2 | GPIO5 / GPIO6 |
| Pump PWM | GPIO7 |
| UV relay | GPIO15 |

## ADS1115 channels (per documentation)
| Input | Sensor |
|---|---|
| AIN0 | pH probe |
| AIN1 | TDS probe |
| AIN2 | Turbidity sensor |
| AIN3 | ORP probe |

> **Note:** the firmware in `code/Main program` currently reads AIN3 as the heavy-metal signal and derives ORP from the pH channel. Make the documentation and firmware agree before final submission.
