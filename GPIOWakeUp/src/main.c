#include <zephyr/zephyr.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>

#define WAKEUP_PORT DT_NODELABEL(gpio0)
#define WAKEUP_PIN 13
#define WAKEUP_FLAGS (GPIO_INPUT | GPIO_PULL_UP)

const struct device *gpio_dev;

void main(void) {
    gpio_dev = DEVICE_DT_GET(WAKEUP_PORT);
    if (!device_is_ready(gpio_dev)) {
        printk("GPIO device not ready\n");
        return;
    }

    gpio_pin_configure(gpio_dev, WAKEUP_PIN, WAKEUP_FLAGS);

    while (1) {
        int value = gpio_pin_get(gpio_dev, WAKEUP_PIN);
        if (value == 0) {
            printk("Button pressed!\n");
        } else {
            printk("Button not pressed.\n");
        }
        k_sleep(K_MSEC(1000));
    }
}
