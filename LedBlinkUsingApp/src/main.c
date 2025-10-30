// /* lED Configuration using Non connectable mode*/

// #include <zephyr/kernel.h>
// #include <zephyr/device.h>
// #include <zephyr/drivers/gpio.h>
// #include <zephyr/bluetooth/bluetooth.h>
// #include <zephyr/bluetooth/hci.h>
// #include <zephyr/logging/log.h>

// LOG_MODULE_REGISTER(scanner, LOG_LEVEL_INF);
// #define SENSOR_ADDRESS "C4:71:0F:19:89:F1" // Static Bluetooth Address
// /* Define LED node from devicetree alias */
// #define LED0_NODE DT_ALIAS(led0)
// static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED0_NODE, gpios);

// /* --- Parse Advertisement Callback --- */
// static bool ad_parse_cb(struct bt_data *data, void *user_data)
// {
//     if (data->type == BT_DATA_MANUFACTURER_DATA)
//     {
//         const uint8_t *payload = data->data;
//         size_t len = data->data_len;

//         // LOG_INF("MFG data received, len=%d", len);

//         if (len >= 2 && payload[0] == 0x59)
//         {
//             uint8_t cmd = payload[2];

//             switch (cmd)
//             {
//             case 0x01:
//                 LOG_INF("Command: Turn LED ON");
//                 gpio_pin_set_dt(&led, 1);
//                 break;

//             case 0x02:
//                 LOG_INF("Command: Turn LED OFF");
//                 gpio_pin_set_dt(&led, 0);
//                 break;

//             default:
//                 LOG_INF("Unknown command: %02X", cmd);
//                 break;
//             }
//         }
//     }
//     return true; // continue parsing
// }

// /* --- Device Found Callback --- */
// static void device_found(const bt_addr_le_t *addr, int8_t rssi,
//                          uint8_t type, struct net_buf_simple *ad)
// {
//     /* With Filter Accept List enabled, we’ll only get the whitelisted device */
//     if (type == BT_GAP_ADV_TYPE_ADV_IND ||
//         type == BT_GAP_ADV_TYPE_ADV_NONCONN_IND)
//     {
//         bt_data_parse(ad, ad_parse_cb, NULL);
//     }
// }

// /* --- Start Observer with Filter Accept List --- */
// int observer_start(void)
// {
//     int err;

//     /* Scan parameters */
//     struct bt_le_scan_param scan_param = {
//         .type = BT_LE_SCAN_TYPE_PASSIVE,
//         .options = NULL,
//         .interval = 0x0030, // ~30 ms
//         .window = 0x0030,   // scan continuously
//     };

//     /* Start scanning with device_found callback */
//     err = bt_le_scan_start(&scan_param, device_found);
//     if (err)
//     {
//         LOG_ERR("Start scanning failed (err %d)", err);
//         return err;
//     }

//     LOG_INF("Started scanning (Filter Accept List mode)...");
//     return 0;
// }

// void main(void)
// {
//     int err;

//     if (!device_is_ready(led.port))
//     {
//         LOG_ERR("LED device not ready");
//         return;
//     }
//     gpio_pin_configure_dt(&led, GPIO_OUTPUT_INACTIVE);

//     err = bt_enable(NULL);
//     if (err)
//     {
//         LOG_ERR("Bluetooth init failed (err %d)", err);
//         return;
//     }

//     LOG_INF("Bluetooth initialized, starting observer...");
//     observer_start();
// }

// /* LED Configuration using Non-connectable mode + Manufacturer Data commands */

// #include <zephyr/kernel.h>
// #include <zephyr/device.h>
// #include <zephyr/drivers/gpio.h>
// #include <zephyr/bluetooth/bluetooth.h>
// #include <zephyr/bluetooth/hci.h>
// #include <zephyr/logging/log.h>

// LOG_MODULE_REGISTER(scanner, LOG_LEVEL_INF);

// /* --- Define 4 LEDs from devicetree aliases --- */
// #define LED0_NODE DT_ALIAS(led0)
// #define LED1_NODE DT_ALIAS(led1)
// #define LED2_NODE DT_ALIAS(led2)
// #define LED3_NODE DT_ALIAS(led3)

