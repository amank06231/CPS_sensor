#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/display.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/sensor/mhz19b.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <zephyr/sys/reboot.h>

#define DISPLAY_WIDTH 128
#define DISPLAY_HEIGHT 64
#define FRAMEBUFFER_SIZE (DISPLAY_WIDTH * DISPLAY_HEIGHT / 8)
#define SLEEP_DURATION_MS 100
#define PROX_THRESHOLD 20

static const struct device *display_dev, *sht, *vcnl, *co2, *tof;
struct sensor_value lux_val, temp_val, hum_val, prox_val, co2_val, tof_val;
char display_str[32];
static int distance = 0;
static int expression_stage = 0;
int prev_distance = 0;           // Store the previous proximity value
static bool reboot_done = false; // Track if reboot has happened

LOG_MODULE_REGISTER(ssd1306_display);

#define LEFT_EYE_X 40
#define LEFT_EYE_Y 32
#define RIGHT_EYE_X 88
#define RIGHT_EYE_Y 32
#define EYE_RADIUS_X 22
#define EYE_RADIUS_Y 28
#define PUPIL_RADIUS 10

static uint8_t framebuffer[FRAMEBUFFER_SIZE];

struct display_buffer_descriptor desc = {
    .buf_size = FRAMEBUFFER_SIZE,
    .width = DISPLAY_WIDTH,
    .height = DISPLAY_HEIGHT,
    .pitch = DISPLAY_WIDTH,
};

// Clear entire framebuffer (black background)
void clear_framebuffer(void)
{
        memset(framebuffer, 0, sizeof(framebuffer));
}

// Set a pixel to white in the framebuffer
void set_pixel(int x, int y)
{
        if (x >= 0 && x < DISPLAY_WIDTH && y >= 0 && y < DISPLAY_HEIGHT)
        {
                int byte_index = (y / 8) * DISPLAY_WIDTH + x;
                int bit_index = y % 8;
                framebuffer[byte_index] |= (1 << bit_index);
        }
}

// Clear a pixel (set to black)
void clear_pixel(int x, int y)
{
        if (x >= 0 && x < DISPLAY_WIDTH && y >= 0 && y < DISPLAY_HEIGHT)
        {
                int byte_index = (y / 8) * DISPLAY_WIDTH + x;
                int bit_index = y % 8;
                framebuffer[byte_index] &= ~(1 << bit_index);
        }
}

// Draw a complete white ellipse (the full eye)
void draw_full_eye(int cx, int cy, int rx, int ry)
{
        for (int y = -ry; y <= ry; y++)
        {
                for (int x = -rx; x <= rx; x++)
                {
                        // Check if (x,y) lies inside the ellipse
                        if ((x * x * ry * ry + y * y * rx * rx) <= (rx * rx * ry * ry))
                        {
                                set_pixel(cx + x, cy + y);
                        }
                }
        }
}

// Draw eyelid by clearing (making black) the upper part of the eye.
// eyelid_offset: number of pixels from the top (of the eye ellipse) to clear.
// For example, 0 means no eyelid drawn, larger values simulate a more closed upper eyelid.
void draw_eyelid(int cx, int cy, int rx, int ry, int eyelid_offset)
{
        // Calculate the top of the ellipse (y-coordinate)
        int top = cy - ry;
        // Determine the cutoff line (all pixels above this line will be cleared)
        int cutoff = top + eyelid_offset;

        // Iterate over the eye ellipse and clear pixels above cutoff
        for (int y = -ry; y <= ry; y++)
        {
                int abs_y = cy + y;
                if (abs_y < cutoff)
                { // If above the cutoff, clear this pixel if it belongs to the ellipse
                        for (int x = -rx; x <= rx; x++)
                        {
                                if ((x * x * ry * ry + y * y * rx * rx) <= (rx * rx * ry * ry))
                                {
                                        clear_pixel(cx + x, cy + y);
                                }
                        }
                }
        }
}

// Draw a black circular pupil
void draw_pupil(int cx, int cy, int radius)
{
        for (int y = -radius; y <= radius; y++)
        {
                for (int x = -radius; x <= radius; x++)
                {
                        if ((x * x + y * y) <= (radius * radius))
                        {
                                clear_pixel(cx + x, cy + y);
                        }
                }
        }
}

