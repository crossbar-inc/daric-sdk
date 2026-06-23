/**
 *******************************************************************************
 * @file    sys_config.h
 * @author  Daric Team
 * @brief   Header file for sys_config.h module.
 *******************************************************************************
 * @attention
 *
 * Copyright 2024-2026 CrossBar, Inc.
 * This file has been modified by CrossBar, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * You may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an AS IS BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 *******************************************************************************
 */
#ifndef _SYS_CONFIG_H_
#define _SYS_CONFIG_H_
#include "daric_hal_wdg.h"

#ifdef CONFIG_SOC_DARIC_NTO_A
#define ITCM_PUT_BUFF_SECTION __attribute__((section("itcmdata"), aligned(4)))
#define DTCM_PUT_BUFF_SECTION __attribute__((section("dtcmdata"), aligned(4)))
#else
#define ITCM_PUT_BUFF_SECTION
#define DTCM_PUT_BUFF_SECTION
#endif

#define WATCHDOG_TIMEOUT_MS (30000 * WDG_TICKS_PER_MS)
#endif // _SYS_CONFIG_H_
