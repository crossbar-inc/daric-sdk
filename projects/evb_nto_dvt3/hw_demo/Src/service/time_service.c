/**
 ******************************************************************************
 * @file    time_service.c
 * @author  AP Team
 * @brief   This file mainly defines the time service.
 *          include get and set current time.
******************************************************************************
* @attention
*
* © Copyright CrossBar, Inc. 2024.
*
* All rights reserved.
*
* This software is the proprietary property of CrossBar, Inc. and is protected
* by copyright laws. Any unauthorized reproduction, distribution, or
* modification is strictly prohibited.
*
******************************************************************************
*/
#include <stdlib.h>
#include "time_service.h"
#undef LOG_TAG
#define LOG_TAG "TIME_SERV"
/*! \brief Initializes the Real-Time Clock (RTC) module.
 *
 * @param none
 * @retval int32_t.
 */
int32_t init_time_service(void) {
    return BSP_RTC_Init();
}

/*! \brief Sets the date in the Real-Time Clock (RTC) module..
 *
 * @param year The year value in binary format:YY.
 * @param month The month value in binary format:MM.
 * @param day The day value in binary format:DD.
 * @param weekday The weekday value in enum RTC_WEEKDAY_T,eg.:SUNDAY.
 * @retval int32_t.
 */
int32_t set_date(uint8_t year, uint8_t month, uint8_t day, RTC_WEEKDAY_T weekday) {
    return BSP_RTC_SetDate(year, month, day, weekday);
}

/*! \brief Retrieves the current date from the Real-Time Clock (RTC) module.
 *
 * @param date A pointer to a character array where the current date will be stored.
 *        The date will be formatted as "YYYY-MM-DD (Weekday: W)".
 * @retval int32_t.
 */
int32_t get_current_date(char *date) {
    return BSP_RTC_GetDate(date);
}

/*! \brief Sets the time in the Real-Time Clock (RTC) module.
 *
 * @param hours The hours value in binary format:HH.
 * @param minutes The minutes value in binary format:MM.
 * @param seconds The seconds value in binary format:SS.
 * @retval int32_t.
 */
int32_t set_time(uint8_t hours, uint8_t minutes, uint8_t seconds) {
    return BSP_RTC_SetTime(hours, minutes, seconds);
}

/*! \brief Retrieves the current time from the Real-Time Clock (RTC) module.
 *
 * @param current_time A pointer to a character array where the current time will be stored.
 *        The time will be formatted as "HH:MM:SS".
 * @retval int32_t.
 */
int32_t get_current_time(char *current_time) {
    return BSP_RTC_GetTime(current_time);
}

/*! \brief convert time string to total seconds.
 *
 * @param none.
 * @retval int32_t total seconds.
 */
uint32_t get_current_time_in_seconds() {
    uint32_t hours = 0, minutes = 0, seconds = 0;
    char buffer[3]; 
    char timeStr[10] = { '\0' };

    //get time string format: HH:MM:SS
    get_current_time(timeStr);

    // handle hours
    strncpy(buffer, timeStr, 2);
    buffer[2] = '\0';
    hours = atoi(buffer);

    // handle minutes
    strncpy(buffer, timeStr + 3, 2);
    buffer[2] = '\0';
    minutes = atoi(buffer);

    // handle seconds
    strncpy(buffer, timeStr + 6, 2);
    buffer[2] = '\0';
    seconds = atoi(buffer);

    // total seconds
    return hours * 3600 + minutes * 60 + seconds;
}

/*! \brief Deinitializes the Real-Time Clock (RTC) module.
 *
 * @param none.
 * @retval int32_t.
 */
int32_t deinit_time_service(void) {
    return BSP_RTC_DeInit();
}