// #include <zephyr/kernel.h>
// #include <zephyr/device.h>
// #include <zephyr/devicetree.h>
// #include <zephyr/drivers/i2c.h>
// #include <zephyr/sys/printk.h>

// #define MAX30100_I2C_ADDR 0x57
// #define REG_MODE_CONFIG 0x06
// #define REG_SPO2_CONFIG 0x07
// #define REG_LED_CONFIG 0x09
// #define REG_FIFO_DATA 0x05
// #define REG_INTERRUPT_STAT 0x00

// #define I2C_NODE DT_NODELABEL(mysensor)

// static const struct i2c_dt_spec i2c_dev = I2C_DT_SPEC_GET(I2C_NODE);

// static int max30100_write_register(uint8_t reg, uint8_t value)
// {
//     return i2c_reg_write_byte_dt(&i2c_dev, reg, value);
// }

// static int max30100_read_register(uint8_t reg, uint8_t *value)
// {
//     return i2c_reg_read_byte_dt(&i2c_dev, reg, value);
// }

// static int max30100_init()
// {
//     uint8_t id;

//     if (max30100_read_register(0xFF, &id) != 0)
//     {
//         printk("MAX30100 not detected!\n");
//         return -1;
//     }

//     // Reset the device
//     max30100_write_register(REG_MODE_CONFIG, 0x40);
//     k_msleep(100);

//     // Enable SpO2 mode
//     max30100_write_register(REG_MODE_CONFIG, 0x03);

//     // Set SpO2 sample rate = 100Hz, LED pulse width = 1600us
//     max30100_write_register(REG_SPO2_CONFIG, 0x27);

//     // Set LED current (IR = 50mA, Red = 27.1mA)
//     max30100_write_register(REG_LED_CONFIG, 0x24);

//     return 0;
// }

// static int max30100_read_data(uint16_t *ir, uint16_t *red)
// {
//     uint8_t data[4];
//     uint8_t reg = REG_FIFO_DATA;

//     if (i2c_write_read_dt(&i2c_dev, &reg, 1, data, 4) != 0)
//     {
//         printk("Failed to read FIFO data!\n");
//         return -1;
//     }

//     *ir = (data[0] << 8) | data[1];
//     *red = (data[2] << 8) | data[3];

//     return 0;
// }

// void main(void)
// {
//     uint16_t ir, red;

//     if (!device_is_ready(i2c_dev.bus))
//     {
//         printk("I2C bus not ready!\n");
//         return;
//     }

//     if (max30100_init() != 0)
//     {
//         printk("MAX30100 initialization failed!\n");
//         return;
//     }

//     printk("MAX30100 Initialized Successfully!\n");

//     while (1)
//     {
//         if (max30100_read_data(&ir, &red) == 0)
//         {
//             printk("IR: %d, RED: %d\n", ir, red);
//         }
//         else
//         {
//             printk("Sensor read failed!\n");
//         }
//         k_msleep(100);
//     }
// }

///////////////////////////////

#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <math.h>

LOG_MODULE_REGISTER(max30100, LOG_LEVEL_INF);

#define MAX30100_ADDR 0x57
#define REG_INTERRUPT_ENABLE 0x01
#define REG_MODE_CONFIG 0x06
#define REG_SPO2_CONFIG 0x07
#define REG_LED_CONFIG 0x09
#define REG_FIFO_DATA 0x05
#define REG_PART_ID 0xFF
#define REG_FIFO_WR_PTR 0x02
#define REG_FIFO_RD_PTR 0x04

#define MAX30100_MODE_SPO2 0x03
#define LED_CURRENT_50MA 0x2F
#define SPO2_SAMPLING_100HZ 0x47
#define FINGER_DETECT_THRESHOLD 5000

#define MA4_SIZE 8
#define MAX_PEAKS 8

#if !DT_NODE_HAS_STATUS(DT_NODELABEL(i2c0), okay)
#error "I2C0 device is not defined or enabled in the device tree"
#endif
const struct device *i2c_dev = DEVICE_DT_GET(DT_NODELABEL(i2c0));

