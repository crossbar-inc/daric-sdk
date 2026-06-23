/**
 ******************************************************************************
 * @file    daric_fingerprint.h
 * @author  PERIPHERIAL BSP Team
 * @brief   This file contains the common defines and functions prototypes for
 *          the fingerprint operation.
 ******************************************************************************
 * @attention
 *
 * © Copyright CrossBar, Inc. 2024.
 * All rights reserved.
 *
 * All rights reserved.
 *
 * This software is the proprietary property of CrossBar, Inc. and is protected
 * by copyright laws. Any unauthorized reproduction, distribution, or
 * modification is strictly prohibited.
 *
 ******************************************************************************
 */

/* Initialization APIs */
#include <stdbool.h>
#include <stdint.h>

#define FP_ERR_ENROLL_FULL      1 // Can't enroll more fingerprint
#define FP_ERR_UNKNOW_CHIP      2 // Unknow chip id

#define FP_DUPLICATE_CHECK_NONE 0 // do not check finger duplication
#define FP_DUPLICATE_CHECK_ALL  1 // check finger duplication in all fingers
#define FP_DUPLICATE_CHECK_USER 2 // check finger duplication in user's fingers

#define FP_DUPLICATE_CHECK_T    FP_DUPLICATE_CHECK_USER

typedef enum {
    FP_EVENT_ENROLL_START      = 0,  // Start enroll finger
    FP_EVENT_ENROLL_CONTINUE   = 1,  // Continue enroll finger
    FP_EVENT_ENROLL_CANCEL     = 2,  // Cancel enroll finger
    FP_EVENT_ENROLL_DONE       = 3,  // Enroll finger succeed
    FP_EVENT_ENROLL_FAILED     = 4,  // Enroll Finger failed
    FP_EVENT_ENROLL_AREA_DUP   = 5,  // Enroll finger failed with duplicate area
    FP_EVENT_ENROLL_FINGER_DUP = 6,  // Enroll finger failed, duplicate finger
    FP_EVENT_ENROLL_NO_FINGER  = 7,  // Enroll finger failed no finger
    FP_EVENT_DETECT_SUCCEED    = 8,  // Finger detected succeed
    FP_EVENT_DETECT_FAILED     = 9,  // Finger detected failed
    FP_EVENT_NAVI              = 10, // Navigation occurred
} FP_EVENT_T;

typedef enum {
    FP_NAVI_0 = 1 << 0, // Down
    FP_NAVI_1 = 1 << 1, // Left
    FP_NAVI_2 = 1 << 2, // Up
    FP_NAVI_3 = 1 << 3, // Right
} FP_NAVI;

/**
 * @brief  Init the fingerprint module.
 * This function must be called first before any fingperprint process
 * @param  uid the user uid
 * @retval BSP_ERROR_NONE if init successfuly, failed else.
 */
int16_t BSP_FP_Init(uint16_t uid);

/**
 * @brief  DeInit the fingerprint module.
 * @retval BSP_ERROR_NONE if deinit successfuly, failed else.
 */
int16_t BSP_FP_DeInit();

/**
 * @brief  Fingerprint module enter low power mode.
 * @retval BSP_ERROR_NONE if enter low power mode successfuly, failed else.
 */
int16_t BSP_FP_Enter_LowPower();

/**
 * @brief  Fingerprint module exit low power mode.
 * @retval BSP_ERROR_NONE if exit low power mode successfuly, failed else.
 */
int16_t BSP_FP_Exit_LowPower();

/**
 * @brief  Calibrate the fingerprint module. T
 * his function should be called in factory calibration process. The fingerprint
 * sensor should be clean when calling the function.
 * @retval BSP_ERROR_NONE if calibrate successfuly, failed else.
 */
int16_t BSP_FP_Calibrate();

/**
 * @brief  Set the user uid.
 * @param  uid the user uid
 * @retval BSP_ERROR_NONE if successfuly, failed else.
 */
int16_t BSP_FP_SetUser(int16_t uid);