// static const struct gpio_dt_spec leds[] = {
//     GPIO_DT_SPEC_GET(LED0_NODE, gpios),
//     GPIO_DT_SPEC_GET(LED1_NODE, gpios),
//     GPIO_DT_SPEC_GET(LED2_NODE, gpios),
//     GPIO_DT_SPEC_GET(LED3_NODE, gpios),
// };

// #define NUM_LEDS ARRAY_SIZE(leds)

// /* --- Helper: set all LEDs --- */
// static void set_all_leds(int state)
// {
//     for (int i = 0; i < NUM_LEDS; i++)
//     {
//         gpio_pin_set_dt(&leds[i], state);
//     }
// }

// /* --- LED Patterns --- */
// static void led_pattern(uint8_t cmd)
// {
//     switch (cmd)
//     {
//     case 0x01:
//         LOG_INF("Pattern 1: All LEDs ON");
//         set_all_leds(1);
//         break;

//     case 0x02:
//         LOG_INF("Pattern 2: All LEDs OFF");
//         set_all_leds(0);
//         break;

//     case 0x03:
//         LOG_INF("Pattern 3: Chase sequence");
//         for (int i = 0; i < NUM_LEDS; i++)
//         {
//             set_all_leds(0);
//             gpio_pin_set_dt(&leds[i], 1);
//             k_sleep(K_MSEC(200));
//         }
//         set_all_leds(0);
//         break;

//     case 0x04:
//         LOG_INF("Pattern 4: Blink all LEDs");
//         for (int i = 0; i < 3; i++)
//         {
//             set_all_leds(1);
//             k_sleep(K_MSEC(300));
//             set_all_leds(0);
//             k_sleep(K_MSEC(300));
//         }
//         break;

//     case 0x05:
//         LOG_INF("Pattern 5: Alternate LEDs");
//         for (int i = 0; i < 4; i++)
//         {
//             gpio_pin_set_dt(&leds[0], i % 2);
//             gpio_pin_set_dt(&leds[2], i % 2);
//             gpio_pin_set_dt(&leds[1], !(i % 2));
//             gpio_pin_set_dt(&leds[3], !(i % 2));
//             k_sleep(K_MSEC(400));
//         }
//         set_all_leds(0);
//         break;

//     default:
//         LOG_INF("Unknown command: 0x%02X", cmd);
//         break;
//     }
// }

// /* --- Parse Advertisement Callback --- */
// static bool ad_parse_cb(struct bt_data *data, void *user_data)
// {
//     if (data->type == BT_DATA_MANUFACTURER_DATA)
//     {
//         const uint8_t *payload = data->data;
//         size_t len = data->data_len;

//         if (len >= 2 && payload[0] == 0x59)
//         { // header check
//             uint8_t cmd = payload[2];
//             led_pattern(cmd);
//         }
//     }
//     return true; // continue parsing
// }

// /* --- Device Found Callback --- */
// static void device_found(const bt_addr_le_t *addr, int8_t rssi,
//                          uint8_t type, struct net_buf_simple *ad)
// {
//     if (type == BT_GAP_ADV_TYPE_ADV_IND ||
//         type == BT_GAP_ADV_TYPE_ADV_NONCONN_IND)
//     {
//         bt_data_parse(ad, ad_parse_cb, NULL);
//     }
// }

// /* --- Start Observer --- */
// int observer_start(void)
// {
//     struct bt_le_scan_param scan_param = {
//         .type = BT_LE_SCAN_TYPE_PASSIVE,
//         .options = 0,
//         .interval = 0x0030, // ~30 ms
//         .window = 0x0030,
//     };

//     int err = bt_le_scan_start(&scan_param, device_found);
//     if (err)
//     {
//         LOG_ERR("Start scanning failed (err %d)", err);
//         return err;
//     }

//     LOG_INF("Started scanning...");
//     return 0;
// }

// void main(void)
// {
//     int err;

//     /* Init LEDs */
//     for (int i = 0; i < NUM_LEDS; i++)
//     {
//         if (!device_is_ready(leds[i].port))
//         {
//             LOG_ERR("LED %d not ready", i);
//             return;
//         }
//         gpio_pin_configure_dt(&leds[i], GPIO_OUTPUT_INACTIVE);
//     }