// Write the framebuffer to the display
void update_display(void)
{
        display_write(display_dev, 0, 0, &desc, framebuffer);
}

// Draw one eye by combining the full eye, eyelid (upper expression), and pupil.
void draw_eye(int cx, int cy, int rx, int ry, int eyelid_offset, int pupil_radius)
{
        // Draw complete white eye
        draw_full_eye(cx, cy, rx, ry);
        // Overlay the eyelid on the upper part
        draw_eyelid(cx, cy, rx, ry, eyelid_offset);
        // Draw the pupil (will be visible if not covered by eyelid)
        draw_pupil(cx, cy, pupil_radius);
}

void blink_eyes(void)
{
        int pupil_offset_x = 0; // Horizontal offset of the pupil
        int pupil_offset_y = 0; // Vertical offset of the pupil

        // Use system uptime as a seed for random number generator
        uint32_t time_ms = k_uptime_get(); // Time in milliseconds since boot
        srand(time_ms);                    // Seed the random number generator

        // Move pupil in random directions
        for (int i = 0; i < 5; i++) // Perform 5 random movements before blinking
        {
                // Randomly select an offset for the pupil (in a range of -2 to 2 for both x and y)
                pupil_offset_x = rand() % 5 - 2; // Generates a value between -2 and 2
                pupil_offset_y = rand() % 5 - 2; // Generates a value between -2 and 2

                // Clear the framebuffer and draw eyes with the new pupil offsets
                clear_framebuffer();
                draw_eye(LEFT_EYE_X, LEFT_EYE_Y, EYE_RADIUS_X, EYE_RADIUS_Y, 0, PUPIL_RADIUS + pupil_offset_x);
                draw_eye(RIGHT_EYE_X, RIGHT_EYE_Y, EYE_RADIUS_X, EYE_RADIUS_Y, 0, PUPIL_RADIUS + pupil_offset_y);
                update_display();
                k_msleep(150); // Small delay for random movement effect
        }

        // Blink by closing both eyes (only the eyelids, not the pupils)
        clear_framebuffer();
        // Draw eyes with eyelids closed (set a high Y offset for eyelids)
        draw_eye(LEFT_EYE_X, LEFT_EYE_Y, EYE_RADIUS_X, EYE_RADIUS_Y, 40, PUPIL_RADIUS);   // Eyelids closed
        draw_eye(RIGHT_EYE_X, RIGHT_EYE_Y, EYE_RADIUS_X, EYE_RADIUS_Y, 40, PUPIL_RADIUS); // Eyelids closed
        update_display();
        k_msleep(200); // Blink duration (while eyelids are closed)

        // Reopen eyes (only the eyelids, pupils stay the same)
        clear_framebuffer();
        // Draw eyes with eyelids open (reset Y offset to 0 for eyelids)
        draw_eye(LEFT_EYE_X, LEFT_EYE_Y, EYE_RADIUS_X, EYE_RADIUS_Y, 0, PUPIL_RADIUS);   // Eyelids open
        draw_eye(RIGHT_EYE_X, RIGHT_EYE_Y, EYE_RADIUS_X, EYE_RADIUS_Y, 0, PUPIL_RADIUS); // Eyelids open
        update_display();
}

void update_eye_expression(void)
{
        int eyelid_offset;

        // Map CO₂ level (0-5000 ppm) to expression stage (0-4)
        if (co2_val.val1 <= 1000)
        {
                expression_stage = 0; // Normal eye (fully open)
        }
        else if (co2_val.val1 <= 2000)
        {
                expression_stage = 1; // Slightly squinted
        }
        else if (co2_val.val1 <= 3000)
        {
                expression_stage = 2; // Mostly closed
        }
        else if (co2_val.val1 <= 4000)
        {
                expression_stage = 3; // Moderately squinted
        }
        else
        {
                expression_stage = 4; // Wide open (same as normal)
        }

        // Set eyelid offset based on expression_stage
        switch (expression_stage)
        {
        case 0:
                eyelid_offset = 0; // Fully open
                break;
        case 1:
                eyelid_offset = 10; // Slightly squinted
                break;
        case 2:
                eyelid_offset = 20; // Mostly closed
                break;
        case 3:
                eyelid_offset = 30; // Moderately squinted
                break;
        case 4:
                eyelid_offset = 40; // Wide open
                break;
        default:
                eyelid_offset = 0;
                break;
        }

        clear_framebuffer();

        // Draw both eyes with the selected expression
        draw_eye(LEFT_EYE_X, LEFT_EYE_Y, EYE_RADIUS_X, EYE_RADIUS_Y, eyelid_offset, PUPIL_RADIUS);
        draw_eye(RIGHT_EYE_X, RIGHT_EYE_Y, EYE_RADIUS_X, EYE_RADIUS_Y, eyelid_offset, PUPIL_RADIUS);

        update_display();
        // If expression is normal (0), blink every 2 seconds
        if (expression_stage == 0)
        {
                blink_eyes();
        }

        // Sleep before checking again
        // k_msleep(5000);
}

