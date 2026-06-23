/**
 ******************************************************************************
 * @file    pinmap_config.h
 * @author  PINMAP Team
 * @brief   Header file of PINMAP module.
 ******************************************************************************
 * @attention
 *
 * © Copyright CrossBar, Inc. 2024.

 * All rights reserved.

 * This software is the proprietary property of CrossBar, Inc. and is protected
 * by copyright laws. Any unauthorized reproduction, distribution, or
 * modification is strictly prohibited.
 *
 ******************************************************************************
 */

#ifndef __PINMAP_CONFIG_H
#define __PINMAP_CONFIG_H

#include "daric_hal_pinmap.h"

/**
 * @brief Array of pin mappings for configuring GPIO pins on a microcontroller.
 *
 * This array defines the initialization parameters for various GPIO pins,
 * specifying the port, pin number, and alternate function for each pin. The
 * pins are configured for different functionalities such as I2C, UART, Timer
 * PWM, and ADC.
 *
 * Each entry in the array is a structure of type `PINMAP_InitTypeDef` that
 * includes:
 * - `PORT_x`: The port to which the pin belongs (e.g., PORT_A).
 * - `PIN_x`: The pin number on the specified port (e.g., PIN_0).
 * - `AFx_PORTy_PINz`: The alternate function assigned to the pin (e.g.,
 * AF2_PA0_I2C1_SCL).
 *
 * Example entries:
 * - Port A Pin 0 is configured for I2C1 SCL.
 * - Port A Pin 1 is configured for I2C1 SDA.
 *
 * This setup allows for flexible GPIO configuration tailored to specific
 * peripheral needs.
 */
PINMAP_InitTypeDef g_pinMap[] = {
    {PORT_A, PIN_0, AF2_PA0_I2C1_SCL}, // Configure PA0
    {PORT_A, PIN_1, AF2_PA1_I2C1_SDA}, // Configure PA1
    {PORT_A, PIN_3, AF1_PA3_UART0_RX}, // Configure PA3
    {PORT_A, PIN_4, AF1_PA4_UART0_TX}, // Configure PA4

    {PORT_B, PIN_11, AF1_PB11_I2C0_SCL}, // Configure PB11
    {PORT_B, PIN_12, AF1_PB12_I2C0_SDA}, // Configure PB12
    {PORT_B, PIN_13, AF1_PB13_UART2_RX}, // Configure PB13
    {PORT_B, PIN_14, AF1_PB14_UART2_TX}, // Configure PB14

    {PORT_C, PIN_5, AF2_PC5_I2C3_SCL},     // Configure PC5
    {PORT_C, PIN_6, AF2_PC6_I2C3_SDA},     // Configure PC6
    {PORT_C, PIN_7, AF1_PC7_SPIM1_SD0},    // Configure PC7
    {PORT_C, PIN_8, AF1_PC8_SPIM1_SD1},    // Configure PC8
    {PORT_C, PIN_11, AF1_PC11_SPIM1_CLK},  // Configure PC11
    {PORT_C, PIN_12, AF1_PC12_SPIM1_CSN0}, // Configure PC12

    {PORT_D, PIN_0, AF1_PD0_SPIM0_SD0},    // Configure PD0
    {PORT_D, PIN_1, AF1_PD1_SPIM0_SD1},    // Configure PD1
    {PORT_D, PIN_2, AF1_PD2_SPIM0_SD2},    // Configure PD2
    {PORT_D, PIN_3, AF1_PD3_SPIM0_SD3},    // Configure PD3
    {PORT_D, PIN_4, AF1_PD4_SPIM0_CLK},    // Configure PD4
    {PORT_D, PIN_5, AF1_PD5_SPIM0_CSN0},   // Configure PD5
    {PORT_D, PIN_7, AF2_PD7_SPIM1_CLK},    // Configure PD7
    {PORT_D, PIN_8, AF2_PD8_SPIM1_SD0},    // Configure PD8
    {PORT_D, PIN_9, AF2_PD9_SPIM1_SD1},    // Configure PD9
    {PORT_D, PIN_10, AF2_PD10_SPIM1_CSN0}, // Configure PD10
    {PORT_D, PIN_13, AF1_PD13_UART1_RX},   // Configure PD13
    {PORT_D, PIN_14, AF1_PD14_UART1_TX},   // Configure PD14
};

#endif