static int max30100_write_register(uint8_t reg, uint8_t value)
{
    uint8_t buffer[2] = {reg, value};
    return i2c_write(i2c_dev, buffer, sizeof(buffer), MAX30100_ADDR);
}

static int max30100_check_presence(void)
{
    uint8_t reg = REG_PART_ID;
    uint8_t part_id;
    int ret = i2c_write_read(i2c_dev, MAX30100_ADDR, &reg, 1, &part_id, 1);
    if (ret < 0 || part_id != 0x11)
    {
        LOG_ERR("MAX30100 not detected (part_id: 0x%02x, err: %d)", part_id, ret);
        return -ENODEV;
    }
    LOG_INF("MAX30100 detected, part ID: 0x%02x", part_id);
    return 0;
}

static int max30100_read_fifo(uint16_t *ir, uint16_t *red)
{
    uint8_t reg = REG_FIFO_WR_PTR;
    uint8_t fifo_wr_ptr, fifo_rd_ptr;
    int ret = i2c_write_read(i2c_dev, MAX30100_ADDR, &reg, 1, &fifo_wr_ptr, 1);
    if (ret < 0)
        return ret;
    reg = REG_FIFO_RD_PTR;
    ret = i2c_write_read(i2c_dev, MAX30100_ADDR, &reg, 1, &fifo_rd_ptr, 1);
    if (ret < 0)
        return ret;
    if (fifo_wr_ptr == fifo_rd_ptr)
        return -ENODATA;

    reg = REG_FIFO_DATA;
    uint8_t data[4];
    ret = i2c_write_read(i2c_dev, MAX30100_ADDR, &reg, 1, data, 4);
    if (ret < 0)
        return ret;

    *ir = (data[0] << 8) | data[1];
    *red = (data[2] << 8) | data[3];
    return 0;
}

// Moving average buffers
static uint16_t ir_buffer[MA4_SIZE] = {0};
static uint16_t red_buffer[MA4_SIZE] = {0};
static uint8_t buf_index = 0;

// RR intervals for HR calculation
static uint32_t rr_intervals[MAX_PEAKS] = {0};
static int rr_index = 0;
static uint32_t last_peak_time = 0;

static void calculate_hr_spo2(uint16_t ir, uint16_t red)
{
    ir_buffer[buf_index] = ir;
    red_buffer[buf_index] = red;
    buf_index = (buf_index + 1) % MA4_SIZE;

    uint32_t ir_avg = 0;
    for (int i = 0; i < MA4_SIZE; i++)
        ir_avg += ir_buffer[i];
    ir_avg /= MA4_SIZE;

    if (ir_avg > FINGER_DETECT_THRESHOLD)
    {
        static uint16_t prev_ir = 0;
        static uint8_t peak_detected = 0;
        uint32_t now = k_uptime_get_32();

        // Simple peak detection
        if (ir > prev_ir && !peak_detected && ir > (ir_avg * 1.02))
            peak_detected = 1;
        else if (ir < prev_ir && peak_detected)
        {
            peak_detected = 0;
            uint32_t time_diff = now - last_peak_time;
            if (time_diff > 300 && time_diff < 2000)
            {
                rr_intervals[rr_index % MAX_PEAKS] = time_diff;
                rr_index++;
                last_peak_time = now;

                if (rr_index >= 3)
                {
                    uint32_t avg_rr = 0;
                    int count = (rr_index < MAX_PEAKS) ? rr_index : MAX_PEAKS;
                    for (int i = 0; i < count; i++)
                        avg_rr += rr_intervals[i];
                    avg_rr /= count;
                    int hr = 60000 / avg_rr;

                    // Simple SpO2 estimate
                    uint32_t red_avg = 0;
                    for (int i = 0; i < MA4_SIZE; i++)
                        red_avg += red_buffer[i];
                    red_avg /= MA4_SIZE;
                    float ratio = (float)red_avg / (float)ir_avg;
                    float spo2 = 110 - 25 * ratio;
                    if (spo2 > 100)
                        spo2 = 100;
                    if (spo2 < 85)
                        spo2 = 85;

                    LOG_INF("HR: %d bpm | SpO2: %.1f%%", hr, spo2);
                }
            }
        }
        prev_ir = ir;
    }
    else
    {
        LOG_INF("No finger detected...");
        rr_index = 0;
    }
}