//     /* Enable Bluetooth */
//     err = bt_enable(NULL);
//     if (err)
//     {
//         LOG_ERR("Bluetooth init failed (err %d)", err);
//         return;
//     }

//     LOG_INF("Bluetooth initialized, starting observer...");
//     observer_start();
// }

// /* LED Interval Control using Non-connectable mode + Manufacturer Data commands */

// #include <zephyr/kernel.h>
// #include <zephyr/device.h>
// #include <zephyr/drivers/gpio.h>
// #include <zephyr/bluetooth/bluetooth.h>
// #include <zephyr/bluetooth/hci.h>
// #include <zephyr/logging/log.h>

// LOG_MODULE_REGISTER(scanner, LOG_LEVEL_INF);

// /* --- Define 4 LEDs from devicetree aliases --- */
// #define LED0_NODE DT_ALIAS(led0)
// #define LED1_NODE DT_ALIAS(led1)
// #define LED2_NODE DT_ALIAS(led2)
// #define LED3_NODE DT_ALIAS(led3)

// static const struct gpio_dt_spec leds[] = {
//     GPIO_DT_SPEC_GET(LED0_NODE, gpios),
//     GPIO_DT_SPEC_GET(LED1_NODE, gpios),
//     GPIO_DT_SPEC_GET(LED2_NODE, gpios),
//     GPIO_DT_SPEC_GET(LED3_NODE, gpios),
// };

// #define NUM_LEDS ARRAY_SIZE(leds)

// /* --- Current blink interval (default 500 ms) --- */
// static int blink_interval_ms = 500;

// /* --- Helper: set all LEDs --- */
// static void set_all_leds(int state)
// {
//     for (int i = 0; i < NUM_LEDS; i++)
//     {
//         gpio_pin_set_dt(&leds[i], state);
//     }
// }

// /* --- LED Blink Task --- */
// void led_blink_thread(void)
// {
//     while (1)
//     {
//         set_all_leds(1);
//         k_sleep(K_MSEC(blink_interval_ms));
//         set_all_leds(0);
//         k_sleep(K_MSEC(blink_interval_ms));
//     }
// }
// K_THREAD_DEFINE(led_blink_tid, 1024, led_blink_thread, NULL, NULL, NULL,
//                 7, 0, 0);

// /* --- Parse Advertisement Callback --- */
// static bool ad_parse_cb(struct bt_data *data, void *user_data)
// {
//     if (data->type == BT_DATA_MANUFACTURER_DATA)
//     {
//         const uint8_t *payload = data->data;
//         size_t len = data->data_len;

//         if (len >= 3 && payload[0] == 0x59)
//         {
//             uint8_t cmd = payload[2];
//             uint8_t value = payload[3];

//             if (cmd == 0x01)
//             {
//                 int old_interval = blink_interval_ms;
//                 int new_interval = value * 100; // scale: 100ms steps

//                 if (new_interval < 100)
//                 {
//                     new_interval = 100; // safeguard
//                 }

//                 LOG_INF("Blink interval change: OLD=%d ms -> NEW=%d ms",
//                         old_interval, new_interval);

//                 blink_interval_ms = new_interval;
//             }
//         }
//     }
//     return true; // continue parsing
// }

// /* --- Device Found Callback --- */
// static void device_found(const bt_addr_le_t *addr, int8_t rssi,
//                          uint8_t type, struct net_buf_simple *ad)
// {
//     if (type == BT_GAP_ADV_TYPE_ADV_IND ||
//         type == BT_GAP_ADV_TYPE_ADV_NONCONN_IND)
//     {
//         bt_data_parse(ad, ad_parse_cb, NULL);
//     }
// }

// /* --- Start Observer --- */
// int observer_start(void)
// {
//     struct bt_le_scan_param scan_param = {
//         .type = BT_LE_SCAN_TYPE_PASSIVE,
//         .options = 0,
//         .interval = 0x0030,
//         .window = 0x0030,
//     };

//     int err = bt_le_scan_start(&scan_param, device_found);
//     if (err)
//     {
//         LOG_ERR("Start scanning failed (err %d)", err);
//         return err;
//     }

//     LOG_INF("Started scanning...");
//     return 0;
// }

