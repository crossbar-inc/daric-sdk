/*
 * Copyright (c) 2009-2021 Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the License); you may
 * not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an AS IS BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "daric.h"
#include <stdio.h>

/*---------------------------------------------------------------------------
  External References
 *---------------------------------------------------------------------------*/
extern uint32_t __INITIAL_SP;
extern uint32_t __STACK_LIMIT;

extern __NO_RETURN void __PROGRAM_START(void);

/*---------------------------------------------------------------------------
  Internal References
 *---------------------------------------------------------------------------*/
#if defined(__CC_ARM)
__NO_RETURN void Reset_Handler(void);
#elif defined(__GNUC__)
__attribute__((naked)) void Reset_Handler(void);
#endif
void Default_Handler(void);

/*---------------------------------------------------------------------------
  Exception / Interrupt Handler
 *---------------------------------------------------------------------------*/
/* Exceptions */
void NMI_Handler(void) __attribute__((weak, alias("HardFault_Handler")));
void HardFault_Handler(void) __attribute__((weak));
void MemManage_Handler(void) __attribute__((weak, alias("HardFault_Handler")));
void BusFault_Handler(void) __attribute__((weak, alias("HardFault_Handler")));
void UsageFault_Handler(void) __attribute__((weak, alias("HardFault_Handler")));
void SecureFault_Handler(void) __attribute__((weak, alias("Default_Handler")));
void SVC_Handler(void) __attribute__((weak, alias("Default_Handler")));
void DebugMon_Handler(void) __attribute__((weak, alias("Default_Handler")));
void PendSV_Handler(void) __attribute__((weak, alias("Default_Handler")));
void SysTick_Handler(void) __attribute__((weak, alias("Default_Handler")));

/* External Interrupt Handler */
void EXT_IRQ_Handler(void) __attribute__((weak, alias("Default_Handler")));

/*----------------------------------------------------------------------------
  Exception / Interrupt Vector table
 *----------------------------------------------------------------------------*/

#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
#endif

extern const VECTOR_TABLE_Type __VECTOR_TABLE[16 + TOTAL_IRQ_NUMS];
const VECTOR_TABLE_Type
    __VECTOR_TABLE[16 + TOTAL_IRQ_NUMS] __VECTOR_TABLE_ATTRIBUTE = {
        (VECTOR_TABLE_Type)(&__INITIAL_SP), /*     Initial Stack Pointer */
        Reset_Handler,                      /*     Reset Handler */
        NMI_Handler,                        /* -14 NMI Handler */
        HardFault_Handler,                  /* -13 Hard Fault Handler */
        MemManage_Handler,                  /* -12 MPU Fault Handler */
        BusFault_Handler,                   /* -11 Bus Fault Handler */
        UsageFault_Handler,                 /* -10 Usage Fault Handler */
        SecureFault_Handler,                /*  -9 Secure Fault Handler */
        0,                                  /*     Reserved */
        0,                                  /*     Reserved */
        0,                                  /*     Reserved */
        SVC_Handler,                        /*  -5 SVCall Handler */
        DebugMon_Handler,                   /*  -4 Debug Monitor Handler */
        0,                                  /*     Reserved */
        PendSV_Handler,                     /*  -2 PendSV Handler */
        SysTick_Handler,                    /*  -1 SysTick Handler */

        /* External Interrupts */
        [16 ...(16 + TOTAL_IRQ_NUMS - 1)] = EXT_IRQ_Handler,
};

#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

/*---------------------------------------------------------------------------
  Reset Handler called on controller reset
 *---------------------------------------------------------------------------*/
