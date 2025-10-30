#include <zephyr/pm/pm.h>         // For power management functions
#include <zephyr/pm/pm_state.h>   // For power state definitions
#include <zephyr/kernel.h>

void main(void)
{
    printk("Forcing deep sleep...\n");

    // Force the device into a deep sleep (soft-off) state
    pm_state_force(0, &(struct pm_state_info){
        .state = PM_STATE_SOFT_OFF,
        .substate_id = 0
    });

    printk("Device is now in deep sleep mode.\n");
}
