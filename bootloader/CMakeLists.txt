cmake_minimum_required(VERSION 3.31)

set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_VERSION 1)

# specify cross-compilers and tools
set(CMAKE_C_COMPILER arm-none-eabi-gcc)
set(CMAKE_ASM_COMPILER arm-none-eabi-gcc)
set(CMAKE_AR arm-none-eabi-ar)
set(CMAKE_OBJCOPY arm-none-eabi-objcopy)
set(CMAKE_OBJDUMP arm-none-eabi-objdump)
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

project(bootloader C ASM)

set(CMAKE_C_STANDARD 11)

#Uncomment for hardware floating point
add_compile_options(-mcpu=cortex-m4 -mthumb -mthumb-interwork)
add_compile_options(-ffunction-sections -fdata-sections -fno-common -fmessage-length=0)

# Enable assembler files preprocessing
add_compile_options($<$<COMPILE_LANGUAGE:ASM>:-x$<SEMICOLON>assembler-with-cpp>)

add_compile_options(-Os -g) # Optimize for size: the bootloader must fit in 64 KB

# HAL sources (only the modules enabled in stm32f4xx_hal_conf.h)
set(HAL_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../Drivers/STM32F4xx_HAL_Driver)
set(BOOT_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/Core/Src/main.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Core/Src/system_stm32f4xx.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Core/Src/stm32f4xx_it.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Core/Src/syscalls.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Core/Src/sysmem.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Core/Src/boot_flag.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Core/Src/boot_flash.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Core/Src/boot_jump.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Core/Src/boot_spi.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Core/Src/boot_protocol.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Core/Src/boot_log.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Core/Src/boot_crc.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Core/Src/boot_version.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Core/Src/boot_extflash.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Core/Src/boot_backup.c
    ${HAL_DIR}/Src/stm32f4xx_hal.c
    ${HAL_DIR}/Src/stm32f4xx_hal_rcc.c
    ${HAL_DIR}/Src/stm32f4xx_hal_rcc_ex.c
    ${HAL_DIR}/Src/stm32f4xx_hal_gpio.c
    ${HAL_DIR}/Src/stm32f4xx_hal_dma.c
    ${HAL_DIR}/Src/stm32f4xx_hal_flash.c
    ${HAL_DIR}/Src/stm32f4xx_hal_flash_ex.c
    ${HAL_DIR}/Src/stm32f4xx_hal_spi.c
    ${HAL_DIR}/Src/stm32f4xx_hal_uart.c
    ${HAL_DIR}/Src/stm32f4xx_hal_cortex.c
    ${HAL_DIR}/Src/stm32f4xx_hal_pwr.c
    ${HAL_DIR}/Src/stm32f4xx_hal_pwr_ex.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../Core/Startup/startup_stm32f407vetx.s
)

set(BOOT_INCLUDES
    ${CMAKE_CURRENT_SOURCE_DIR}/Core/Inc
    ${CMAKE_CURRENT_SOURCE_DIR}/../Drivers/STM32F4xx_HAL_Driver/Inc
    ${CMAKE_CURRENT_SOURCE_DIR}/../Drivers/STM32F4xx_HAL_Driver/Inc/Legacy
    ${CMAKE_CURRENT_SOURCE_DIR}/../Drivers/CMSIS/Device/ST/STM32F4xx/Include
    ${CMAKE_CURRENT_SOURCE_DIR}/../Drivers/CMSIS/Include
)

add_definitions(-DUSE_HAL_DRIVER -DSTM32F407xx)

add_link_options(-Wl,-gc-sections,--print-memory-usage,-Map=${PROJECT_BINARY_DIR}/boot.map)
add_link_options(-mcpu=cortex-m4 -mthumb -mthumb-interwork)
add_link_options(-T ${CMAKE_CURRENT_SOURCE_DIR}/STM32F407VGTX_BOOT.ld)

add_executable(boot.elf ${BOOT_SOURCES} ${CMAKE_CURRENT_SOURCE_DIR}/STM32F407VGTX_BOOT.ld)
target_include_directories(boot.elf PRIVATE ${BOOT_INCLUDES})

set(HEX_FILE ${PROJECT_BINARY_DIR}/boot.hex)
set(BIN_FILE ${PROJECT_BINARY_DIR}/boot.bin)

add_custom_command(TARGET boot.elf POST_BUILD
        COMMAND ${CMAKE_OBJCOPY} -Oihex $<TARGET_FILE:boot.elf> ${HEX_FILE}
        COMMAND ${CMAKE_OBJCOPY} -Obinary $<TARGET_FILE:boot.elf> ${BIN_FILE}
        COMMENT "Building ${HEX_FILE}
Building ${BIN_FILE}")