// void main(void)
// {
//     int err;

//     /* Init LEDs */
//     for (int i = 0; i < NUM_LEDS; i++)
//     {
//         if (!device_is_ready(leds[i].port))
//         {
//             LOG_ERR("LED %d not ready", i);
//             return;
//         }
//         gpio_pin_configure_dt(&leds[i], GPIO_OUTPUT_INACTIVE);
//     }

//     /* Enable Bluetooth */
//     err = bt_enable(NULL);
//     if (err)
//     {
//         LOG_ERR("Bluetooth init failed (err %d)", err);
//         return;
//     }

//     LOG_INF("Bluetooth initialized, starting observer...");
//     observer_start();
// }

///////////////////////

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(ble_state, LOG_LEVEL_INF);

/* --- Define 4 LEDs from devicetree aliases --- */
#define LED0_NODE DT_ALIAS(led0)
#define LED1_NODE DT_ALIAS(led1)
#define LED2_NODE DT_ALIAS(led2)
#define LED3_NODE DT_ALIAS(led3)

static const struct gpio_dt_spec leds[] = {
    GPIO_DT_SPEC_GET(LED0_NODE, gpios),
    GPIO_DT_SPEC_GET(LED1_NODE, gpios),
    GPIO_DT_SPEC_GET(LED2_NODE, gpios),
    GPIO_DT_SPEC_GET(LED3_NODE, gpios),
};
int err;
#define DEVICE_NAME CONFIG_BT_DEVICE_NAME
#define DEVICE_NAME_LEN (sizeof(DEVICE_NAME) - 1)
#define NUM_LEDS ARRAY_SIZE(leds)
#define SENSOR_ADDRESS "DE:AD:BE:AF:BA:11" // Static Bluetooth Address
/* --- Current blink interval (default 500 ms) --- */
static int blink_interval_ms = 500;

/* --- BLE State Management --- */
typedef enum
{
    MODE_ADVERTISING,
    MODE_SCANNING
} ble_mode_t;

static ble_mode_t current_mode = MODE_ADVERTISING;

// Global pointer for the advertising set
struct bt_le_ext_adv *adv;

// The manufacturer data buffer to hold the payload.
uint8_t mfg_data[10] = {0};

// Global advertising parameters
struct bt_le_adv_param adv_param =
    BT_LE_ADV_PARAM_INIT(BT_LE_ADV_OPT_NONE | BT_LE_ADV_OPT_EXT_ADV |
                             BT_LE_ADV_OPT_USE_IDENTITY | BT_LE_ADV_OPT_NO_2M,
                         0x30, 0x30, NULL);

// The advertisement structure referencing the global buffer.
static const struct bt_data ad[] = {
    BT_DATA(BT_DATA_MANUFACTURER_DATA, mfg_data, 10),
    BT_DATA(BT_DATA_NAME_COMPLETE, DEVICE_NAME, DEVICE_NAME_LEN),
};

/* --- BLE setup helpers --- */
void set_random_static_address(void)
{

    bt_addr_le_t addr;
    // converts the string into a binary address and stores this address in a buffer whose address is addr.
    err = bt_addr_le_from_str(SENSOR_ADDRESS, "random", &addr); // Only random static address can be given when the type is set to "random"
    if (err)
    {
        printk("Invalid BT address (err %d)\n", err);
    }
    // create a new Identity address. This must be done "before enabling Bluetooth" (before calling bt_enable). Addr is the address used for the new identity
    int err1 = bt_id_create(&addr, NULL);
    if (err1 < 0)
    {
        printk("Creating new ID failed (err %d)\n", err1);
    }
    printk("Created new address\n");
}

void adv_param_init(void)
{
    int err;

    err = bt_le_ext_adv_create(&adv_param, NULL, &adv); // Attempt to create an extended advertising set, 'NULL' indicating that there are no additional parameters or options provided.

    if (err) // Check if there was an error in creating the advertising set
    {
        printk("Failed to create advertising set (err %d)\n", err); // Print an error message indicating the failure to create the advertising set
        return;
    }
    printk("Created extended advertising set \n");
}

