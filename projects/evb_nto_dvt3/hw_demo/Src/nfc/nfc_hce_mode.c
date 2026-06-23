/**
 ******************************************************************************
 * @file    nfc_hce_mode.c
 * @author  AP Team
 * @brief   Establish NFC routing, receive data through HCE and return data through response
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
#include "bluetooth_uart.h"
#include "nfc_hce_mode.h"
#include "nfc_service.h"
#include "tx_log.h"

#undef LOG_TAG
#define LOG_TAG "NFC_TAG"

#define APDU_INS_SELECT_CMD               0xA4
uint8_t rspNfcBuf[300];
uint8_t outputNfcBuf[1000];
uint16_t rsp_length;

uint8_t hce_callback(uint8_t *p_data, uint16_t data_length)
{
    uint16_t len;
    uint8_t ins = *(p_data + 1);

    LOGV("hce_callback ins = %0x, data_length = %d", ins, data_length);

	uint8_t hce_send_result = 0;
    switch(ins) {

        case APDU_INS_SELECT_CMD:
            {
                rsp_length = 2;
                *rspNfcBuf = 0x90;
                *(rspNfcBuf + 1) = 0x00;
            }
            return NFCC_HceSendData(rspNfcBuf, rsp_length);
        case UPDATE_BINARY:
            outputNfcBuf[0] = *(p_data + 4);//Lc
            len = outputNfcBuf[0];
            if (outputNfcBuf[0] < 0x80) {
                memcpy(outputNfcBuf+1,p_data+5,outputNfcBuf[0]);  //L+V
            } else {
                outputNfcBuf[0] = 0x81;
                outputNfcBuf[1] = len;
                memcpy(outputNfcBuf+2,p_data+5,outputNfcBuf[1]);  //0x81+L+V
            }
            rsp_length = 2;
            *rspNfcBuf = 0x90;
            *(rspNfcBuf + 1) = 0x00;
            break;
        case READ_BINARY:
            if(outputNfcBuf[0] != 0x81) {
                if(outputNfcBuf[0] < 0x7E) {  //L+V
                    *rspNfcBuf = outputNfcBuf[0]+2;
                    memcpy(rspNfcBuf+1,outputNfcBuf+1,outputNfcBuf[0]);
                    *(rspNfcBuf + outputNfcBuf[0]+1) = 0x90;
                    *(rspNfcBuf + outputNfcBuf[0]+2) = 0x00;
                } else {  //0x81+L+V
                    *rspNfcBuf = 0x81;
                    *(rspNfcBuf + 1)= outputNfcBuf[0]+2;
                    memcpy(rspNfcBuf+2,outputNfcBuf+1,outputNfcBuf[0]);
                    *(rspNfcBuf + outputNfcBuf[0]+2) = 0x90;
                    *(rspNfcBuf + outputNfcBuf[0]+3) = 0x00;
                }
            } else {
                if(outputNfcBuf[1] < 0xFE) {  //0x81+00+L2+V
                    * rspNfcBuf = outputNfcBuf[0];
                    *(rspNfcBuf+1) = outputNfcBuf[1]+2;
                    memcpy(rspNfcBuf+2,outputNfcBuf+2,outputNfcBuf[1]);
                    *(rspNfcBuf + outputNfcBuf[1]+2) = 0x90;
                    *(rspNfcBuf + outputNfcBuf[1]+3) = 0x00;
                } else {  //0x82+L1+L2+V
                    *rspNfcBuf = 0x82;
                    *(rspNfcBuf+1) = (outputNfcBuf[1]+2)>>8;
                    *(rspNfcBuf+2) = (outputNfcBuf[1]+2);
                    memcpy(rspNfcBuf+3,outputNfcBuf+2,outputNfcBuf[1]);
                    *(rspNfcBuf + outputNfcBuf[1]+3) = 0x90;
                    *(rspNfcBuf + outputNfcBuf[1]+4) = 0x00;
                }
            }
            break;
        default:
            LOGV("NFCC_HceSendData 6D 00");
            rsp_length = 2;
            *rspNfcBuf = 0x6D;
            *(rspNfcBuf + 1) = 0x00;
            break;
        }
    hce_send_result = (NFCC_HceSendData(rspNfcBuf, rsp_length));
    LOGD("hce_send_result =%d ", hce_send_result);
    return hce_send_result;
}
