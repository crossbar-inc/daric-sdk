cmake_minimum_required(VERSION 3.16)

set(CMAKE_C_STANDARD 99)
set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_TOOLCHAIN_FILE toolchain-arm-none-eabi.cmake)

project(hw_demo C ASM)

set(TARGET hw_demo)
set(GCC_PROJECT_DIR "${CMAKE_CURRENT_LIST_DIR}/../..")
include(${GCC_PROJECT_DIR}/project.cmake)

aux_source_directory(${GCC_PROJECT_DIR}/../Src main_srcs)
aux_source_directory(${GCC_PROJECT_DIR}/../Src/service main_srcs)
aux_source_directory(${GCC_PROJECT_DIR}/../Src/gui main_srcs)
aux_source_directory(${GCC_PROJECT_DIR}/../Src/cfg main_srcs)
aux_source_directory(${GCC_PROJECT_DIR}/../Src/nfc main_srcs)
aux_source_directory(${GCC_PROJECT_DIR}/../Src/bt main_srcs)
aux_source_directory(${GCC_PROJECT_DIR}/../Src/gui/480x600 main_srcs)
file(GLOB_RECURSE gui_srcs ${GCC_PROJECT_DIR}/../Src/gui/480x600/daric_gui/*.c)


list(APPEND main_srcs ${gui_srcs})
set_source_files_properties(
    ${GCC_PROJECT_DIR}/../Src/gui/480x600/daric_gui/daric_gui_specifications.c
    ${GCC_PROJECT_DIR}/../Src/gui/480x600/daric_gui/daric_gui_resources.c
)


add_executable(${TARGET} ${main_srcs})
target_include_directories(${TARGET} PRIVATE ${GCC_PROJECT_DIR}/../Inc)


add_library(configs INTERFACE)
target_include_directories(configs INTERFACE ${GCC_PROJECT_DIR}/../Inc/config)
target_include_directories(configs INTERFACE ${GCC_PROJECT_DIR}/../Inc/service)
target_include_directories(configs INTERFACE ${GCC_PROJECT_DIR}/../Inc/gui)
target_include_directories(configs INTERFACE ${GCC_PROJECT_DIR}/../Inc/gui/480x600/daric_gui)
target_include_directories(configs INTERFACE ${GCC_PROJECT_DIR}/../Inc/file)
target_include_directories(configs INTERFACE ${GCC_PROJECT_DIR}/../Inc/nfc)
target_include_directories(configs INTERFACE ${GCC_PROJECT_DIR}/../Inc/bt)


include(${GCC_PROJECT_DIR}/../../../../bsp/${CONFIG_BOARD}/board.cmake)
include(${GCC_PROJECT_DIR}/scripts/cmake/gen_configs.cmake)
target_link_libraries(${TARGET} PRIVATE configs)

add_subdirectory(${GCC_PROJECT_DIR}/../../../../middleware/third_party/cmsis        cmsis)
add_subdirectory(${GCC_PROJECT_DIR}/../../../../drivers                 driver)
add_subdirectory(${GCC_PROJECT_DIR}/../../../../middleware/third_party/threadx      threadx)
add_subdirectory(${GCC_PROJECT_DIR}/../../../../middleware/third_party/threadx/guix guix)
add_subdirectory(${GCC_PROJECT_DIR}/../../../../middleware/third_party/threadx/filex filex)
add_subdirectory(${GCC_PROJECT_DIR}/../../../../middleware/third_party/threadx/levelx levelx)
add_subdirectory(${GCC_PROJECT_DIR}/../../../../middleware/third_party/threadx/filex_levelx filex_levelx)
add_subdirectory(${GCC_PROJECT_DIR}/../../../../middleware/third_party         libs)
add_subdirectory(${GCC_PROJECT_DIR}/../../../../bsp                     bsp)
add_subdirectory(${GCC_PROJECT_DIR}/../../../../middleware/crossbar/usb_device_library   usb_device)
# add_subdirectory(${GCC_PROJECT_DIR}/../../../../daric_sdk/middleware/SEGGER_RTT_V630d         rtt)

# target_link_libraries(${TARGET} PRIVATE rtt)
target_link_libraries(${TARGET} PRIVATE driver)
target_link_libraries(${TARGET} PRIVATE threadx)
target_link_libraries(${TARGET} PRIVATE guix)
target_link_libraries(${TARGET} PRIVATE filex)
target_link_libraries(${TARGET} PRIVATE levelx)
target_link_libraries(${TARGET} PRIVATE filex_levelx)
target_link_libraries(${TARGET} PRIVATE cmsis)
target_link_libraries(${TARGET} PRIVATE bsp)
target_link_libraries(${TARGET} PRIVATE usb_device)
target_link_libraries(${TARGET} PRIVATE NFC)

if(CONFIG_MPC_SDK STREQUAL "y")
    target_link_libraries(${TARGET} PRIVATE rust_mpc_sdk)
endif()

# Make sure "libs" is always linked behind all other libraries.
target_link_libraries(${TARGET} PRIVATE libs)

# Force bndivision_le symbol from daric_crypto to be linked (needed by mini_gmp SCE wrapper)
# This ensures the SCE hardware division function is available for BICYCL acceleration
target_link_options(${TARGET} PRIVATE "-Wl,-u,bndivision_le")

set_target_properties(${TARGET} PROPERTIES SUFFIX ".elf")

if(CONFIG_EXECUTION_MODE STREQUAL "SRAM_MODE")
    set(GCC_LD_FILE "../ld/daric_sram.ld")
    message(DEBUG "Compiling for SRAM_MODE")
elseif(CONFIG_EXECUTION_MODE STREQUAL "ITCM_MODE")
    set(GCC_LD_FILE "../ld/daric_itcm.ld")
    message(DEBUG "Compiling for ITCM_MODE")
elseif(CONFIG_EXECUTION_MODE STREQUAL "RERAM_BLOADER_MODE")
    set(GCC_LD_FILE "../ld/daric_reram_bloader.ld")
    message(DEBUG "Compiling for RERAM_BLOADER_MODE")
else()
    set(GCC_LD_FILE "../ld/daric_reram.ld")
    message(DEBUG "Compiling for RERAM_MODE")
endif()

target_link_options(${TARGET}
    PRIVATE
        -T ${GCC_LD_FILE}
        -Wl,-Map=${TARGET}.map
        # Allow multiple definitions for Rust allocator symbols
        # Multiple Rust staticlibs (bicycl_support, bicycl_testcase, tecdsa_embedded)
        # all define the same ThreadX allocator from bicycl_common
        -Wl,--allow-multiple-definition
)

add_custom_target(${TARGET}.bin ALL
    DEPENDS ${TARGET}
        COMMAND ${CMAKE_OBJCOPY} -Obinary ${TARGET}.elf ${TARGET}.bin
        COMMAND ${CMAKE_OBJCOPY} -Oihex ${TARGET}.elf ${TARGET}.hex
        COMMAND ${CMAKE_OBJDUMP} -S ${TARGET}.elf > ${TARGET}.S
)