struct bt_le_scan_param scan_param = {
    .type = BT_LE_SCAN_TYPE_PASSIVE,
    .options = 0,
    .interval = BT_GAP_SCAN_FAST_INTERVAL,
    .window = BT_GAP_SCAN_FAST_WINDOW,
};
/* --- Helper: set all LEDs --- */
static void set_all_leds(int state)
{
    for (int i = 0; i < NUM_LEDS; i++)
    {
        gpio_pin_set_dt(&leds[i], state);
    }
}

/* --- LED Blink Task --- */
void led_blink_thread(void)
{
    while (1)
    {
        set_all_leds(1);
        k_sleep(K_MSEC(blink_interval_ms));
        set_all_leds(0);
        k_sleep(K_MSEC(blink_interval_ms));
    }
}
K_THREAD_DEFINE(led_blink_tid, 1024, led_blink_thread, NULL, NULL, NULL,
                7, 0, 0);

/* --- Parse Advertisement Callback --- */
static bool ad_parse_cb(struct bt_data *data, void *user_data)
{
    if (data->type == BT_DATA_MANUFACTURER_DATA)
    {
        const uint8_t *payload = data->data;
        size_t len = data->data_len;

        // Check for the expected company ID (0x0059) and command byte (0x01)
        if (len >= 4 && payload[0] == 0x59 && payload[1] == 0x00)
        {
            uint8_t cmd = payload[2];
            uint8_t value = payload[3];

            if (cmd == 0x01)
            {
                int old_interval = blink_interval_ms;
                int new_interval = value * 100; // scale: 100ms steps

                if (new_interval < 100)
                {
                    new_interval = 100; // safeguard
                }

                LOG_INF("Blink interval change: OLD=%d ms -> NEW=%d ms",
                        old_interval, new_interval);

                blink_interval_ms = new_interval;
            }
        }
    }
    return true; // continue parsing
}

/* --- Device Found Callback --- */
static void device_found(const bt_addr_le_t *addr, int8_t rssi,
                         uint8_t type, struct net_buf_simple *ad)
{
    if (type == BT_GAP_ADV_TYPE_ADV_IND ||
        type == BT_GAP_ADV_TYPE_ADV_NONCONN_IND)
    {
        bt_data_parse(ad, ad_parse_cb, NULL);
    }
}

/* --- Start Advertising --- */
static void start_advertising(void)
{
    // Update the value in the global buffer with the current blink interval
    mfg_data[3] = blink_interval_ms / 100;

    int err = bt_le_ext_adv_set_data(adv, ad, ARRAY_SIZE(ad), NULL, 0);
    if (err)
    {
        LOG_ERR("Failed to set advertising data (err %d)", err);
    }

    err = bt_le_ext_adv_start(adv, NULL);
    if (err)
    {
        LOG_ERR("Advertising failed to start (err %d)", err);
    }
    else
    {
        LOG_INF("Started advertising with interval %d ms", blink_interval_ms);
    }
}

/* --- Stop Advertising --- */
static void stop_advertising(void)
{
    int err = bt_le_ext_adv_stop(adv);
    if (err)
    {
        LOG_ERR("Advertising failed to stop (err %d)", err);
    }
    else
    {
        LOG_INF("Stopped advertising.");
    }
}

/* --- Start Observer --- */
static void observer_start(void)
{
    int err = bt_le_scan_start(&scan_param, device_found);
    if (err)
    {
        LOG_ERR("Start scanning failed (err %d)", err);
    }
    else
    {
        LOG_INF("Started scanning...");
    }
}

/* --- Main BLE state thread --- */
void ble_thread_handler(void)
{
    while (1)
    {
        if (current_mode == MODE_ADVERTISING)
        {
            start_advertising();
            k_sleep(K_MSEC(500)); // Advertise for a short period
            stop_advertising();
            current_mode = MODE_SCANNING;
        }
        else
        {
            observer_start();
            k_sleep(K_MSEC(500)); // Scan for a short period
            bt_le_scan_stop();
            current_mode = MODE_ADVERTISING;
        }
    }
}
K_THREAD_DEFINE(ble_handler_tid, 1024, ble_thread_handler, NULL, NULL, NULL,
                7, 0, 0);