#if defined(__CC_ARM)
__NO_RETURN void Reset_Handler(void)
#elif defined(__GNUC__)
__attribute__((naked)) void Reset_Handler(void)
#endif
{
  /* Make sure ALL interrupts are disabled, (only) will be enabled at kernel
   * enter. */
  __disable_irq();

  __set_MSP((uint32_t)(&__INITIAL_SP));
  __set_PSP((uint32_t)(&__INITIAL_SP));

  // Set CONTROL register to use PSP and remain in privileged mode
  __set_CONTROL(__get_CONTROL() | CONTROL_SPSEL_Msk);

  /* CMSIS System Initialization */
  SystemInit();

  /* Enter PreMain (C library entry point) */
  __PROGRAM_START();
}

#if defined(__ARMCC_VERSION) && (__ARMCC_VERSION >= 6010050)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmissing-noreturn"
#endif

/*---------------------------------------------------------------------------
  Hard Fault Handler
 *---------------------------------------------------------------------------*/
// void HardFault_Handler(void) {
//   while (1) {
//     /* loop */
//   }
// }

/*---------------------------------------------------------------------------
  Default Handler for Exceptions / Interrupts
 *---------------------------------------------------------------------------*/
void Default_Handler(void) {
  while (1) {
    /* loop */
  }
}

// M7 exception stack struct
typedef struct {
    uint32_t r0;
    uint32_t r1;
    uint32_t r2;
    uint32_t r3;
    uint32_t r12;
    uint32_t lr;
    uint32_t pc;
    uint32_t psr;
} ExceptionStackFrame;


// CFSR bit def
// MemManageFault (MMFSR, CFSR[7:0])
// BusFault (BFSR, CFSR[15:8])
// UsageFault (UFSR, CFSR[31:16])

/**
 * @brief Print detailed HardFault exception information
 * @param stack_frame Pointer to the exception stack frame
 */
