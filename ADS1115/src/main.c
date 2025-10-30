#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <stdio.h>

#define ADS1115_ADDR 0x48
#define I2C0_NODE DT_NODELABEL(mysensor)
static const struct i2c_dt_spec i2c_dev = I2C_DT_SPEC_GET(I2C0_NODE);

// Configure ADS1115 for continuous mode, AIN0, ±0.512V, 250 SPS
uint16_t ads1115_read_adc()
{
    uint8_t config[3] = {0x01, 0xC3, 0x83};
    // 0xC3 = AIN0, PGA ±0.512V, continuous mode
    // 0x83 = 250 SPS (datasheet Table 8)

    uint8_t read_buf[2] = {0};
    uint8_t reg_pointer = 0x00;

    // Write config
    if (i2c_write(i2c_dev.bus, config, 3, ADS1115_ADDR) != 0)
    {
        printk("I2C write failed\n");
        return 0;
    }

    // Small delay for conversion to start
    k_sleep(K_MSEC(4)); // ~250 SPS → 4 ms per sample

    // Read conversion register
    if (i2c_write_read(i2c_dev.bus, ADS1115_ADDR, &reg_pointer, 1, read_buf, 2) != 0)
    {
        printk("I2C read failed\n");
        return 0;
    }

    return (read_buf[0] << 8) | read_buf[1];
}

void main(void)
{
    if (!i2c_dev.bus)
    {
        printk("Failed to get I2C device\n");
        return;
    }

    while (1)
    {
        uint32_t sum = 0;
        int samples = 8;  // average 8 samples for stability

        for (int i = 0; i < samples; i++)
        {
            sum += ads1115_read_adc();
            k_sleep(K_MSEC(4)); // wait one conversion at 250 SPS
        }

        uint16_t adc_value = sum / samples;
        float voltage = (adc_value * 0.512 * 2) / 32768.0f;
        // ±0.512 V full scale, so LSB = 0.000015625 V

        printk("ADC Value: %d, Voltage: %.6f V\n", adc_value, voltage);
        k_sleep(K_MSEC(100)); // print every 100 ms
    }
}

// #include <zephyr/kernel.h>
// #include <zephyr/device.h>
// #include <zephyr/devicetree.h>
// #include <zephyr/drivers/i2c.h>
// #include <zephyr/sys/printk.h>

// #define ADS1115_ADDR 0x48
// #define I2C_NODE DT_NODELABEL(mysensor)
// static const struct i2c_dt_spec i2c_dev = I2C_DT_SPEC_GET(I2C_NODE);

// #define ADS1115_REG_CONVERT 0x00
// #define ADS1115_REG_CONFIG 0x01

// // LSB for ±0.256 V range
// #define ADS1115_LSB_mV 0.0078125f // 7.8125 µV

// // ADS1115 config: AIN0, ±0.256 V, continuous, 250 SPS, comparator off
// static const uint16_t ADS1115_CFG = 0x4AA3; // see breakdown above

// static int ads1115_write_config(uint16_t cfg)
// {
//     uint8_t buf[3] = {ADS1115_REG_CONFIG, (uint8_t)(cfg >> 8), (uint8_t)(cfg & 0xFF)};
//     return i2c_write_dt(&i2c_dev, buf, sizeof(buf));
// }

// static int16_t ads1115_read_raw(void)
// {
//     uint8_t reg = ADS1115_REG_CONVERT;
//     uint8_t data[2];
//     if (i2c_write_read_dt(&i2c_dev, &reg, 1, data, 2) != 0)
//         return INT16_MIN;
//     return (int16_t)((data[0] << 8) | data[1]);
// }

// void main(void)
// {
//     if (!device_is_ready(i2c_dev.bus))
//     {
//         printk("I2C bus not ready!\n");
//         return;
//     }

//     if (ads1115_write_config(ADS1115_CFG) != 0)
//     {
//         printk("ADS1115 config write failed\n");
//         return;
//     }

//     // One small delay to let the first conversion complete at 250 SPS (~4 ms)
//     k_msleep(5);

//     while (1)
//     {
//         int32_t sum = 0;
//         const int N = 8;

//         for (int i = 0; i < N; i++)
//         {
//             int16_t raw = ads1115_read_raw();
//             if (raw == INT16_MIN)
//             {
//                 printk("ADC read failed\n");
//                 continue;
//             }
//             sum += raw;
//             k_msleep(4); // ~1 conversion interval at 250 SPS
//         }

//         float avg_raw = sum / (float)N;
//         float voltage_mV = avg_raw * ADS1115_LSB_mV;

//         printk("ADC Value: %6.0f, Voltage: %.3f mV\n", avg_raw, voltage_mV);

//         // If you see values approaching ±256 mV (clip risk), raise PGA to ±0.512 V.
//         k_msleep(100);
//     }
// }
