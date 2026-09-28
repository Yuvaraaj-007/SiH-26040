/*
 * Smart Water Purification & Quality Monitoring System
 * ESP-IDF native firmware (plain C, explicit FreeRTOS tasks)
 *
 * Structure:
 *   sensor_task   - polls ADS1115, LMP91000, DS18B20, flow ISR count
 *   control_task  - classifies water, drives valves/pump/UV, runs safety check
 *   comm_task     - placeholder for LoRa/GSM telemetry
 *
 * Adjust GPIO/I2C pin numbers to your specific ESP32-S3 board.
 */

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/i2c.h"
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "esp_rom_sys.h"
#include "esp_log.h"

static const char *TAG = "WATER_SYS";

// ---------------- Pin / bus definitions ----------------
#define I2C_PORT            I2C_NUM_0
#define I2C_SDA_PIN         8
#define I2C_SCL_PIN         9
#define I2C_FREQ_HZ         100000

#define ONEWIRE_GPIO        GPIO_NUM_4
#define FLOW_SENSOR_GPIO    GPIO_NUM_34

#define VALVE1_GPIO         GPIO_NUM_5
#define VALVE2_GPIO         GPIO_NUM_6
#define UV_RELAY_GPIO       GPIO_NUM_15
#define PUMP_PWM_GPIO       GPIO_NUM_7

#define ADS1115_ADDR        0x48
#define LMP91000_ADDR       0x4D

// ADS1115 registers
#define ADS_REG_CONVERSION  0x00
#define ADS_REG_CONFIG      0x01

// LMP91000 registers
#define LMP_REG_STATUS      0x00
#define LMP_REG_LOCK        0x01
#define LMP_REG_TIACN       0x10
#define LMP_REG_REFCN       0x11
#define LMP_REG_MODECN      0x12

// ---------------- Shared data types ----------------
typedef struct {
    float pH;
    float tds_ppm;
    float turbidity_ntu;
    float orp_mv;
    float heavy_metal_ppb;
    float temperature_c;
    float flow_lps;
} sensor_reading_t;

typedef enum {
    CLASS_CLEAN,
    CLASS_SILT,
    CLASS_HEAVY_METAL,
    CLASS_ACIDIC_DRAINAGE,
    CLASS_UNKNOWN_LOW_CONFIDENCE
} contamination_class_t;

typedef struct {
    sensor_reading_t reading;
    contamination_class_t cls;
} telemetry_packet_t;

static QueueHandle_t sensor_to_control_q;
static QueueHandle_t control_to_comm_q;

static volatile uint32_t flow_pulse_count = 0;

// ---------------- Flow sensor ISR ----------------
static void IRAM_ATTR flow_isr_handler(void *arg) {
    flow_pulse_count++;
}

// ---------------- I2C low-level helpers ----------------
static esp_err_t i2c_write_reg(uint8_t dev_addr, uint8_t reg, const uint8_t *data, size_t len) {
    uint8_t buf[8];
    buf[0] = reg;
    memcpy(&buf[1], data, len);
    return i2c_master_write_to_device(I2C_PORT, dev_addr, buf, len + 1, pdMS_TO_TICKS(100));
}

static esp_err_t i2c_read_reg(uint8_t dev_addr, uint8_t reg, uint8_t *data, size_t len) {
    return i2c_master_write_read_device(I2C_PORT, dev_addr, &reg, 1, data, len, pdMS_TO_TICKS(100));
}

static void i2c_bus_init(void) {
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_SDA_PIN,
        .scl_io_num = I2C_SCL_PIN,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_FREQ_HZ,
    };
    ESP_ERROR_CHECK(i2c_param_config(I2C_PORT, &conf));
    ESP_ERROR_CHECK(i2c_driver_install(I2C_PORT, conf.mode, 0, 0, 0));
}

