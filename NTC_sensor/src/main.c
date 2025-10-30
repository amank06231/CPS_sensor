#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/sys/printk.h>
#include <math.h>

// NTC parameters
#define NTC_R25 10000.0f         // Resistance at 25°C (ohms)
#define NTC_BETA 3950.0f         // Beta constant (K)
#define SERIES_RESISTOR 10000.0f // Fixed resistor in voltage divider (ohms)
#define V_SUPPLY 3300.0f         // Supply voltage to divider in mV

// ADC parameters
#define ADC_RESOLUTION 14
#define NUM_SAMPLES 50

// Get ADC channel from Devicetree
#define USER_NODE DT_PATH(zephyr_user)
#if !DT_NODE_EXISTS(USER_NODE) || !DT_NODE_HAS_PROP(USER_NODE, io_channels)
#error "No suitable ADC io-channels in devicetree"
#endif

static const struct adc_dt_spec adc_channel = ADC_DT_SPEC_GET_BY_IDX(USER_NODE, 0);

void main(void)
{
        int ret;

        struct adc_sequence sequence = {
            .buffer = NULL,
            .buffer_size = sizeof(int16_t),
            .resolution = ADC_RESOLUTION,
            .oversampling = 8,
            .channels = BIT(0),
        };

        static int16_t sample;
        sequence.buffer = &sample;

        // Init ADC
        if (!device_is_ready(adc_channel.dev))
        {
                printk("ADC not ready\n");
                return;
        }
        if ((ret = adc_channel_setup_dt(&adc_channel)) < 0)
        {
                printk("ADC channel setup failed: %d\n", ret);
                return;
        }

        printk("System Ready. Reading NTC temperature...\n");

        while (1)
        {
                int32_t total_mv = 0;

                for (int i = 0; i < NUM_SAMPLES; i++)
                {
                        adc_sequence_init_dt(&adc_channel, &sequence);

                        ret = adc_read_dt(&adc_channel, &sequence);
                        if (ret < 0)
                        {
                                printk("ADC read failed: %d\n", ret);
                                continue;
                        }

                        int32_t val_mv = sample; // raw ADC counts
                        ret = adc_raw_to_millivolts_dt(&adc_channel, &val_mv);
                        if (ret == 0)
                        {
                                if (val_mv < 0)
                                        val_mv = 0; // clamp to avoid negatives
                                if (val_mv > V_SUPPLY)
                                        val_mv = V_SUPPLY; // clamp to supply
                                total_mv += val_mv;
                        }
                        else
                        {
                                printk("Failed to convert ADC to mV\n");
                        }

                        k_sleep(K_MSEC(5));
                }

                float avg_mv = total_mv / (float)NUM_SAMPLES;

                // Voltage ratio
                float v_ratio = avg_mv / V_SUPPLY;

                // Avoid divide-by-zero
                if (v_ratio > 0.0f && v_ratio < 1.0f)
                {
                        // NTC resistance
                        float ntc_resistance = SERIES_RESISTOR * v_ratio / (1.0f - v_ratio);

                        // Steinhart–Hart simplified
                        float temp_k = 1.0f / ((1.0f / (25.0f + 273.15f)) +
                                               (1.0f / NTC_BETA) * logf(ntc_resistance / NTC_R25));
                        float temp_c = temp_k - 273.15f;

                        printk("Avg Voltage: %.2f mV | NTC Resistance: %.2f ohms | Temp: %.2f °C\n",
                               avg_mv, ntc_resistance, temp_c);
                }
                else
                {
                        printk("Invalid voltage ratio: %.4f\n", v_ratio);
                }

                // float avg_mv = total_mv / (float)NUM_SAMPLES;

                // Calculate NTC resistance
                // float v_ratio = avg_mv / V_SUPPLY;
                float ntc_resistance = SERIES_RESISTOR * v_ratio / (1.0f - v_ratio);

                // Convert resistance to temperature (Steinhart–Hart simplified)
                float temp_k = 1.0f / ((1.0f / (25.0f + 273.15f)) + (1.0f / NTC_BETA) * logf(ntc_resistance / NTC_R25));
                float temp_c = temp_k - 273.15f;

                printk("Avg Voltage: %.2f mV | NTC Resistance: %.2f ohms | Temp: %.2f °C\n",
                       avg_mv, ntc_resistance, temp_c);

                k_sleep(K_MSEC(1000));
        }
}
