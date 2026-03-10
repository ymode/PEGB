cmake_minimum_required(VERSION 3.13)

# Must be set before project()
set(PICO_BOARD pimoroni_explorer)
set(PICO_BOARD_HEADER_DIRS ${CMAKE_CURRENT_LIST_DIR}/src/boards)

include(pico_sdk_import.cmake)

project(pe_gbc C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()

add_executable(pe_gbc
    src/main.c
    src/display.c
    src/input.c
    lib/minigb_apu/minigb_apu.c
)

pico_generate_pio_header(pe_gbc ${CMAKE_CURRENT_LIST_DIR}/src/display.pio)

target_include_directories(pe_gbc PRIVATE
    src
    lib/peanut-gb
    lib/minigb_apu
)

# Overclock to 250 MHz for comfortable emulation headroom
target_compile_definitions(pe_gbc PRIVATE
    PICO_FLASH_SPI_CLKDIV=2
    # Peanut-GB config
    ENABLE_LCD=1
    ENABLE_SOUND=1
    MINIGB_APU_AUDIO_FORMAT_S16SYS=1
    PEANUT_GB_12_COLOUR=1
    PEANUT_GB_HIGH_LCD_ACCURACY=0
)

target_link_libraries(pe_gbc
    pico_stdlib
    hardware_pio
    hardware_dma
    hardware_pwm
    hardware_clocks
    hardware_vreg
)

pico_add_extra_outputs(pe_gbc)