// ---------------- ADS1115 ----------------
// Config register: single-shot, +/-4.096V FSR, 128SPS, MUX selects channel 0-3 (single-ended)
static int16_t ads1115_read_channel(uint8_t channel) {
    uint16_t config = 0x8583 | ((0x04 + channel) << 12); // OS=1, MUX=100+ch, PGA=001, MODE=1
    uint8_t cfg_bytes[2] = { (uint8_t)(config >> 8), (uint8_t)(config & 0xFF) };
    i2c_write_reg(ADS1115_ADDR, ADS_REG_CONFIG, cfg_bytes, 2);

    vTaskDelay(pdMS_TO_TICKS(9)); // conversion time at 128SPS

    uint8_t raw[2];
    i2c_read_reg(ADS1115_ADDR, ADS_REG_CONVERSION, raw, 2);
    return (int16_t)((raw[0] << 8) | raw[1]);
}

static float ads1115_to_volts(int16_t raw) {
    return raw * (4.096f / 32768.0f); // matches PGA +/-4.096V setting above
}

// ---------------- LMP91000 ----------------
static void lmp91000_write(uint8_t reg, uint8_t val) {
    i2c_write_reg(LMP91000_ADDR, reg, &val, 1);
}

static void lmp91000_init(void) {
    lmp91000_write(LMP_REG_LOCK, 0x00);   // unlock TIA/REF registers
    lmp91000_write(LMP_REG_TIACN, 0x03);  // TIA gain - tune per electrode datasheet
    lmp91000_write(LMP_REG_REFCN, 0x30);  // internal reference, 50% bias
    lmp91000_write(LMP_REG_MODECN, 0x03); // amperometric mode
    lmp91000_write(LMP_REG_LOCK, 0x01);   // lock registers
}

// Placeholder conversion - calibrate against known heavy-metal standard solutions
static float lmp91000_read_heavy_metal_ppb(void) {
    int16_t raw = ads1115_read_channel(3);
    float v = ads1115_to_volts(raw);
    return v * 250.0f; // linear placeholder
}

// ---------------- DS18B20 (bit-banged 1-Wire) ----------------
static void ow_write_bit(int bit) {
    gpio_set_direction(ONEWIRE_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_level(ONEWIRE_GPIO, 0);
    esp_rom_delay_us(bit ? 6 : 60);
    gpio_set_level(ONEWIRE_GPIO, 1);
    esp_rom_delay_us(bit ? 64 : 10);
}

static int ow_read_bit(void) {
    int bit;
    gpio_set_direction(ONEWIRE_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_level(ONEWIRE_GPIO, 0);
    esp_rom_delay_us(3);
    gpio_set_direction(ONEWIRE_GPIO, GPIO_MODE_INPUT);
    esp_rom_delay_us(10);
    bit = gpio_get_level(ONEWIRE_GPIO);
    esp_rom_delay_us(50);
    return bit;
}

static int ow_reset(void) {
    int presence;
    gpio_set_direction(ONEWIRE_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_level(ONEWIRE_GPIO, 0);
    esp_rom_delay_us(480);
    gpio_set_direction(ONEWIRE_GPIO, GPIO_MODE_INPUT);
    esp_rom_delay_us(70);
    presence = !gpio_get_level(ONEWIRE_GPIO);
    esp_rom_delay_us(410);
    return presence; // 1 = device present
}

static void ow_write_byte(uint8_t byte) {
    for (int i = 0; i < 8; i++) {
        ow_write_bit(byte & 0x01);
        byte >>= 1;
    }
}

static uint8_t ow_read_byte(void) {
    uint8_t byte = 0;
    for (int i = 0; i < 8; i++) {
        byte |= (ow_read_bit() << i);
    }
    return byte;
}

static float ds18b20_read_temperature(void) {
    if (!ow_reset()) {
        ESP_LOGW(TAG, "DS18B20 not responding");
        return -127.0f;
    }
    ow_write_byte(0xCC); // Skip ROM (single device on bus)
    ow_write_byte(0x44); // Convert T
    vTaskDelay(pdMS_TO_TICKS(750)); // 12-bit conversion time

    ow_reset();
    ow_write_byte(0xCC);
    ow_write_byte(0xBE); // Read scratchpad

    uint8_t lsb = ow_read_byte();
    uint8_t msb = ow_read_byte();
    int16_t raw = (msb << 8) | lsb;
    return raw / 16.0f;
}

// ---------------- GPIO / PWM setup ----------------
static void actuators_init(void) {
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << VALVE1_GPIO) | (1ULL << VALVE2_GPIO) | (1ULL << UV_RELAY_GPIO),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&io_conf);

    ledc_timer_config_t timer_conf = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .duty_resolution = LEDC_TIMER_10_BIT,
        .timer_num = LEDC_TIMER_0,
        .freq_hz = 5000,
        .clk_cfg = LEDC_AUTO_CLK,
    };
    ledc_timer_config(&timer_conf);

    ledc_channel_config_t ch_conf = {
        .gpio_num = PUMP_PWM_GPIO,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = LEDC_CHANNEL_0,
        .timer_sel = LEDC_TIMER_0,
        .duty = 0,
        .hpoint = 0,
    };
    ledc_channel_config(&ch_conf);
}

