/**
 *****************************************************************************
 * @file    time_service.h
 * @author  AP Team
 * @brief   Header file of time_service.
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
#include "rtc_ins5t8900.h"

int32_t init_time_service(void);
int32_t set_date(uint8_t year, uint8_t month, uint8_t day, RTC_WEEKDAY_T weekday);
int32_t get_current_date(char *date);
int32_t set_time(uint8_t hours, uint8_t minutes, uint8_t seconds);
int32_t get_current_time(char *time);
uint32_t get_current_time_in_seconds();
int32_t deinit_time_service(void);
