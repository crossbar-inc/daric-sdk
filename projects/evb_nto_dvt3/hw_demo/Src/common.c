#include "common.h"
#include "daric_gui.h"
#include "tx_log.h"
#include "gx_api.h"

#undef LOG_TAG
#define LOG_TAG "COMMON_TAG"

#define REFRESH_SCREEN_DELAY 1//1 ms
#define REFRESH_SCREEN_TIMER_FLAG 1

TX_TIMER refresh_screen_timer;
extern GX_WINDOW_ROOT *root;
extern void daricGuiSwitchMode(LCD_Mode mode);
void switchLCDRefreshMode(LCD_Mode mode)
{
#ifdef CONFIG_LCD_ED028TC1_SPI
    daricGuiSwitchMode(mode);
#endif
}

void refresh_screen_timer_callback(ULONG arg);

void activate_refresh_screen_timer()
{
    deactivate_refresh_screen_timer();
    tx_timer_activate(&refresh_screen_timer);
}

void deactivate_refresh_screen_timer()
{
    tx_timer_deactivate(&refresh_screen_timer);
    tx_timer_change(&refresh_screen_timer,
                    REFRESH_SCREEN_DELAY,
                    (ULONG)0);
}

void init_refresh_screen_timer()
{
    UINT status = tx_timer_create(&refresh_screen_timer, "refresh_screen_timer",
                                  refresh_screen_timer_callback, REFRESH_SCREEN_TIMER_FLAG,
                                  REFRESH_SCREEN_DELAY,
                                  (ULONG)0,
                                  TX_NO_ACTIVATE);
    LOGV("init_refresh_screen_timer, stauts: %d", status);
}

void refresh_screen_timer_callback(ULONG arg)
{
    LOGV("refresh_screen_timer_callback");
    switch(arg)
    {
        case REFRESH_SCREEN_TIMER_FLAG:
            switchLCDRefreshMode(GC16);
            GX_EVENT resource_event;
            resource_event.gx_event_target = current_screen;
            resource_event.gx_event_type = GX_EVENT_CLIENT_UPDATED;
            resource_event.gx_event_sender = current_screen->gx_widget_id;
            gx_system_event_send(&resource_event);
            break;
    }
}

GX_WIDGET* get_current_window()
{
    GX_WIDGET *top_window = GX_NULL;

    UINT status = gx_widget_top_visible_child_find((GX_WIDGET *)root, &top_window);
    if (status == GX_SUCCESS)
    {
        return top_window;
    }
    return GX_NULL;
}

bool is_leap_year(int32_t year) {
    return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
}

bool is_date_valid(int32_t year, MONTH_E month, int32_t day)
{
    bool valid = false;
    if (year < 2000 || year > 9999 || month > 12 || day > 32
        || month <= 0 || day <= 0)
    {
        goto __exit;
    }
    switch (month)
    {
        case JANUARY:
        case MARCH:
        case MAY:
        case JULY:
        case AUGUST:
        case OCTOBER:
        case DECEMBER:
            valid = (day <= 31)? true : false;
            break;
        case FEBRUARY:
            if (is_leap_year(year))
            {
                valid = (day <= 29)? true : false;
            }
            else
            {
                valid = (day <= 28)? true : false;
            }
            break;
        case APRIL:
        case JUNE:
        case SEPTEMBER:
        case NOVEMBER:
            valid = (day <= 30)? true : false;
            break;
    }
__exit:
    return valid;
}

bool is_time_valid(int32_t hour, int32_t minute, int32_t second)
{
    if (hour < 0 || hour >= 24 || minute < 0 || minute >= 60
        || second < 0 || second >= 60)
    {
        return false;
    }
    return true;
}