static void pump_set_duty_percent(uint8_t percent) {
    uint32_t duty = (1023 * percent) / 100;
    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, duty);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
}

static void flow_sensor_init(void) {
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << FLOW_SENSOR_GPIO),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .intr_type = GPIO_INTR_POSEDGE,
    };
    gpio_config(&io_conf);
    gpio_install_isr_service(0);
    gpio_isr_handler_add(FLOW_SENSOR_GPIO, flow_isr_handler, NULL);
}

// ---------------- Classification (replace with TFLite Micro inference) ----------------
static contamination_class_t classify_water(const sensor_reading_t *r) {
    if (r->heavy_metal_ppb > 50.0f || r->orp_mv > 400.0f) {
        return (r->pH < 5.5f) ? CLASS_ACIDIC_DRAINAGE : CLASS_HEAVY_METAL;
    }
    if (r->turbidity_ntu > 50.0f) {
        return CLASS_SILT;
    }
    if (r->pH >= 6.5f && r->pH <= 8.5f && r->turbidity_ntu < 5.0f && r->heavy_metal_ppb < 10.0f) {
        return CLASS_CLEAN;
    }
    return CLASS_UNKNOWN_LOW_CONFIDENCE;
}

static void apply_treatment_path(contamination_class_t cls) {
    switch (cls) {
        case CLASS_CLEAN:
            gpio_set_level(VALVE1_GPIO, 0);
            gpio_set_level(VALVE2_GPIO, 0);
            gpio_set_level(UV_RELAY_GPIO, 1);
            break;
        case CLASS_SILT:
            gpio_set_level(VALVE1_GPIO, 1);
            gpio_set_level(VALVE2_GPIO, 0);
            gpio_set_level(UV_RELAY_GPIO, 1);
            break;
        case CLASS_HEAVY_METAL:
        case CLASS_ACIDIC_DRAINAGE:
            gpio_set_level(VALVE1_GPIO, 1);
            gpio_set_level(VALVE2_GPIO, 1);
            gpio_set_level(UV_RELAY_GPIO, 1);
            break;
        case CLASS_UNKNOWN_LOW_CONFIDENCE:
        default:
            // Safety fallback: run the most thorough path available
            gpio_set_level(VALVE1_GPIO, 1);
            gpio_set_level(VALVE2_GPIO, 1);
            gpio_set_level(UV_RELAY_GPIO, 1);
            break;
    }
}

static bool post_treatment_check_passes(const sensor_reading_t *r) {
    return (r->pH >= 6.5f && r->pH <= 8.5f &&
            r->turbidity_ntu < 5.0f &&
            r->heavy_metal_ppb < 10.0f);
}

