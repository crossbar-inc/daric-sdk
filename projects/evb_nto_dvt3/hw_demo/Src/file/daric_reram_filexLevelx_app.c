/**
******************************************************************************
* @file    daric_levelx_app.c
* @author  OS Team
* @brief   This file's contents are the adaptation of Levelx for the ReRam storage.
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

/* Include necessary system files. */
#include "stdio.h"
#include "daric_hal_reram.h"
#include "reram_filex_levelx_app.h"

/*
 * ┌────────────────────────┬───────────────────────┬───────────┬──────────────────────┐
 * │    Address Range       │     Name in DARIC     │   Size    │ Name In Active Card  │
 * ├────────────────────────┼───────────────────────┼───────────┼──────────────────────┤
 * │ 6000 0000 ~ 6001 FFFF  │                 Boot 0│     128 KB│              Boot ROM│
 * ├────────────────────────┼───────────────────────┼───────────┼──────────────────────┤
 * │ 6002 0000 ~ 6003 FFFF  │                 Boot 1│     128 KB│           Boot loader│
 * ├────────────────────────┼───────────────────────┼───────────┼──────────────────────┤
 * │ 6004 0000 ~ 602D 9FFF  │               Firmware│    2664 KB│         User Firmware│
 * ├────────────────────────┼───────────────────────┼───────────┼──────────────────────┤
 * │ 602D A000 ~ 603D 9FFF  │                   Data│       1 MB│          FAT12 System│
 * ├────────────────────────┼───────────────────────┼───────────┼──────────────────────┤
 * │ 603D A000 ~ 603D BFFF  │       one-wantacounter│       8 KB│                      │
 * ├────────────────────────┼───────────────────────┼───────────┼──────────────────────┤
 * │ 603D C000 ~ 603D FFFF  │ Access Contorl Setting│      16 KB│                      │
 * ├────────────────────────┼───────────────────────┼───────────┼──────────────────────┤
 * │ 603E 0000 ~ 603E FFFF  │          Dataslot area│      64 KB│                      │
 * ├────────────────────────┼───────────────────────┼───────────┼──────────────────────┤
 * │ 603F 0000 ~ 603F FFFF  │           Keyslot Area│      64 KB│                      │
 * └────────────────────────┴───────────────────────┴───────────┴──────────────────────┘
 */

/* Private macro -------------------------------------------------------------*/
#define LX_DATA_RERAM_BASE_ADDR         0x602DA000
#define LX_DATA_TOTAL_BLOCKS            64
#define LX_DATA_SECTS_PER_BLOCK         32
#define LX_DATA_SECTOR_SIZE             512
#define LX_DATA_WORDS_PER_SECTOR        (LX_DATA_SECTOR_SIZE/sizeof(ULONG))

/* Buffer for levelx LX_NOR_FLASH sector cache. 
   This must be large enough for at least one sector, 
   which are typically 512 bytes in size.  
 */
unsigned char gLevelxSectorCache[512];

/* Define FileX global data structures.  */

LX_NOR_FLASH gReramLevelx;

t_ReramLevelxInfo gReRAMLevelxInfo = {
    .mediaPtr       = &gReramLevelx,
    .diskBaseAddr   = LX_DATA_RERAM_BASE_ADDR,
    .cache          = gLevelxSectorCache,
    .cacheSize      = sizeof(gLevelxSectorCache),
    .totalBlocks    = LX_DATA_TOTAL_BLOCKS,
    .SectsPerBlock  = LX_DATA_SECTS_PER_BLOCK,
    .wordsPerSect   = LX_DATA_WORDS_PER_SECTOR,
    .bInitialed     = 0,
};

#define FX_DATA_NUBER_OF_FATS          2
#define FX_DATA_DIRECTORY_ENTRIES      128
#define FX_DATA_HIDDEN_SECTORS         0
#define FX_DATA_TOTAL_SECTORS          1922
#define FX_DATA_SECTOR_SIZE            512
#define FX_DATA_SECTORS_PER_CLUSTER    1

/* Buffer for FileX FX_MEDIA sector cache. 
   This must be large enough for at least one sector, 
   which are typically 512 bytes in size.  
 */
unsigned char gDataSectorCache[512];

/* Define FileX global data structures.  */

FX_MEDIA        gReramDisk;

t_ReramDiskInfo gReRAMDiskInfo = {
    .mediaPtr       = &gReramDisk,
    .diskBaseAddr   = 0,            /* 
                                       FileX does not need to directly access ReRAM; 
                                       instead, it accesses ReRAM through LevelX, 
                                       so no address is required here. 
                                     */
    .cache          = gDataSectorCache,
    .cacheSize      = sizeof(gDataSectorCache),
    .volName        = "filex_levelx",
    .numOfFats      = FX_DATA_NUBER_OF_FATS,
    .dirEntries     = FX_DATA_DIRECTORY_ENTRIES,
    .hiddenSects    = FX_DATA_HIDDEN_SECTORS,
    .totalSects     = FX_DATA_TOTAL_SECTORS,
    .bytesPerSect   = FX_DATA_SECTOR_SIZE,
    .sectPerCluster = FX_DATA_SECTORS_PER_CLUSTER,
    .bInitialed     = 0,
};