static struct k_timer max30100_timer;

static void timer_handler(struct k_timer *timer)
{
    uint16_t ir, red;
    while (max30100_read_fifo(&ir, &red) == 0)
    {
        calculate_hr_spo2(ir, red);
    }
}

void main(void)
{
    LOG_INF("Starting MAX30100 Heart Rate Monitor");

    if (!device_is_ready(i2c_dev))
    {
        LOG_ERR("I2C device not ready!");
        return;
    }

    if (max30100_check_presence() < 0)
        return;

    // Reset sensor
    max30100_write_register(REG_MODE_CONFIG, 0x40);
    k_msleep(100);

    // Clear FIFO pointers
    max30100_write_register(REG_FIFO_WR_PTR, 0x00);
    max30100_write_register(REG_FIFO_RD_PTR, 0x00);

    // Initialize sensor
    max30100_write_register(REG_MODE_CONFIG, MAX30100_MODE_SPO2);
    max30100_write_register(REG_SPO2_CONFIG, SPO2_SAMPLING_100HZ);
    max30100_write_register(REG_LED_CONFIG, LED_CURRENT_50MA);
    max30100_write_register(REG_INTERRUPT_ENABLE, 0x00); // Disable interrupts

    LOG_INF("MAX30100 initialized.");

    k_timer_init(&max30100_timer, timer_handler, NULL);
    k_timer_start(&max30100_timer, K_MSEC(10), K_MSEC(10)); // 100 Hz sampling

    k_sleep(K_FOREVER);
}

// #include <zephyr/kernel.h>
// #include <zephyr/device.h>
// #include <zephyr/devicetree.h>
// #include <zephyr/drivers/i2c.h>
// #include <zephyr/sys/printk.h>
// #include <math.h>

// #define MAX30100_I2C_ADDR 0x57
// #define I2C_NODE DT_NODELABEL(mysensor)

// static const struct i2c_dt_spec max30100 = I2C_DT_SPEC_GET(I2C_NODE);

// #define FIFO_DATA_REG 0x05
// #define MODE_CONFIG 0x06
// #define SPO2_CONFIG 0x07
// #define LED_CONFIG 0x09

// #define SAMPLE_BUF_SIZE 100

// static uint16_t ir_buf[SAMPLE_BUF_SIZE];
// static uint16_t red_buf[SAMPLE_BUF_SIZE];
// static int sample_index = 0;

// // --- Utility Functions ---
// static int write_register(uint8_t reg, uint8_t value)
// {
//     uint8_t data[2] = {reg, value};
//     return i2c_write_dt(&max30100, data, 2);
// }

// static int read_fifo(uint16_t *ir, uint16_t *red)
// {
//     uint8_t data[4];
//     int ret = i2c_burst_read_dt(&max30100, FIFO_DATA_REG, data, 4);
//     if (ret != 0)
//         return ret;
//     *ir = ((uint16_t)data[0] << 8) | data[1];
//     *red = ((uint16_t)data[2] << 8) | data[3];
//     return 0;
// }

// // --- Filters ---
// static float dc_filter(float input, float *dc_estimate)
// {
//     *dc_estimate = *dc_estimate + 0.1f * (input - *dc_estimate);
//     return input - *dc_estimate;
// }

// static float moving_average(float input, float *avg, int window)
// {
//     *avg = (*avg * (window - 1) + input) / window;
//     return *avg;
// }

