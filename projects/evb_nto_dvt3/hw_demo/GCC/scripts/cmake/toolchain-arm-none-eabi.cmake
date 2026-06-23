# usage
# cmake -DCMAKE_TOOLCHAIN_FILE=../toolchain-arm-none-eabi.cmake ../

# The Generic system name is used for embedded targets (targets without OS) in
set(CMAKE_SYSTEM_NAME          Generic)
set(CMAKE_SYSTEM_PROCESSOR     arm)

# Only need to set this if the toolchain isn't in your system path.
if (NOT ARM_COMPILER_PATH)
    set(ARM_COMPILER_PATH $ENV{ARM_NONE_EABI_TOOLCHAIN_BINPATH})

    if(ARM_COMPILER_PATH)
        get_filename_component(ARM_COMPILER_PATH "${ARM_COMPILER_PATH}" REALPATH)
        file(TO_CMAKE_PATH "${ARM_COMPILER_PATH}" ARM_COMPILER_PATH)
        string(APPEND ARM_COMPILER_PATH "/")
    endif()
endif()

message(VERBOSE "ARM_COMPILER_PATH @ ${ARM_COMPILER_PATH}")

# The toolchain prefix for all toolchain executables
set(CROSS_COMPILE arm-none-eabi-)

# specify the cross compiler. We force the compiler so that CMake doesn't
# attempt to build a simple test program as this will fail without us using
# the -nostartfiles option on the command line
set(CMAKE_C_COMPILER    ${ARM_COMPILER_PATH}${CROSS_COMPILE}gcc)
# set(CMAKE_CXX_COMPILER  ${ARM_COMPILER_PATH}/${CROSS_COMPILE}g++)
set(CMAKE_ASM_COMPILER  ${ARM_COMPILER_PATH}${CROSS_COMPILE}gcc)
set(CMAKE_LINKER        ${ARM_COMPILER_PATH}${CROSS_COMPILE}gcc)
set(CMAKE_SIZE_UTIL     ${ARM_COMPILER_PATH}${CROSS_COMPILE}size)
set(CMAKE_OBJCOPY       ${ARM_COMPILER_PATH}${CROSS_COMPILE}objcopy)
set(CMAKE_OBJDUMP       ${ARM_COMPILER_PATH}${CROSS_COMPILE}objdump)
set(CMAKE_NM_UTIL       ${ARM_COMPILER_PATH}${CROSS_COMPILE}gcc-nm)
set(CMAKE_AR            ${ARM_COMPILER_PATH}${CROSS_COMPILE}gcc-ar)
set(CMAKE_RANLIB        ${ARM_COMPILER_PATH}${CROSS_COMPILE}gcc-ranlib)

set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

# Because the cross-compiler cannot directly generate a binary without complaining, just test
# compiling a static library instead of an executable program
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

# Compiler and linker flags
set(CMAKE_COMMON_FLAGS "-mcpu=cortex-m7 -mfloat-abi=soft -g3 -ffunction-sections -fdata-sections -fno-strict-aliasing -fno-builtin -fno-common -Wall -Werror -Wshadow -Wdouble-promotion -Wno-unused-parameter -nostdlib")

set(CMAKE_C_FLAGS_INIT "${CMAKE_COMMON_FLAGS}")
set(CMAKE_CXX_FLAGS_INIT "${CMAKE_COMMON_FLAGS}")
set(CMAKE_ASM_FLAGS_INIT "${CMAKE_COMMON_FLAGS}")
set(CMAKE_EXE_LINKER_FLAGS_INIT "${LD_FLAGS} -nostdlib -specs=nosys.specs -Wl,--gc-sections,-print-memory-usage")

set(CMAKE_C_FLAGS_DEBUG_INIT "-O0")
set(CMAKE_CXX_ASM_FLAGS_DEBUG_INIT "-O0")
set(CMAKE_ASM_FLAGS_DEBUG_INIT "")
set(CMAKE_EXE_LINKER_FLAGS_DEBUG_INIT "")

set(CMAKE_C_FLAGS_RELEASE_INIT "-Os -flto")
set(CMAKE_CXX_FLAGS_RELEASE_INIT "-Os -flto")
set(CMAKE_ASM_FLAGS_RELEASE_INIT "")
set(CMAKE_EXE_LINKER_FLAGS_RELEASE_INIT "-flto")
