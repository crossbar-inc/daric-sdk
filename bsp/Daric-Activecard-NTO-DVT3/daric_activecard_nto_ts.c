/**
 ******************************************************************************
 * @file    daric_activecard_nto_ts.c
 * @author  PERIPHERIAL BSP Team
 * @brief   Generic Touch interface for upper layer calls
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

#include "hyn_core.h"
#include "daric_activecard_nto_ts.h"


#define SUPPORT_MAX 10

const TS_Drv_t *Ts_Drv = NULL;
TX_EVENT_FLAGS_GROUP gTouchEventGroup;
extern struct hyn_ts_data *hyn_92xxdata;

/**
  * @brief  List of supported touch screen drivers.
  *         This array holds the names and corresponding driver functions for each supported touch screen.
  *         Each entry in the array represents a specific touch screen driver, identified by its name and
  *         associated function pointer structure.
  * @note   The array size is defined by SUPPORT_MAX, and any unused entries will have a NULL name and driver.
  * @param  name  The name of the touch screen driver as a string.
  * @param  driver  The function pointer structure that holds the driver operations for the touch screen.
  */

static Ts_Drv_list Touch_Driver_list[SUPPORT_MAX] = {
    {"cst9220",&cst92xx_fuc_using},

};

/**
  * @brief  Gets the touch panel driver operations based on the instance index.
  * @param  Instance  The index of the touch driver instance.
  *                   Should be between 0 and SUPPORT_MAX - 1.
  * @retval Pointer to TS_Drv_t structure if successful, NULL otherwise.
  */

const TS_Drv_t *get_tp_driver_ops(uint32_t Instance) {
    if (Instance >= SUPPORT_MAX) {
        printf("Instance index out of bounds.\n");
        return NULL;
    }

    if (Touch_Driver_list[Instance].name != NULL) {
        printf("Found driver: %s at index %ld\n", Touch_Driver_list[Instance].name, Instance);
        return Touch_Driver_list[Instance].driver; // 返回指向 driver 的指针
    } else {
        printf("No driver found at index %ld\n", Instance);
        return NULL;
    }
}

/**
  * @brief  Probes the touch driver and checks if it is available.
  * @param  Instance  The index of the touch driver instance.
  * @retval 0 if the probe is successful, -1 otherwise.
  */

static int Touch_Probe(uint32_t Instance)
{
    int ret = BSP_ERROR_NO_INIT;
    Ts_Drv = get_tp_driver_ops(Instance);
    if (Ts_Drv && Ts_Drv->Init != NULL) {
        ret =BSP_ERROR_NONE;
        printf("Touch_Probe success,this driver is %s\n",Touch_Driver_list[Instance].name);
    } else {
        printf("THE DRIVER DOES NOT MATCH THE Touch_Driver_list\n");
    }
    return ret;
}


/**
  * @brief  Initializes the touch screen.
  * @param  Instance  TS instance index.
  * @retval BSP status:
  *         - Returns 0 on success.
  *         - Returns a negative value on failure.
  */

int32_t BSP_TS_Init(uint32_t Instance)
{
    int ret = BSP_ERROR_NO_INIT;
    static struct hyn_ts_data ts_data;

    tx_event_flags_create(&gTouchEventGroup, "TouchPanel Events");

    memset((void*)&ts_data,0,sizeof(ts_data));
    hyn_92xxdata = &ts_data;
    ret = Touch_Probe(Instance);
    if(ret == 0)
    {
        ret = Ts_Drv->Init(Instance,hyn_92xxdata);
        if(ret == 0)
        printf("BSP_TS_Init Success\n");
    }
    return ret;
}



/**
 * @brief  De-Initializes the touch screen functionalities
 * @param  Instance TS instance.
 * @retval BSP status
 */
int32_t BSP_TS_DeInit(uint32_t Instance)
{
    Ts_Drv->DeInit(Instance);
    return BSP_ERROR_NONE;
}

/**
 * @brief  Configures and enables the touch screen interrupts.
 * @param  Instance TS instance.
 * @retval BSP status
 */
int32_t BSP_TS_EnableIT(uint32_t Instance)
{
    Ts_Drv->EnableIT(Instance);
    return BSP_ERROR_NONE;
}

/**
 * @brief  Disables the touch screen interrupts.
 * @param  Instance TS instance.
 * @retval BSP status
 */
int32_t BSP_TS_DisableIT(uint32_t Instance)
{

    Ts_Drv->DisableIT(Instance);
    return BSP_ERROR_NONE;
}

/**
 * @brief  Puts the touch screen into sleep mode.
 * @param  Instance TS instance.
 * @retval BSP status
 */
int32_t BSP_TS_EnterSleep(uint32_t Instance)
{
    Ts_Drv->EnterSleep(Instance);
    return BSP_ERROR_NONE;
}

/**
 * @brief  Wakes the touch screen from sleep mode.
 * @param  Instance TS instance.
 * @retval BSP status
 */
int32_t BSP_TS_ExitSleep(uint32_t Instance)
{
    Ts_Drv->ExitSleep(Instance);
    return BSP_ERROR_NONE;
}

/**
  * @brief  Retrieves the multi-touch state of the touch screen.
  * @param  Instance  TS instance. Could be only 0.
  * @param  TS_State  Pointer to a structure that holds the current state of the touch screen.
  *                   The function will populate this structure with the multi-touch data.
  * @retval BSP status:
  *         - Returns 0 on success.
  *         - Returns a negative value on failure.
  */

int32_t BSP_TS_Get_MultiTouchState(uint32_t Instance, pos_info* TS_State)
{
    int ret = BSP_ERROR_NO_INIT;
    ret = Ts_Drv->GetMultiTouchState(Instance,TS_State);
    if(ret ==0)
    {
        printf("BSP_TS_Get_MultiTouchState success\n");
    }
    else
    {
        printf("BSP_TS_Get_MultiTouchState fail");
    }
    return ret;
}