t_ReramFilexLevelxInfo gReramFilexLevelxInfo = {
    .reramDiskInfo      = &gReRAMDiskInfo,
    .reramLevelxInfo    = &gReRAMLevelxInfo,
};

/**
 * @brief  Initializes the reram levelx structure with the specified configuration.
 * @retval LX status code indicating the success of initialization,
 *         LX_SUCCESS means success, other values indicate errors.
 *
 * This function sets up the base address, geometry, driver functions, and sector buffer
 * for the reram. It configures the read, write, erase, and verify operations
 * with specific implementations.
 */
uint32_t daricReramLevelxInit(void)
{
    return daricReramLevelxLoad(&gReRAMLevelxInfo);
}

/**
 * @brief  Erases all data in the ReRAM LevelX storage by writing 0xFF to head sector of blocks.
 * @retval LX status code indicating the success of the operation,
 *         LX_SUCCESS means all blocks were successfully erased,
 *         other values indicate errors during the erase process.
 *
 * This function performs a complete erase of the ReRAM LevelX storage by:
 * Iterating through all blocks in the storage
 * The operation uses the hardware abstraction layer (HAL) for ReRAM writes.
 */
uint32_t daricDataLevelxEraseAll(void)
{
    return daricReramLevelxEraseAllHead(&gReRAMLevelxInfo);
}

ULONG  daricReramLevelxPrintAllHead(void)
{
    ULONG reramBaseAddr;
    ULONG bytesPerSector;
    t_ReramLevelxInfo *reramLevelxInfo = &gReRAMLevelxInfo;
    UCHAR buffer[16];

    reramBaseAddr = reramLevelxInfo->diskBaseAddr;
    bytesPerSector = reramLevelxInfo->wordsPerSect * sizeof(ULONG);

    for (int i = 0; i < reramLevelxInfo->totalBlocks; i++)
    {
        HAL_RERAM_Read(reramBaseAddr, (UCHAR *)buffer, 16);
        printf("Reram addr 0x%lx: ", reramBaseAddr);
        for (int j = 0; j < 16; j++)
        {
            printf("%02x ", buffer[j]);
        }
        printf("\r\n");
        reramBaseAddr  += reramLevelxInfo->SectsPerBlock * bytesPerSector;
    }

    return(LX_SUCCESS);
}

/**
 * @brief  Initializes the data disk info according to the specified parameters.
 * @param  none.
 * @retval FS status indicating the success or error of the initialization process,
 *         FX_SUCCESS means success, return a non-zero value means failure.
 * 
 * This function configures the filex disk according to the parameters specified
 * in the gReRAMDiskInfo structure, including cache, numOfFats, and other
 * relevant settings.
 */
uint32_t daricDataDiskLoad(void)
{
    UINT status;

//    status = daricReramLevelxLoad(&gReRAMLevelxInfo);
//    if (status != LX_SUCCESS)
//    {
//        printf("reram flash open error, errorNum:%d", status);
//        return status;
//    }

    status = daricReRamLevelxDiskLoad(&gReramFilexLevelxInfo);
    if (status != FX_SUCCESS)
    {
        printf("reram filex open error, errorNum:%d", status);
        return status;
    }

    return(LX_SUCCESS);
}

/**
 * @brief  Format the data partition specified by the parameters.
 * @param  reramDiskInfo Pointer to a t_ReramDiskInfo structure that contains
 *         the configuration information for the specified filex disk.
 * @retval FS status indicating the success or error of the initialization process,
 *         FX_SUCCESS means success, return a non-zero value means failure.
 * 
 * This function format the file system specified by the parameters.
 * Please note that this interface will destroy the data on the file system. 
 * Make sure that the data on the disk is no longer needed 
 * before calling this interface.
 */
uint32_t daricDataDiskFormat(void)
{
    UINT status;

//    status = daricReramLevelxLoad(&gReRAMLevelxInfo);
//    if (status != LX_SUCCESS)
//    {
//        printf("reram flash open error, errorNum:%d", status);
//        return status;
//    }

    status = daricReRamLevelxDiskFormat(&gReramFilexLevelxInfo);
    if (status != FX_SUCCESS)
    {
        printf("reram flash format error, errorNum:%d", status);
        return status;
    }

    return(FX_SUCCESS);
}


/**
 * @brief  This function initializes the filex.
 * @param  none.
 * @retval none.
 *
 * This function initializes the various control data structures
 * for the FileX System component
 */
void daricFilesystemInit(void)
{
    /* Initialize NOR flash.  */
    lx_nor_flash_initialize();
    /* Initialize filex.  */
    fx_system_initialize();

}