void display_sensor_data()
{
        char display_str[48]; // Increased buffer size

        cfb_framebuffer_clear(display_dev, false);
        uint8_t font_height;
        cfb_get_font_size(display_dev, 0, NULL, &font_height);

        // Print Lux value
        snprintf(display_str, sizeof(display_str), "Lux: %d", lux_val.val1);
        if (cfb_print(display_dev, display_str, 0, 0) != 0)
        {
                LOG_ERR("Failed to print Lux data");
        }

        // Print Temperature & Humidity
        snprintf(display_str, sizeof(display_str), "T:%.1fC H:%.1f%%",
                 sensor_value_to_double(&temp_val), sensor_value_to_double(&hum_val));
        if (cfb_print(display_dev, display_str, 0, font_height + 2) != 0)
        {
                LOG_ERR("Failed to print Temp/Humidity data");
        }

        // Print CO2 value
        snprintf(display_str, sizeof(display_str), "CO2:%d ppm", co2_val.val1);
        if (cfb_print(display_dev, display_str, 0, 2 * font_height + 2) != 0)
        {
                LOG_ERR("Failed to print CO2 data");
        }

        // cfb_framebuffer_invert(display_dev);
        cfb_framebuffer_finalize(display_dev);
}

void main(void)
{
        LOG_INF("Starting Node");
        display_dev = DEVICE_DT_GET(DT_NODELABEL(ssd1306));
        vcnl = DEVICE_DT_GET_ANY(vishay_vcnl4040);
        sht = DEVICE_DT_GET_ANY(sensirion_sht4x);
        co2 = DEVICE_DT_GET_ONE(winsen_mhz19b);
        tof = DEVICE_DT_GET_ANY(st_vl53l0x);
        k_msleep(5000);

        if (!device_is_ready(display_dev) || !device_is_ready(sht) || !device_is_ready(vcnl) || !device_is_ready(tof))
        {
                LOG_ERR("One or more essential devices (display, SHT, VCNL, TOF) are not ready.");
                return;
        }

        // Check CO₂ sensor separately
        if (!device_is_ready(co2))
        {
                LOG_WRN("CO₂ sensor not ready, continuing without it.");
                sys_reboot(SYS_REBOOT_WARM);
        }
        else
        {
                LOG_INF("CO₂ sensor is ready.");
        }

        LOG_INF("All sensors are ready");

        cfb_framebuffer_init(display_dev);
        display_blanking_off(display_dev);

        while (1)
        {
                if (sensor_sample_fetch(tof) == 0 && sensor_channel_get(tof, SENSOR_CHAN_DISTANCE, &tof_val) == 0)
                {
                        distance = sensor_value_to_double(&tof_val) * 100.0;
                }

                if (distance < 20)
                {
                        display_sensor_data();
                        k_msleep(3000);
                }
                else
                {

                        update_eye_expression();
                }
                if (sensor_sample_fetch(sht) == 0)
                {
                        sensor_channel_get(sht, SENSOR_CHAN_AMBIENT_TEMP, &temp_val);
                        sensor_channel_get(sht, SENSOR_CHAN_HUMIDITY, &hum_val);
                }

                if (sensor_sample_fetch(vcnl) == 0)
                {
                        sensor_channel_get(vcnl, SENSOR_CHAN_LIGHT, &lux_val);
                        LOG_INF("lUX: %d lumininus ", lux_val.val1);
                        LOG_INF("Distance: %d ", distance);
                }

                if (sensor_sample_fetch(co2) == 0)
                {
                        sensor_channel_get(co2, SENSOR_CHAN_CO2, &co2_val);
                        LOG_INF("CO₂ Value: %d ppm", co2_val.val1);
                }

                // Update display only when proximity value changes
                k_msleep(SLEEP_DURATION_MS);
        }
}