void main(void)
{
    int err;

    /* Init LEDs */
    for (int i = 0; i < NUM_LEDS; i++)
    {
        if (!device_is_ready(leds[i].port))
        {
            LOG_ERR("LED %d not ready", i);
            return;
        }
        gpio_pin_configure_dt(&leds[i], GPIO_OUTPUT_INACTIVE);
    }

    /* Enable Bluetooth */
    err = bt_enable(NULL);
    if (err)
    {
        LOG_ERR("Bluetooth init failed (err %d)", err);
        return;
    }

    /* Create the advertising set once */
    set_random_static_address();
    adv_param_init();

    mfg_data[0] = 0x59; // Company ID LSB
    mfg_data[1] = 0x00; // Company ID MSB
    mfg_data[2] = 0x01; // Command: Set Blink Interval

    k_msleep(100); // Wait for Bluetooth stack to stabilize
    LOG_INF("Bluetooth initialized, starting BLE handler...");
}

//////////////////

// #include <zephyr/kernel.h>
// #include <zephyr/device.h>
// #include <zephyr/drivers/gpio.h>
// #include <zephyr/bluetooth/bluetooth.h>
// #include <zephyr/bluetooth/hci.h>
// #include <zephyr/logging/log.h>

// LOG_MODULE_REGISTER(ble_state, LOG_LEVEL_INF);

// /* --- Define 4 LEDs from devicetree aliases --- */
// #define LED0_NODE DT_ALIAS(led0)
// #define LED1_NODE DT_ALIAS(led1)
// #define LED2_NODE DT_ALIAS(led2)
// #define LED3_NODE DT_ALIAS(led3)

// static const struct gpio_dt_spec leds[] = {
//     GPIO_DT_SPEC_GET(LED0_NODE, gpios),
//     GPIO_DT_SPEC_GET(LED1_NODE, gpios),
//     GPIO_DT_SPEC_GET(LED2_NODE, gpios),
//     GPIO_DT_SPEC_GET(LED3_NODE, gpios),
// };

// #define DEVICE_NAME CONFIG_BT_DEVICE_NAME
// #define DEVICE_NAME_LEN (sizeof(DEVICE_NAME) - 1)
// #define NUM_LEDS ARRAY_SIZE(leds)
// #define SENSOR_ADDRESS "DE:AD:BE:AF:BA:11" // Static Bluetooth Address

// /* --- Current blink interval (default 500 ms) --- */
// static int blink_interval_ms = 500;

// /* --- BLE State Management --- */
// typedef enum
// {
//         MODE_ADVERTISING,
//         MODE_SCANNING
// } ble_mode_t;

// static ble_mode_t current_mode = MODE_ADVERTISING;

// // Global pointer for the advertising set
// struct bt_le_ext_adv *adv;

// // The manufacturer data buffer to hold the payload.
// uint8_t mfg_data[10] = {0};

// // Global advertising parameters
// struct bt_le_adv_param adv_param =
//     BT_LE_ADV_PARAM_INIT(BT_LE_ADV_OPT_NONE | BT_LE_ADV_OPT_EXT_ADV |
//                              BT_LE_ADV_OPT_USE_IDENTITY | BT_LE_ADV_OPT_NO_2M,
//                          0x30, 0x30, NULL);

// // The advertisement structure referencing the global buffer.
// static const struct bt_data ad[] = {
//     BT_DATA(BT_DATA_MANUFACTURER_DATA, mfg_data, 10),
//     BT_DATA(BT_DATA_NAME_COMPLETE, DEVICE_NAME, DEVICE_NAME_LEN),
// };

// /* --- Commands --- */
// #define CMD_SET_INTERVAL 0x01
// #define CMD_SET_PATTERN 0x02

// /* --- LED Patterns --- */
// typedef enum
// {
//         PATTERN_ALL_BLINK = 0, // All LEDs blink together
//         PATTERN_CHASE,         // LEDs chase one by one
//         PATTERN_ALTERNATE,     // LED0+LED2 ON, then LED1+LED3 ON
//         PATTERN_BOUNCE         // Bounce left → right → left
// } led_pattern_t;

// static led_pattern_t current_pattern = PATTERN_ALL_BLINK;

