#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#define SLEEP_TIME_MS   1000

#define LED0_NODE DT_NODELABEL(led0)
#define BUTTON_NODE DT_NODELABEL(button0)

static const struct gpio_dt_spec led_spec = GPIO_DT_SPEC_GET(LED0_NODE, gpios);
static const struct gpio_dt_spec button_spec = GPIO_DT_SPEC_GET(BUTTON_NODE, gpios);
static struct gpio_callback button_cb;


int ret;
static void timer_cb(struct k_timer *ct){
	gpio_pin_toggle_dt(&led_spec);
	K_MSEC(100);
	
	// printk("%d",ret);

}
struct k_timer mytimer;

void button_pressed_callback(const struct device *gpioB, struct gpio_callback *cb, gpio_port_pins_t pins){
	// gpio_pin_toggle_dt(&led_spec);
	// k_timer_stop(&mytimer);
	gpio_pin_set_dt(&led_spec, 1);
	// printk("Yo\n");
}


int main(void)
{
	gpio_pin_configure_dt(&led_spec, GPIO_OUTPUT);
	gpio_pin_configure_dt(&button_spec, GPIO_INPUT);
	gpio_init_callback(&button_cb,button_pressed_callback, BIT(button_spec.pin));
	gpio_add_callback(button_spec.port,&button_cb);
	gpio_pin_interrupt_configure_dt(&button_spec, GPIO_INT_EDGE_TO_ACTIVE);
	
	k_timer_init(&mytimer , timer_cb,NULL);
	k_timer_start(&mytimer, K_MSEC(100),K_MSEC(5000));
	while(1){
		k_msleep(1000);
	}
	return 0;
}