// /* Enviroment Monitoring Device*/

// #include <zephyr/device.h>
// #include <zephyr/kernel.h>
// #include <zephyr/drivers/display.h>
// #include <zephyr/kernel.h>
// #include <zephyr/logging/log.h>
// #include <zephyr/drivers/sensor.h>
// #include <zephyr/drivers/sensor/mhz19b.h>
// #include <stdlib.h>
// #include <time.h>
// #include <string.h>
// #include <zephyr/sys/reboot.h>

// #define DISPLAY_WIDTH 128
// #define DISPLAY_HEIGHT 64
// #define FRAMEBUFFER_SIZE (DISPLAY_WIDTH * DISPLAY_HEIGHT / 8)
// #define SLEEP_DURATION_MS 100
// #define PROX_THRESHOLD 20

// static const struct device *display_dev, *sht, *vcnl, *co2, *tof;
// struct sensor_value lux_val, temp_val, hum_val, prox_val, co2_val, tof_val;
// char display_str[32];
// static int distance = 0;
// static int expression_stage = 0;
// int prev_distance = 0;           // Store the previous proximity value
// static bool reboot_done = false; // Track if reboot has happened

// #define LEFT_EYE_X 40
// #define LEFT_EYE_Y 32
// #define RIGHT_EYE_X 88
// #define RIGHT_EYE_Y 32
// #define EYE_RADIUS_X 22
// #define EYE_RADIUS_Y 28
// #define PUPIL_RADIUS 10

// static uint8_t framebuffer[FRAMEBUFFER_SIZE];

// struct display_buffer_descriptor desc = {
//     .buf_size = FRAMEBUFFER_SIZE,
//     .width = DISPLAY_WIDTH,
//     .height = DISPLAY_HEIGHT,
//     .pitch = DISPLAY_WIDTH,
// };

// LOG_MODULE_REGISTER(ssd1306_draw_eyes);

// // Clear entire framebuffer
// void clear_framebuffer(void)
// {
//         memset(framebuffer, 0, sizeof(framebuffer));
// }

// void set_pixel(int x, int y)
// {
//         if (x >= 0 && x < DISPLAY_WIDTH && y >= 0 && y < DISPLAY_HEIGHT)
//         {
//                 int byte_index = (y / 8) * DISPLAY_WIDTH + x;
//                 int bit_index = y % 8;
//                 framebuffer[byte_index] |= (1 << bit_index);
//         }
// }

// void clear_pixel(int x, int y)
// {
//         if (x >= 0 && x < DISPLAY_WIDTH && y >= 0 && y < DISPLAY_HEIGHT)
//         {
//                 int byte_index = (y / 8) * DISPLAY_WIDTH + x;
//                 int bit_index = y % 8;
//                 framebuffer[byte_index] &= ~(1 << bit_index);
//         }
// }

// void draw_full_eye(int cx, int cy, int rx, int ry)
// {
//         for (int y = -ry; y <= ry; y++)
//         {
//                 for (int x = -rx; x <= rx; x++)
//                 {
//                         if ((x * x * ry * ry + y * y * rx * rx) <= (rx * rx * ry * ry))
//                         {
//                                 set_pixel(cx + x, cy + y);
//                         }
//                 }
//         }
// }

// void draw_eyelid(int cx, int cy, int rx, int ry, int eyelid_offset)
// {
//         int top = cy - ry;
//         int cutoff = top + eyelid_offset;

//         for (int y = -ry; y <= ry; y++)
//         {
//                 int abs_y = cy + y;
//                 if (abs_y < cutoff)
//                 {
//                         for (int x = -rx; x <= rx; x++)
//                         {
//                                 if ((x * x * ry * ry + y * y * rx * rx) <= (rx * rx * ry * ry))
//                                 {
//                                         clear_pixel(cx + x, cy + y);
//                                 }
//                         }
//                 }
//         }
// }