// /* --- BLE setup helpers --- */
// void set_random_static_address(void)
// {
//         bt_addr_le_t addr;
//         int err = bt_addr_le_from_str(SENSOR_ADDRESS, "random", &addr);
//         if (err)
//         {
//                 printk("Invalid BT address (err %d)\n", err);
//         }
//         int err1 = bt_id_create(&addr, NULL);
//         if (err1 < 0)
//         {
//                 printk("Creating new ID failed (err %d)\n", err1);
//         }
//         printk("Created new address\n");
// }

// void adv_param_init(void)
// {
//         int err = bt_le_ext_adv_create(&adv_param, NULL, &adv);
//         if (err)
//         {
//                 printk("Failed to create advertising set (err %d)\n", err);
//                 return;
//         }
//         printk("Created extended advertising set \n");
// }

// struct bt_le_scan_param scan_param = {
//     .type = BT_LE_SCAN_TYPE_PASSIVE,
//     .options = 0,
//     .interval = BT_GAP_SCAN_FAST_INTERVAL,
//     .window = BT_GAP_SCAN_FAST_WINDOW,
// };

// /* --- Helper: set all LEDs --- */
// static void set_all_leds(int state)
// {
//         for (int i = 0; i < NUM_LEDS; i++)
//         {
//                 gpio_pin_set_dt(&leds[i], state);
//         }
// }

// /* --- LED Blink Task --- */
// void led_blink_thread(void)
// {
//         int pos = 0; // used in chase/bounce
//         int dir = 1; // bounce direction

//         while (1)
//         {
//                 switch (current_pattern)
//                 {
//                 case PATTERN_ALL_BLINK:
//                         set_all_leds(1);
//                         k_msleep(blink_interval_ms);
//                         set_all_leds(0);
//                         k_msleep(blink_interval_ms);
//                         break;

//                 case PATTERN_CHASE:
//                         set_all_leds(0);
//                         gpio_pin_set_dt(&leds[pos], 1);
//                         k_msleep(blink_interval_ms);
//                         pos = (pos + 1) % NUM_LEDS;
//                         break;

//                 case PATTERN_ALTERNATE:
//                         // Group 1: LED0 + LED2
//                         gpio_pin_set_dt(&leds[0], 1);
//                         gpio_pin_set_dt(&leds[2], 1);
//                         gpio_pin_set_dt(&leds[1], 0);
//                         gpio_pin_set_dt(&leds[3], 0);
//                         k_msleep(blink_interval_ms);

//                         // Group 2: LED1 + LED3
//                         gpio_pin_set_dt(&leds[0], 0);
//                         gpio_pin_set_dt(&leds[2], 0);
//                         gpio_pin_set_dt(&leds[1], 1);
//                         gpio_pin_set_dt(&leds[3], 1);
//                         k_msleep(blink_interval_ms);
//                         break;

//                 case PATTERN_BOUNCE:
//                         set_all_leds(0);
//                         gpio_pin_set_dt(&leds[pos], 1);
//                         k_msleep(blink_interval_ms);

//                         pos += dir;
//                         if (pos == NUM_LEDS - 1 || pos == 0)
//                         {
//                                 dir = -dir; // reverse direction
//                         }
//                         break;
//                 }
//         }
// }
// K_THREAD_DEFINE(led_blink_tid, 1024, led_blink_thread, NULL, NULL, NULL,
//                 7, 0, 0);

// /* --- Parse Advertisement Callback --- */
// static bool ad_parse_cb(struct bt_data *data, void *user_data)
// {
//         if (data->type == BT_DATA_MANUFACTURER_DATA)
//         {
//                 const uint8_t *payload = data->data;
//                 size_t len = data->data_len;

//                 if (len >= 4 && payload[0] == 0x59 && payload[1] == 0x00)
//                 {
//                         uint8_t cmd = payload[2];
//                         uint8_t value = payload[3];

//                         if (cmd == CMD_SET_INTERVAL)
//                         {
//                                 int new_interval = value * 100; // scale: 100ms steps
//                                 if (new_interval < 100)
//                                         new_interval = 100;

