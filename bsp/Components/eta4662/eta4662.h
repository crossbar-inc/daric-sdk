/**
 ******************************************************************************
 * @file    eta4662.h
 * @author  PERIPHERIAL BSP Team
 * @brief   eta4662 driver.
 *          This file provides some functions for eta4662.
 *          charging functions
 *
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

#ifndef _ETA4662_H_
#define _ETA4662_H_

#include <stdbool.h>
#include <stdint.h>

#include "bsp_common.h"

void ETA4662_I2C_init(void);

/**
 * @brief  Initialize the registers
 * @param  reg_val_kv, the register value map
 * @param  len, the map size
 */
void ETA4662_initRegister(const REG8_MAP *reg_val_kv, uint8_t len);

/**
 * @brief  Set the pre-charge current
 * @param  current, the setted curren in mA;
 */
void ETA4662_setPreChargeCurrent(uint16_t current);

/**
 * @brief  Set the fast charge current
 * @param  current, the setted curren in mA;
 */
void ETA4662_setConstCurrent(uint16_t current);

/**
 * @brief  Set the charge const voltage
 * @param  current, the setted voltage in mV;
 */
void ETA4662_setConstVoltage(uint16_t voltage);

/**
 * @brief  Set the termination charge current
 * @param  current, the setted current in mA;
 */
void ETA4662_setTermCurrent(uint16_t current);

/**
 * @brief  Enable the charge function
 */
void ETA4662_enableCharge();

/**
 * @brief  Check if charger is enabled
 */
bool ETA4662_isChargerEnable();

/**
 * @brief  Enable the CE_HIZ
 */
void ETA4662_enable_hiz();

/**
 * @brief  Disable the CE_HIZ
 */
void ETA4662_disable_hiz();

/**
 * @brief  Disable the charge function
 */
void ETA4662_disableCharge();

/**
 * @brief  The interrup enum type.
 */
typedef enum {
    ETA4662_IRQ_BATT_OVT,
    ETA4662_IRQ_NTC,
    ETA4662_IRQ_CHG_STAT,
    ETA4662_IRQ_EOC,
    ETA4662_IRQ_PG,
    ETA4662_IRQ_ALL,
} ETA4662_IRQ_E;

/**
 * @brief  Enable or disable the interrukp
 * @param  flag, the interrkupt will controled;
 * @param  en, enable if true, disable else.
 */
void ETA4662_enableInterrup(ETA4662_IRQ_E flag, bool en);

#define ETA4662_FAULT_WTD     BIT(8+7)   // Reg08, Watchdog timer falut
#define ETA4662_FAULT_PPM     BIT(8+2)   // Reg08, Power management status
#define ETA4662_FAULT_PG      BIT(8+1)   // Reg08, Power good status
#define ETA4662_FAULT_THERM   BIT(8+0)   // Reg08, Thermal regulation status 
#define ETA4662_FAULT_VIN     BIT(5)     // Reg09, Input power fault, OVP or bad SOURCE
#define ETA4662_FAULT_THERMSD BIT(4)     // Reg09, Thermal shut down
#define ETA4662_FAULT_BATTOVP BIT(3)     // Reg09, Batter over voltage protection
#define ETA4662_FAULT_CHGTO   BIT(2)     // Reg09, Safety timer expiration
#define ETA4662_FAULT_NTCHOT  BIT(1)     // Reg09, NTC hot condition
#define ETA4662_FAULT_NTCCOLD BIT(0)     // Reg09, NTC cold condition

/**
 * @brief  Get the fault flags
 * @param  flags [out], the getted flags. The result is multiple ETA4662_FAULT_*
 * flags OR.
 * @return BSP_ERROR_NONE if succeed, false else.
 */
int ETA4662_getFaultStatus(uint16_t *flags);

typedef enum {
    ETA4662_CHG_ST_NONE = 0,
    ETA4662_CHG_ST_PRE  = 1,
    ETA4662_CHG_ST_CHG  = 2,
    ETA4662_CHG_ST_DONE = 3,
} ETA4662_CHG_ST_E;

/**
 * @brief  Get the charge status
 * @param  status [out], the getted status
 # @return BSP_ERROR_NONE if succeed, false else.
*/
int ETA4662_getChargeSt(ETA4662_CHG_ST_E *status);

/**
 * @brief Dump all the register.
 */
void ETA4662_dumpRegister(void);

#endif //_ETA4662_H_