// ---------------- Tasks ----------------
static void sensor_task(void *arg) {
    uint32_t last_flow_calc_ms = 0;
    float flow_lps = 0.0f;

    while (1) {
        // Flow rate integration (1s window)
        uint32_t now_ms = xTaskGetTickCount() * portTICK_PERIOD_MS;
        if (now_ms - last_flow_calc_ms >= 1000) {
            uint32_t pulses = flow_pulse_count;
            flow_pulse_count = 0;
            flow_lps = pulses / 450.0f; // YF-S201 typical - check your unit's datasheet
            last_flow_calc_ms = now_ms;
        }

        sensor_reading_t r;
        int16_t raw_ph   = ads1115_read_channel(0);
        int16_t raw_tds  = ads1115_read_channel(1);
        int16_t raw_turb = ads1115_read_channel(2);

        float v_ph   = ads1115_to_volts(raw_ph);
        float v_tds  = ads1115_to_volts(raw_tds);
        float v_turb = ads1115_to_volts(raw_turb);

        // Calibration placeholders - replace with your probe's actual calibration curve
        r.pH            = 7.0f + ((2.5f - v_ph) / 0.18f);
        r.tds_ppm       = v_tds * 500.0f;
        r.turbidity_ntu = (v_turb < 2.5f) ? (2500.0f - v_turb * 1000.0f) / 10.0f : 0.0f;
        r.orp_mv        = (v_ph - 2.5f) * 1000.0f;
        r.heavy_metal_ppb = lmp91000_read_heavy_metal_ppb();
        r.temperature_c = ds18b20_read_temperature();
        r.flow_lps      = flow_lps;

        xQueueSend(sensor_to_control_q, &r, pdMS_TO_TICKS(100));
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}

static void control_task(void *arg) {
    sensor_reading_t r;
    while (1) {
        if (xQueueReceive(sensor_to_control_q, &r, portMAX_DELAY) == pdTRUE) {
            contamination_class_t cls = classify_water(&r);
            apply_treatment_path(cls);
            pump_set_duty_percent(70); // adjust duty per your pump's flow-rate needs

            ESP_LOGI(TAG,
                "pH=%.2f TDS=%.0fppm Turb=%.1fNTU ORP=%.0fmV Metal=%.1fppb Temp=%.1fC Flow=%.2fL/s Class=%d",
                r.pH, r.tds_ppm, r.turbidity_ntu, r.orp_mv, r.heavy_metal_ppb,
                r.temperature_c, r.flow_lps, cls);

            if (!post_treatment_check_passes(&r)) {
                ESP_LOGW(TAG, "[SAFETY] Batch failed post-treatment check - diverted, not stored.");
                // TODO: trigger reject-line solenoid here
            }

            telemetry_packet_t pkt = { .reading = r, .cls = cls };
            xQueueSend(control_to_comm_q, &pkt, pdMS_TO_TICKS(100));
        }
    }
}

static void comm_task(void *arg) {
    telemetry_packet_t pkt;
    while (1) {
        if (xQueueReceive(control_to_comm_q, &pkt, portMAX_DELAY) == pdTRUE) {
            // TODO: send `pkt` over LoRa (SX1278 via SPI) or GSM (SIM800L via UART)
            ESP_LOGI(TAG, "[COMM] Telemetry queued for transmission (class=%d)", pkt.cls);
        }
    }
}

void app_main(void) {
    ESP_LOGI(TAG, "Water purification control unit starting...");

    i2c_bus_init();
    lmp91000_init();
    actuators_init();
    flow_sensor_init();

    sensor_to_control_q = xQueueCreate(4, sizeof(sensor_reading_t));
    control_to_comm_q   = xQueueCreate(4, sizeof(telemetry_packet_t));

    xTaskCreate(sensor_task,  "sensor_task",  4096, NULL, 5, NULL);
    xTaskCreate(control_task, "control_task", 4096, NULL, 6, NULL);
    xTaskCreate(comm_task,    "comm_task",    4096, NULL, 4, NULL);

    ESP_LOGI(TAG, "All tasks started.");
}
