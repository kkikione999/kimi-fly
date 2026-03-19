#!/bin/bash
# STM32F411CEU6 Sensor Test Build Script

set -e

TARGET=sensor_test
BUILD_DIR=build

# ARM toolchain
PREFIX=/Users/ll/embedded-tools/arm-gnu-toolchain-13.3.rel1-darwin-arm64-arm-none-eabi/bin/arm-none-eabi-
CC=${PREFIX}gcc
OBJCOPY=${PREFIX}objcopy
SIZE=${PREFIX}size

# STM32Cube framework paths
STM32CUBE_DIR=/Users/ll/.platformio/packages/framework-stm32cubef4
HAL_DIR=${STM32CUBE_DIR}/Drivers/STM32F4xx_HAL_Driver
CMSIS_DIR=${STM32CUBE_DIR}/Drivers/CMSIS
CMSIS_DEVICE=${CMSIS_DIR}/Device/ST/STM32F4xx

# MCU flags
MCU="-mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard"

# Compiler flags
CFLAGS="$MCU -Wall -Og -g3 -gdwarf-2 \
  -ffunction-sections -fdata-sections \
  -DSTM32F411xE -DUSE_HAL_DRIVER \
  -Isrc \
  -I${HAL_DIR}/Inc \
  -I${CMSIS_DIR}/Include \
  -I${CMSIS_DEVICE}/Include"

# Create build directory
mkdir -p ${BUILD_DIR}

# Create linker script
echo "Creating linker script..."
cat > ${BUILD_DIR}/stm32f411.ld << 'EOF'
MEMORY {
  FLASH (rx) : ORIGIN = 0x08000000, LENGTH = 512K
  RAM (rwx)  : ORIGIN = 0x20000000, LENGTH = 128K
}

_estack = 0x20020000;

ENTRY(Reset_Handler)

SECTIONS {
  .text : {
    KEEP(*(.isr_vector))
    *(.text*)
    *(.rodata*)
    . = ALIGN(4);
    /* C++ constructors / destructors and init arrays */
    KEEP(*(.init))
    KEEP(*(.fini))
    . = ALIGN(4);
    __preinit_array_start = .;
    KEEP(*(.preinit_array*))
    __preinit_array_end = .;
    . = ALIGN(4);
    __init_array_start = .;
    KEEP(*(SORT(.init_array.*)))
    KEEP(*(.init_array*))
    __init_array_end = .;
    . = ALIGN(4);
    __fini_array_start = .;
    KEEP(*(SORT(.fini_array.*)))
    KEEP(*(.fini_array*))
    __fini_array_end = .;
    . = ALIGN(4);
    _etext = .;
  } > FLASH

  _sidata = LOADADDR(.data);

  .data : {
    _sdata = .;
    *(.data*)
    . = ALIGN(4);
    _edata = .;
  } > RAM AT > FLASH

  .bss : {
    _sbss = .;
    *(.bss*)
    *(COMMON)
    . = ALIGN(4);
    _ebss = .;
  } > RAM

  ._user_heap_stack : {
    . = ALIGN(8);
    end = .;
    . = . + 0x400;
    . = . + 0x1000;
    . = ALIGN(8);
  } > RAM
}
EOF

echo "Compiling startup file..."
${CC} $MCU -x assembler-with-cpp -c \
  ${CMSIS_DEVICE}/Source/Templates/gcc/startup_stm32f411xe.s \
  -o ${BUILD_DIR}/startup_stm32f411xe.o

echo "Compiling system file..."
${CC} ${CFLAGS} -c \
  ${CMSIS_DEVICE}/Source/Templates/system_stm32f4xx.c \
  -o ${BUILD_DIR}/system_stm32f4xx.o

echo "Compiling HAL library..."
HAL_SRCS=(
  stm32f4xx_hal.c
  stm32f4xx_hal_cortex.c
  stm32f4xx_hal_gpio.c
  stm32f4xx_hal_i2c.c
  stm32f4xx_hal_spi.c
  stm32f4xx_hal_uart.c
  stm32f4xx_hal_rcc.c
  stm32f4xx_hal_rcc_ex.c
  stm32f4xx_hal_pwr.c
  stm32f4xx_hal_pwr_ex.c
  stm32f4xx_hal_dma.c
)

HAL_OBJS=""
for src in "${HAL_SRCS[@]}"; do
  out="${BUILD_DIR}/${src%.c}.o"
  ${CC} ${CFLAGS} -c ${HAL_DIR}/Src/${src} -o ${out}
  HAL_OBJS="${HAL_OBJS} ${out}"
done

echo "Compiling application..."
${CC} ${CFLAGS} -c src/sensor_test_main.c -o ${BUILD_DIR}/sensor_test_main.o

echo "Linking..."
${CC} \
  ${BUILD_DIR}/startup_stm32f411xe.o \
  ${BUILD_DIR}/system_stm32f4xx.o \
  ${BUILD_DIR}/sensor_test_main.o \
  ${HAL_OBJS} \
  $MCU --specs=nano.specs -u _printf_float -lc -lm -lnosys \
  -Wl,--gc-sections \
  -Wl,-Map=${BUILD_DIR}/${TARGET}.map,-T${BUILD_DIR}/stm32f411.ld \
  -o ${BUILD_DIR}/${TARGET}.elf

${OBJCOPY} -O ihex   ${BUILD_DIR}/${TARGET}.elf ${BUILD_DIR}/${TARGET}.hex
${OBJCOPY} -O binary ${BUILD_DIR}/${TARGET}.elf ${BUILD_DIR}/${TARGET}.bin

echo "Build complete!"
${SIZE} ${BUILD_DIR}/${TARGET}.elf