void print_hardfault_info(ExceptionStackFrame *stack_frame)
{
    uint32_t hfsr = SCB->HFSR;
    uint32_t cfsr = SCB->CFSR;
    uint32_t mmfar = SCB->MMFAR;
    uint32_t bfar = SCB->BFAR;
    
    printf("\r\n");
    printf("======================================\r\n");
    printf("Exception Log\n");
    printf("======================================\r\n");
    
    printf("Exception Stack Frame:\r\n");
    printf("  R0  = 0x%08lX\r\n", stack_frame->r0);
    printf("  R1  = 0x%08lX\r\n", stack_frame->r1);
    printf("  R2  = 0x%08lX\r\n", stack_frame->r2);
    printf("  R3  = 0x%08lX\r\n", stack_frame->r3);
    printf("  R12 = 0x%08lX\r\n", stack_frame->r12);
    printf("  LR  = 0x%08lX\r\n", stack_frame->lr);
    printf("  PC  = 0x%08lX\r\n", stack_frame->pc);
    printf("  PSR = 0x%08lX\r\n", stack_frame->psr);
    printf("\r\n");
    
    //HardFault status Register
    printf("HardFault Status Register (0x%08lX):\r\n", hfsr);
    if (hfsr & SCB_HFSR_FORCED_Msk) {
        printf("  [FORCED] Forced HardFault\r\n");
    }
    if (hfsr & SCB_HFSR_VECTTBL_Msk) {
        printf("  [VECTTBL] Vector table read error\r\n");
    }
    printf("\r\n");
    
    // SCB CFSR
    printf("Configurable Fault Status (0x%08lX):\r\n", cfsr);
    
    // MemManageFault
    if (cfsr & 0xFF) {
        printf("  *** MemManage Fault ***\r\n");
        if (cfsr & SCB_CFSR_IACCVIOL_Msk) {
            printf("  [IACCVIOL] Instruction access violation\r\n");
        }
        if (cfsr & SCB_CFSR_DACCVIOL_Msk) {
            printf("  [DACCVIOL] Data access violation\r\n");
        }
        if (cfsr & SCB_CFSR_MUNSTKERR_Msk) {
            printf("  [MUNSTKERR] MemManage fault on unstack\r\n");
        }
        if (cfsr & SCB_CFSR_MSTKERR_Msk) {
            printf("  [MSTKERR] MemManage fault on stack\r\n");
        }
        if (cfsr & SCB_CFSR_MLSPERR_Msk) {
            printf("  [MLSPERR] MemManage fault on flaotPoint\r\n");
        }
        if (cfsr & SCB_CFSR_MMARVALID_Msk) {
            printf("  [MMARVALID] Fault address: 0x%08lX\r\n", mmfar);
        }
    }
    
    // BusFault
    if (cfsr & 0xFF00) {
        printf("  *** BusFault ***\r\n");
        if (cfsr & SCB_CFSR_IBUSERR_Msk) {
            printf("  [IBUSERR] Instruction bus error\r\n");
        }
        if (cfsr & SCB_CFSR_PRECISERR_Msk) {
            printf("  [PRECISERR] Precise data bus error\r\n");
        }
        if (cfsr & SCB_CFSR_IMPRECISERR_Msk) {
            printf("  [IMPRECISERR] Imprecise data bus error\r\n");
        }
        if (cfsr & SCB_CFSR_UNSTKERR_Msk) {
            printf("  [UNSTKERR] BusFault on unstack\r\n");
        }
        if (cfsr & SCB_CFSR_STKERR_Msk) {
            printf("  [STKERR] BusFault on stack\r\n");
        }
        if (cfsr & SCB_CFSR_LSPERR_Msk) {
            printf("  [LSPERR] BusFault on flaotPoint\r\n");
        }
        if (cfsr & SCB_CFSR_BFARVALID_Msk) {
            printf("  [BFARVALID] Fault address: 0x%08lX\r\n", bfar);
        }
    }
    
    // UsageFault
    if (cfsr & 0xFFFF0000) {
        printf("  *** UsageFault ***\r\n");
        if (cfsr & SCB_CFSR_UNDEFINSTR_Msk) {
            printf("  [UNDEFINSTR] Undefined instruction\r\n");
        }
        if (cfsr & SCB_CFSR_INVSTATE_Msk) {
            printf("  [INVSTATE] Invalid state\r\n");
        }
        if (cfsr & SCB_CFSR_INVPC_Msk) {
            printf("  [INVPC] Invalid PC load\r\n");
        }
        if (cfsr & SCB_CFSR_NOCP_Msk) {
            printf("  [NOCP] Coprocessor error\r\n");
        }
        if (cfsr & SCB_CFSR_UNALIGNED_Msk) {
            printf("  [UNALIGNED] Unaligned access\r\n");
        }
        if (cfsr & SCB_CFSR_DIVBYZERO_Msk) {
            printf("  [DIVBYZERO] Division by zero\r\n");
        }
    }
    
    printf("\r\n");
    printf("======================================\r\n");
    printf("Backtrace:\n");
    printf("======================================\r\n");
    printf("  #00 pc 0x%08lX\n", stack_frame->pc);
    //uint32_t *sp;
    
    //__asm volatile ("mov %0, sp" : "=r"(sp));
    //sp = (uint32_t *)(stack_frame + 1);

    extern void printExcepStack(uint32_t *stack_frame);
    printExcepStack((uint32_t *)stack_frame);

}

/**
 * @brief HardFault exception c handler
 * @param none
 */
__attribute__((used, noinline)) void HardFault_Handler_C(ExceptionStackFrame *stack_frame)
{
    print_hardfault_info(stack_frame);
    
    while (1) {
        /* loop */
    }
}

/**
 * @brief HardFault exception asm handle
 * @param none
 */
__attribute__((naked)) void HardFault_Handler(void)
{
    __asm volatile (
        "    .global HardFault_Handler_C \n"
        "    tst lr, #4 \n"
        "    ite eq \n" // equal 0 means msp
        "    mrseq r0, msp \n"
        "    mrsne r0, psp \n"
        "    b HardFault_Handler_C \n"
        ::: "r0", "memory"
    );
}
#if defined(__ARMCC_VERSION) && (__ARMCC_VERSION >= 6010050)
#pragma clang diagnostic pop
#endif
