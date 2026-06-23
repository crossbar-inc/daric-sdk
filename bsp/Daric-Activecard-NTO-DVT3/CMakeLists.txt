add_library(${CONFIG_BOARD} STATIC)

# Get project and board configurations
target_link_libraries(${CONFIG_BOARD} PRIVATE configs)

target_link_libraries(${CONFIG_BOARD} PRIVATE driver)
target_link_libraries(${CONFIG_BOARD} PRIVATE threadx)
target_link_libraries(${CONFIG_BOARD} PRIVATE c)

target_include_directories(${CONFIG_BOARD} PUBLIC ./)


target_include_directories(${CONFIG_BOARD} PUBLIC ../include)

# LCD
if(CONFIG_LCD_COMMON)
#Inc
    target_include_directories(${CONFIG_BOARD} PUBLIC ../Components/Common)
#Src
    target_sources(${CONFIG_BOARD} PRIVATE daric_activecard_nto_lcm.c)
    target_sources(${CONFIG_BOARD} PRIVATE ../Components/Common/lcd_common.c)
    target_sources(${CONFIG_BOARD} PRIVATE ../Components/Common/lcd_array_data.c)
    if(CONFIG_LCD_SSD1680_SPI)
        target_sources(${CONFIG_BOARD} PRIVATE ../Components/SSD1680/lcd_ssd1680_spi.c)
    endif()
    if(CONFIG_LCD_SSD1685_SPI)
        target_sources(${CONFIG_BOARD} PRIVATE ../Components/SSD1685/lcd_ssd1685_spi.c)
    endif()
    if(CONFIG_LCD_ED028TC1_SPI)
        target_link_libraries(${CONFIG_BOARD} PRIVATE filex)
        target_sources(${CONFIG_BOARD} PRIVATE ../Components/LCD-ED028TC1/lcd_ed028tc1_spi.c)
    endif()
endif()

# RTC
if(CONFIG_RTC_INS5T8900)
    target_include_directories(${CONFIG_BOARD} PUBLIC ../Components/INS5T8900)
    target_sources(${CONFIG_BOARD} PRIVATE ../Components/INS5T8900/rtc_ins5t8900.c)
endif()

# PMIC
if(CONFIG_PMIC_TG28)
    target_include_directories(${CONFIG_BOARD} PUBLIC ../Components/TG28)
    target_sources(${CONFIG_BOARD} PRIVATE ../Components/TG28/tg28.c)
endif()

if(CONFIG_BATT_ETA4662)
    target_include_directories(${CONFIG_BOARD} PUBLIC ../Components/eta4662)
    target_sources(${CONFIG_BOARD} PRIVATE ../Components/eta4662/eta4662.c)
endif()

#PMU Power control
if(CONFIG_PMU_POWER_CTRL)
    target_sources(${CONFIG_BOARD} PRIVATE ./daric_pm.c)
endif()
# Charger

# RGB LED
if(CONFIG_RGB_LED)
    target_include_directories(${CONFIG_BOARD} PUBLIC ../Components/AW2023)
    target_include_directories(${CONFIG_BOARD} PUBLIC ../Components/Common)
    target_sources(${CONFIG_BOARD} PRIVATE ../Components/AW2023/aw2023.c)
    target_sources(${CONFIG_BOARD} PRIVATE ./daric_activecard_nto_rgb_led.c)
endif()

# TOUCH
if(CONFIG_TOUCH)
    target_include_directories(${CONFIG_BOARD} PUBLIC ../Components/CST9220)
    target_include_directories(${CONFIG_BOARD} PUBLIC ../Components/Common)
    target_sources(${CONFIG_BOARD} PRIVATE ../Components/CST9220/hyn_cst92xx.c)
    target_sources(${CONFIG_BOARD} PRIVATE ../Components/CST9220/hyn_i2c.c)
    target_sources(${CONFIG_BOARD} PRIVATE ../Components/CST9220/hyn_ts_ext.c)
    target_sources(${CONFIG_BOARD} PRIVATE ./daric_activecard_nto_ts.c)
    target_sources(${CONFIG_BOARD} PRIVATE ../Components/Common/Touch_common.c)
endif()

# Fingerprint
if(CONFIG_FINGERPRINT)
    if(APPLE OR CMAKE_HOST_SYSTEM_NAME STREQUAL "Darwin")
    target_link_libraries(${CONFIG_BOARD} INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/../Components/mlb/libmlb_daric32_float_soft.a)
    else()
    target_link_libraries(${CONFIG_BOARD} INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/../Components/mlb/libmlb_daric32_float_softfp.a)
    endif()
    target_include_directories(${CONFIG_BOARD} PUBLIC ../Components/mlb)
    target_sources(${CONFIG_BOARD} PUBLIC ../Components/mlb/mlb_platform.c)
    target_sources(${CONFIG_BOARD} PUBLIC ../Components/mlb/mlb_extend_intf.c)
    target_sources(${CONFIG_BOARD} PRIVATE ./daric_fingerprint.c)
    #target_sources(${CONFIG_BOARD} PRIVATE ../Components/mlb/mlb_test.c)
    target_link_libraries(${CONFIG_BOARD} PRIVATE filex_levelx)
endif()

# NFC
if(CONFIG_NFC)
    add_subdirectory(../Components/NFC_2 NFC)
    target_link_libraries(${CONFIG_BOARD} PRIVATE NFC)
endif()

# BT
if(CONFIG_BT)
    target_sources(${CONFIG_BOARD} PRIVATE daric_bt.c)
endif()

# USB
if(CONFIG_USB_DETEC)
    target_sources(${CONFIG_BOARD} PRIVATE ./daric_activecard_nto_usb.c)
endif()

# Vibarator
if(CONFIG_VIBRATOR)
    target_sources(${CONFIG_BOARD} PRIVATE ./daric_activecard_nto_vibrator.c)
    target_include_directories(${CONFIG_BOARD} PUBLIC ../Components/AWHaptivNv)
    target_sources(${CONFIG_BOARD} PRIVATE ../Components/AWHapticNv/haptic_nv.c)
    target_sources(${CONFIG_BOARD} PRIVATE ../Components/AWHapticNv/haptic_nv_config.c)
    target_sources(${CONFIG_BOARD} PRIVATE ../Components/AWHapticNv/haptic_nv_params.c)
    target_sources(${CONFIG_BOARD} PRIVATE ../Components/AWHapticNv/aw8623x.c)
    target_sources(${CONFIG_BOARD} PRIVATE ../Components/AWHapticNv/aw8624x.c)
    target_sources(${CONFIG_BOARD} PRIVATE ../Components/AWHapticNv/aw862x.c)
    target_sources(${CONFIG_BOARD} PRIVATE ../Components/AWHapticNv/aw862xx.c)

    target_sources(${CONFIG_BOARD} PRIVATE ./daric_vibrator_linear.c)
    target_sources(${CONFIG_BOARD} PRIVATE ./daric_vibrator_rot.c)
endif()

# Board information
target_sources(${CONFIG_BOARD} PRIVATE ./daric_board.c)