// // --- SpO2 calculation (ratio of ratios) ---
// static float calculate_spo2(float red_ac, float ir_ac, float red_dc, float ir_dc)
// {
//     float R = (red_ac / red_dc) / (ir_ac / ir_dc);
//     float spo2 = 110.0f - 25.0f * R; // Approximation
//     if (spo2 > 100.0f)
//         spo2 = 100.0f;
//     if (spo2 < 70.0f)
//         spo2 = 70.0f;
//     return spo2;
// }

// void main(void)
// {
//     if (!device_is_ready(max30100.bus))
//     {
//         printk("I2C bus not ready!\n");
//         return;
//     }

//     printk("Initializing MAX30100...\n");

//     // --- Configure MAX30100 ---
//     write_register(MODE_CONFIG, 0x03); // SPO2+HR mode
//     write_register(SPO2_CONFIG, 0x27); // SPO2_HI_RES + 100Hz sample rate
//     write_register(LED_CONFIG, 0xFF);  // IR=50mA, RED=50mA

//     float dc_ir = 0, dc_red = 0;
//     float prev_ir = 0;
//     float ir_ac_sum = 0, red_ac_sum = 0;
//     float ir_dc_sum = 0, red_dc_sum = 0;

//     int beat_count = 0;
//     uint32_t start_time = k_uptime_get();

//     while (1)
//     {
//         uint16_t ir_raw, red_raw;
//         if (read_fifo(&ir_raw, &red_raw) == 0)
//         {
//             float ir = dc_filter((float)ir_raw, &dc_ir);
//             float red = dc_filter((float)red_raw, &dc_red);

//             ir = moving_average(ir, &prev_ir, 5);

//             ir_buf[sample_index] = ir_raw;
//             red_buf[sample_index] = red_raw;
//             sample_index = (sample_index + 1) % SAMPLE_BUF_SIZE;

//             // Finger detection (IR must cross threshold)
//             static int finger_present = 0;
//             if (ir_raw > 20000)
//             {
//                 if (!finger_present)
//                 {
//                     printk("Finger detected. Turning LED ON\n");
//                     write_register(LED_CONFIG, 0xFF);
//                     finger_present = 1;
//                 }
//             }
//             else
//             {
//                 if (finger_present)
//                 {
//                     printk("No finger detected. Turning LED OFF\n");
//                     write_register(LED_CONFIG, 0x00);
//                     finger_present = 0;
//                 }
//                 k_msleep(50);
//                 continue;
//             }

//             // Simple Peak Detection
//             static int prev_state = 0;
//             if (ir > 1000)
//             { // Threshold (tune)
//                 if (!prev_state)
//                 {
//                     beat_count++;
//                     prev_state = 1;
//                 }
//             }
//             else
//             {
//                 prev_state = 0;
//             }

//             // Collect AC/DC components for SpO2
//             ir_ac_sum += fabsf(ir);
//             red_ac_sum += fabsf(red);
//             ir_dc_sum += dc_ir;
//             red_dc_sum += dc_red;

//             // Every 5 seconds -> calculate BPM + SpO2
//             if (k_uptime_get() - start_time > 5000)
//             {
//                 float bpm = (beat_count / 5.0f) * 60.0f;
//                 float spo2 = calculate_spo2(
//                     red_ac_sum / SAMPLE_BUF_SIZE,
//                     ir_ac_sum / SAMPLE_BUF_SIZE,
//                     red_dc_sum / SAMPLE_BUF_SIZE,
//                     ir_dc_sum / SAMPLE_BUF_SIZE);

//                 printk("Samples=%d  HR=%.1f bpm  SpO2=%.1f%%\n",
//                        SAMPLE_BUF_SIZE, bpm, spo2);

//                 // Reset counters
//                 beat_count = 0;
//                 ir_ac_sum = red_ac_sum = 0;
//                 ir_dc_sum = red_dc_sum = 0;
//                 start_time = k_uptime_get();
//             }
//         }

//         k_msleep(10); // ~100Hz sample rate
//     }
// }