/**
 * @brief  Get the user uid.
 * @param  uid point to output user uid
 * @retval BSP_ERROR_NONE if successfuly, failed else.
 */
int16_t BSP_FP_GetUser(int16_t *uid);

/**
 * @brief  Get all the user uids.
 * @param  uids point to output user uid buffer. The buffer size should be large
 * than @FP_MAX_USER_UID
 * @param  uid_num output user uid count
 * @retval BSP_ERROR_NONE if successfuly, failed else.
 */
int16_t BSP_FP_GetAllUsers(uint16_t *uids, uint16_t *uid_num);

/**
 * @brief  Delete the user uid. Can't delete the current user
 * @param  uid the user uid
 * @retval BSP_ERROR_NONE if successfuly, failed else.
 */
int16_t BSP_FP_DltUser(int16_t uid);

/**
 * @brief  Get current user's fids.
 * @param  fids pint to the output buffer store the fids. The buffer must be
 * large than @FP_MAX_USER_FID
 * @param  fid_num output fid count.
 * @retval BSP_ERROR_NONE if successfuly, failed else.
 */
int16_t BSP_FP_GetCurrentUserFid(uint16_t *fids, uint8_t *fid_num);

/**
 * @brief  Get a user's fids.
 * @param  uid user uid
 * @param  fids pint to the output buffer store the fids. The buffer must be
 * large than @FP_MAX_USER_FID
 * @param  fid_num output fid count.
 * @retval BSP_ERROR_NONE if successfuly, failed else.
 */
int16_t BSP_FP_GetAUserFid(uint16_t uid, uint16_t *fids, uint8_t *fid_num);

/**
 * @brief  Delete current user's fid.
 * @param  fid the fid
 * @retval BSP_ERROR_NONE if successfuly, failed else.
 */
int16_t BSP_FP_DltUsrFid(uint16_t fid);

/**
 * @brief  Deleate current user's all fids and deleate all other users.
 * @retval BSP_ERROR_NONE if successfuly, failed else.
 */
int16_t BSP_FP_Reset();

/**
 * @brief  Begin to enroll fingerprint.
 * Once this function is called, the bsp layer will continue enroll fingerprint
 * until the enrolling completed(succeeded or failed) or @BSP_FP_EnrollCancel is
 * called. During the enrolling process, The callback(@BSP_FP_LISENTER_T, if
 * registered) will be called to notify the status of enrolling.
 * @param  fid output the new fid
 * @retval BSP_ERROR_NONE if successfuly, failed else.
 */
int16_t BSP_FP_EnrollStart(uint16_t *fid);

/**
 * @brief  cancel the pre-enroll process.
 * @param  fid the fid
 * @retval BSP_ERROR_NONE if successfuly, failed else.
 */
int16_t BSP_FP_EnrollCancel(uint16_t fid);

/**
 * @brief  Start detect finger touch continously
 * @param  continoues, true detect continouesly, else detect only once.
 * @retval BSP_ERROR_NONE if successfuly, failed else.
 */
int16_t BSP_FP_DetectStart(bool continoues);

/**
 * @brief  Stop detect finger touch
 * @param
 * @retval BSP_ERROR_NONE if successfuly, failed else.
 */
int16_t BSP_FP_DetectStop();

/**
 * @brief  Start checking fingerprint navigation
 * @param
 * @retval BSP_ERROR_NONE if successfuly, failed else.
 */
int16_t BSP_FP_NaviStart();

/**
 * @brief  Stop checking fingerprint navigation
 * @param
 * @retval BSP_ERROR_NONE if successfuly, failed else.
 */
int16_t BSP_FP_NaviStop();

/**
 * The callback type. Used to notify uplayer the enrolling or valication status.
 */
typedef void (*BSP_FP_LISENTER_T)(FP_EVENT_T BSP_FP_EVENT, uint16_t fid);

/**
 * @brief  Register the callback to listener the fingerprint event.
 * @param  listener the listener, NULL to stop listener
 * @retval BSP_ERROR_NONE if successfuly, failed else.
 */
int16_t BSP_FP_Reg(BSP_FP_LISENTER_T listener);