// void draw_pupil(int cx, int cy, int radius)
// {
//         for (int y = -radius; y <= radius; y++)
//         {
//                 for (int x = -radius; x <= radius; x++)
//                 {
//                         if ((x * x + y * y) <= (radius * radius))
//                         {
//                                 clear_pixel(cx + x, cy + y);
//                         }
//                 }
//         }
// }

// void update_display(void)
// {
//         display_write(display_dev, 0, 0, &desc, framebuffer);
// }

// void draw_eye(int cx, int cy, int rx, int ry, int eyelid_offset, int pupil_offset_x, int pupil_offset_y, int pupil_radius)
// {
//         draw_full_eye(cx, cy, rx, ry);
//         draw_eyelid(cx, cy, rx, ry, eyelid_offset);
//         draw_pupil(cx + pupil_offset_x, cy + pupil_offset_y, pupil_radius);
// }

// void draw_both_eyes(int eyelid_offset, int pupil_offset_x, int pupil_offset_y)
// {
//         clear_framebuffer();
//         draw_eye(LEFT_EYE_X, LEFT_EYE_Y, EYE_RADIUS_X, EYE_RADIUS_Y, eyelid_offset, pupil_offset_x, pupil_offset_y, PUPIL_RADIUS);
//         draw_eye(RIGHT_EYE_X, RIGHT_EYE_Y, EYE_RADIUS_X, EYE_RADIUS_Y, eyelid_offset, pupil_offset_x, pupil_offset_y, PUPIL_RADIUS);
//         update_display();
// }

// void draw_eye_movement_sequence(int eyelid_offset)
// {
//         // Center
//         draw_both_eyes(eyelid_offset, 0, 0);
//         k_msleep(500);

//         // Left
//         draw_both_eyes(eyelid_offset, -5, 0);
//         k_msleep(500);

//         // Right
//         draw_both_eyes(eyelid_offset, 5, 0);
//         k_msleep(500);

//         // Up
//         draw_both_eyes(eyelid_offset, 0, -5);
//         k_msleep(500);

//         // Down
//         draw_both_eyes(eyelid_offset, 0, 5);
//         k_msleep(500);

//         // Center again
//         draw_both_eyes(eyelid_offset, 0, 0);
//         k_msleep(500);
// }

// void animate_pupil_movement(void)
// {
//         draw_eye_movement_sequence(0); // eyelid_offset = 0
// }

// void animate_blink(int eyelid_offset)
// {
//         // Simulate closing by increasing the eyelid offset
//         int blink_closed_offset = eyelid_offset + 20;

//         // Clamp to a max if needed
//         if (blink_closed_offset > 40)
//                 blink_closed_offset = 40;

//         draw_both_eyes(blink_closed_offset, 0, 0); // Closed
//         k_msleep(100);
//         draw_both_eyes(eyelid_offset, 0, 0); // Back to original
//         k_msleep(100);
// }

// void update_eye_expression(void)
// {
//         int eyelid_offset;

//         if (co2_val.val1 <= 1000)
//                 expression_stage = 0;
//         else if (co2_val.val1 <= 2000)
//                 expression_stage = 1;
//         else if (co2_val.val1 <= 3000)
//                 expression_stage = 2;
//         else if (co2_val.val1 <= 4000)
//                 expression_stage = 3;
//         else
//                 expression_stage = 4;

//         switch (expression_stage)
//         {
//         case 0:
//                 eyelid_offset = 0;
//                 animate_pupil_movement();
//                 animate_blink(eyelid_offset);
//                 break;

//         case 1:
//                 eyelid_offset = 10;
//                 draw_eye_movement_sequence(eyelid_offset);
//                 animate_blink(eyelid_offset);
//                 break;

//         case 2:
//                 eyelid_offset = 20;
//                 draw_eye_movement_sequence(eyelid_offset);
//                 animate_blink(eyelid_offset);
//                 break;

//         case 3:
//                 eyelid_offset = 30;
//                 animate_blink(eyelid_offset);
//                 break;

//         case 4:
//                 eyelid_offset = 40;
//                 animate_blink(eyelid_offset);
//                 break;

//         default:
//                 eyelid_offset = 0;
//                 break;
//         }

//         if (expression_stage > 0)
//         {
//                 draw_both_eyes(eyelid_offset, 0, 0);
//                 k_msleep(5000);
//         }
//         k_msleep(10000);
// }

// void display_sensor_data()
// {
//         char display_str[48]; // Increased buffer size