//                                 LOG_INF("Blink interval change: %d ms -> %d ms",
//                                         blink_interval_ms, new_interval);
//                                 blink_interval_ms = new_interval;
//                         }
//                         else if (cmd == CMD_SET_PATTERN)
//                         {
//                                 if (value <= PATTERN_BOUNCE)
//                                 {
//                                         LOG_INF("LED pattern change -> %d", value);
//                                         current_pattern = (led_pattern_t)value;
//                                 }
//                                 else
//                                 {
//                                         LOG_WRN("Unknown pattern %d", value);
//                                 }
//                         }
//                 }
//         }
//         return true; // continue parsing
// }

// /* --- Device Found Callback --- */
// static void device_found(const bt_addr_le_t *addr, int8_t rssi,
//                          uint8_t type, struct net_buf_simple *ad)
// {
//         if (type == BT_GAP_ADV_TYPE_ADV_IND ||
//             type == BT_GAP_ADV_TYPE_ADV_NONCONN_IND)
//         {
//                 bt_data_parse(ad, ad_parse_cb, NULL);
//         }
// }

// /* --- Start Advertising --- */
// static void start_advertising(void)
// {
//         // Update mfg_data with current settings
//         mfg_data[3] = blink_interval_ms / 100;
//         mfg_data[4] = (uint8_t)current_pattern;

//         int err = bt_le_ext_adv_set_data(adv, ad, ARRAY_SIZE(ad), NULL, 0);
//         if (err)
//         {
//                 LOG_ERR("Failed to set advertising data (err %d)", err);
//         }

//         err = bt_le_ext_adv_start(adv, NULL);
//         if (err)
//         {
//                 LOG_ERR("Advertising failed to start (err %d)", err);
//         }
//         else
//         {
//                 LOG_INF("Started advertising [interval=%d ms, pattern=%d]",
//                         blink_interval_ms, current_pattern);
//         }
// }

// /* --- Stop Advertising --- */
// static void stop_advertising(void)
// {
//         int err = bt_le_ext_adv_stop(adv);
//         if (err)
//         {
//                 LOG_ERR("Advertising failed to stop (err %d)", err);
//         }
//         else
//         {
//                 LOG_INF("Stopped advertising.");
//         }
// }

// /* --- Start Observer --- */
// static void observer_start(void)
// {
//         int err = bt_le_scan_start(&scan_param, device_found);
//         if (err)
//         {
//                 LOG_ERR("Start scanning failed (err %d)", err);
//         }
//         else
//         {
//                 LOG_INF("Started scanning...");
//         }
// }

// /* --- Main BLE state thread --- */
// void ble_thread_handler(void)
// {
//         while (1)
//         {
//                 if (current_mode == MODE_ADVERTISING)
//                 {
//                         start_advertising();
//                         k_sleep(K_MSEC(500)); // Advertise briefly
//                         stop_advertising();
//                         current_mode = MODE_SCANNING;
//                 }
//                 else
//                 {
//                         observer_start();
//                         k_sleep(K_MSEC(500)); // Scan briefly
//                         bt_le_scan_stop();
//                         current_mode = MODE_ADVERTISING;
//                 }
//         }
// }
// K_THREAD_DEFINE(ble_handler_tid, 1024, ble_thread_handler, NULL, NULL, NULL,
//                 7, 0, 0);

// void main(void)
// {
//         int err;

//         /* Init LEDs */
//         for (int i = 0; i < NUM_LEDS; i++)
//         {
//                 if (!device_is_ready(leds[i].port))
//                 {
//                         LOG_ERR("LED %d not ready", i);
//                         return;
//                 }
//                 gpio_pin_configure_dt(&leds[i], GPIO_OUTPUT_INACTIVE);
//         }

//         /* Enable Bluetooth */
//         err = bt_enable(NULL);
//         if (err)
//         {
//                 LOG_ERR("Bluetooth init failed (err %d)", err);
//                 return;
//         }

//         /* Create the advertising set once */
//         set_random_static_address();
//         adv_param_init();

//         mfg_data[0] = 0x59;             // Company ID LSB
//         mfg_data[1] = 0x00;             // Company ID MSB
//         mfg_data[2] = CMD_SET_INTERVAL; // Default command
//         mfg_data[3] = blink_interval_ms / 100;
//         mfg_data[4] = (uint8_t)current_pattern;

//         k_msleep(100); // Wait for Bluetooth stack to stabilize
//         LOG_INF("Bluetooth initialized, starting BLE handler...");
// }
