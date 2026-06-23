/**
 * Minimal stubs for BSP power-management hooks used by daric_pm.c in hw_demo.
 * Full implementations live in Activecard_NTO (sleep_mode.c, battery_service.c).
 */

#include <stdbool.h>

void sleep_mode_registerCallbackListener(void (*listener)(bool))
{
    (void)listener;
}

bool sleep_mode_is_screen_off(void)
{
    return false;
}

void bsp_pm_send_short_powerkey_event(void)
{
}

bool is_fast_boot()
{
    return false;
}