//         cfb_framebuffer_clear(display_dev, false);
//         uint8_t font_height;
//         cfb_get_font_size(display_dev, 0, NULL, &font_height);

//         // Print Lux value
//         snprintf(display_str, sizeof(display_str), "Lux: %d", lux_val.val1);
//         if (cfb_print(display_dev, display_str, 0, 0) != 0)
//         {
//                 LOG_ERR("Failed to print Lux data");
//         }

//         // Print Temperature & Humidity
//         snprintf(display_str, sizeof(display_str), "T:%.1fC H:%.1f%%",
//                  sensor_value_to_double(&temp_val), sensor_value_to_double(&hum_val));
//         if (cfb_print(display_dev, display_str, 0, font_height + 2) != 0)
//         {
//                 LOG_ERR("Failed to print Temp/Humidity data");
//         }

//         // Print CO2 value
//         snprintf(display_str, sizeof(display_str), "CO2:%d ppm", co2_val.val1);
//         if (cfb_print(display_dev, display_str, 0, 2 * font_height + 2) != 0)
//         {
//                 LOG_ERR("Failed to print CO2 data");
//         }

//         // cfb_framebuffer_invert(display_dev);
//         cfb_framebuffer_finalize(display_dev);
// }

// void main(void)
// {
//         LOG_INF("Starting Node");
//         display_dev = DEVICE_DT_GET(DT_NODELABEL(ssd1306));
//         vcnl = DEVICE_DT_GET_ANY(vishay_vcnl4040);
//         sht = DEVICE_DT_GET_ANY(sensirion_sht4x);
//         co2 = DEVICE_DT_GET_ONE(winsen_mhz19b);
//         tof = DEVICE_DT_GET_ANY(st_vl53l0x);
//         k_msleep(5000);

//         if (!device_is_ready(display_dev) || !device_is_ready(sht) || !device_is_ready(vcnl) || !device_is_ready(tof))
//         {
//                 LOG_ERR("One or more essential devices (display, SHT, VCNL, TOF) are not ready.");
//                 return;
//         }

//         // Check CO₂ sensor separately
//         if (!device_is_ready(co2))
//         {
//                 LOG_WRN("CO₂ sensor not ready, continuing without it.");
//                 sys_reboot(SYS_REBOOT_WARM);
//         }
//         else
//         {
//                 LOG_INF("CO₂ sensor is ready.");
//         }

//         LOG_INF("All sensors are ready");

//         cfb_framebuffer_init(display_dev);
//         display_blanking_off(display_dev);

//         while (1)
//         {
//                 // Always fetch TOF distance first
//                 if (sensor_sample_fetch(tof) == 0 &&
//                     sensor_channel_get(tof, SENSOR_CHAN_DISTANCE, &tof_val) == 0)
//                 {
//                         distance = sensor_value_to_double(&tof_val) * 100.0;
//                 }

//                 if (distance < 20)
//                 {
//                         // Immediately react to close object
//                         display_sensor_data();
//                         k_msleep(3000); // delay to avoid flickering / unnecessary updates
//                         continue;       // skip other updates, restart loop
//                 }

//                 // Continue with regular updates only if no nearby object
//                 update_eye_expression();

//                 // SHT sensor
//                 if (sensor_sample_fetch(sht) == 0)
//                 {
//                         sensor_channel_get(sht, SENSOR_CHAN_AMBIENT_TEMP, &temp_val);
//                         sensor_channel_get(sht, SENSOR_CHAN_HUMIDITY, &hum_val);
//                 }

//                 // VCNL sensor
//                 if (sensor_sample_fetch(vcnl) == 0)
//                 {
//                         sensor_channel_get(vcnl, SENSOR_CHAN_LIGHT, &lux_val);
//                         LOG_INF("Lux: %d", lux_val.val1);
//                         LOG_INF("Distance: %d mm", distance);
//                 }

//                 // CO2 sensor
//                 if (sensor_sample_fetch(co2) == 0)
//                 {
//                         sensor_channel_get(co2, SENSOR_CHAN_CO2, &co2_val);
//                         LOG_INF("CO₂: %d ppm", co2_val.val1);
//                 }

//                 // Sleep before next loop iteration
//                 k_msleep(SLEEP_DURATION_MS);
//         }
// }