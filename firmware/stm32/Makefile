# STM32F411CEU6 Makefile for kimi-fly project
# Hardware verification firmware

# Target name (can be overridden: make TARGET=uart_comm_test)
TARGET = verify_hardware

# Build directory
BUILD_DIR = build
STARTUP_DIR = startup

# Toolchain
PREFIX = arm-none-eabi-
CC = $(PREFIX)gcc
CXX = $(PREFIX)g++
AS = $(PREFIX)gcc -x assembler-with-cpp
CP = $(PREFIX)objcopy
SZ = $(PREFIX)size
AR = $(PREFIX)ar
LD = $(PREFIX)gcc
HEX = $(CP) -O ihex
BIN = $(CP) -O binary -S

# MCU flags
MCU = -mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard

# Defines
DEFS = -DUSE_HAL_DRIVER -DSTM32F411xE

# Include paths
INCS = \
-I/Users/ll/.platformio/packages/framework-stm32cubef4/Drivers/STM32F4xx_HAL_Driver/Inc \
-I/Users/ll/.platformio/packages/framework-stm32cubef4/Drivers/CMSIS/Device/ST/STM32F4xx/Include \
-I/Users/ll/.platformio/packages/framework-stm32cubef4/Drivers/CMSIS/Include \
-Ihal \
-Idrivers \
-Imain \
-I../platform

# HAL Library sources
HAL_SRCS = \
/Users/ll/.platformio/packages/framework-stm32cubef4/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal.c \
/Users/ll/.platformio/packages/framework-stm32cubef4/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_cortex.c \
/Users/ll/.platformio/packages/framework-stm32cubef4/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_gpio.c \
/Users/ll/.platformio/packages/framework-stm32cubef4/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_i2c.c \
/Users/ll/.platformio/packages/framework-stm32cubef4/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_spi.c \
/Users/ll/.platformio/packages/framework-stm32cubef4/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_uart.c \
/Users/ll/.platformio/packages/framework-stm32cubef4/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_rcc.c \
/Users/ll/.platformio/packages/framework-stm32cubef4/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_rcc_ex.c \
/Users/ll/.platformio/packages/framework-stm32cubef4/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_pwr.c \
/Users/ll/.platformio/packages/framework-stm32cubef4/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_pwr_ex.c \
/Users/ll/.platformio/packages/framework-stm32cubef4/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_dma.c \
/Users/ll/.platformio/packages/framework-stm32cubef4/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_tim.c \
/Users/ll/.platformio/packages/framework-stm32cubef4/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_tim_ex.c

# Project sources (with full paths) - depends on TARGET
ifeq ($(TARGET),uart_comm_test)
PROJ_C_SRCS = \
$(STARTUP_DIR)/system_stm32f4xx.c \
hal/gpio.c \
hal/uart.c \
main/uart_comm_test.c
else
PROJ_C_SRCS = \
$(STARTUP_DIR)/system_stm32f4xx.c \
hal/gpio.c \
hal/i2c.c \
hal/spi.c \
hal/uart.c \
hal/pwm.c \
drivers/icm42688.c \
drivers/lps22hb.c \
main/verify_hardware.c
endif

# Assembly sources
ASM_SRCS = $(STARTUP_DIR)/startup_stm32f411xe.s

# VPATH for source search
VPATH = $(sort $(dir $(PROJ_C_SRCS)))
VPATH += $(STARTUP_DIR)

# Object files
PROJ_OBJECTS = $(addprefix $(BUILD_DIR)/,$(notdir $(PROJ_C_SRCS:.c=.o)))
HAL_OBJECTS = $(addprefix $(BUILD_DIR)/hal_,$(notdir $(HAL_SRCS:.c=.o)))
ASM_OBJECTS = $(addprefix $(BUILD_DIR)/,$(notdir $(ASM_SRCS:.s=.o)))
OBJECTS = $(PROJ_OBJECTS) $(HAL_OBJECTS) $(ASM_OBJECTS)

# Compile flags
CFLAGS = $(MCU) $(DEFS) $(INCS)
CFLAGS += -Wall -Wextra -Wno-unused-parameter
CFLAGS += -Og -g3 -gdwarf-2
CFLAGS += -ffunction-sections -fdata-sections
CFLAGS += --specs=nano.specs

# Assembly flags
ASFLAGS = $(MCU) $(DEFS) $(INCS) -Wall -fdata-sections -ffunction-sections

# Linker flags
LDFLAGS = $(MCU) -specs=nosys.specs -specs=nano.specs
LDFLAGS += -T$(STARTUP_DIR)/STM32F411CEU_FLASH.ld
LDFLAGS += -lc -lm -lnosys
LDFLAGS += -Wl,-Map=$(BUILD_DIR)/$(TARGET).map,--cref -Wl,--gc-sections

# Build rules
.PHONY: all clean flash flash-stlink size reset debug help uart-test

all: $(BUILD_DIR)/$(TARGET).elf $(BUILD_DIR)/$(TARGET).hex $(BUILD_DIR)/$(TARGET).bin size

# Create build directory
$(BUILD_DIR):
	@mkdir -p $(BUILD_DIR)

# Pattern rule for C files in project directories
$(BUILD_DIR)/%.o: %.c | $(BUILD_DIR)
	@echo "CC $<"
	@$(CC) -c $(CFLAGS) $< -o $@

# Pattern rule for assembly files
$(BUILD_DIR)/%.o: %.s | $(BUILD_DIR)
	@echo "AS $<"
	@$(AS) -c $(ASFLAGS) $< -o $@

# Pattern rule for HAL library files
$(BUILD_DIR)/hal_%.o: /Users/ll/.platformio/packages/framework-stm32cubef4/Drivers/STM32F4xx_HAL_Driver/Src/%.c | $(BUILD_DIR)
	@echo "CC [HAL] $<"
	@$(CC) -c $(CFLAGS) $< -o $@

# Link
$(BUILD_DIR)/$(TARGET).elf: $(OBJECTS)
	@echo "LD $@"
	@$(LD) $(OBJECTS) $(LDFLAGS) -o $@

# Generate hex
$(BUILD_DIR)/$(TARGET).hex: $(BUILD_DIR)/$(TARGET).elf
	@$(HEX) $< $@
	@echo "HEX $@"

# Generate bin
$(BUILD_DIR)/$(TARGET).bin: $(BUILD_DIR)/$(TARGET).elf
	@$(BIN) $< $@
	@echo "BIN $@"

# Display size
size: $(BUILD_DIR)/$(TARGET).elf
	@echo "\n=== Firmware Size ==="
	@$(SZ) $<

# Flash using OpenOCD
flash: $(BUILD_DIR)/$(TARGET).elf
	@echo "\n=== Flashing firmware ==="
	openocd -f interface/stlink.cfg -f target/stm32f4x.cfg \
		-c "program $(BUILD_DIR)/$(TARGET).elf verify reset exit"

# Flash using st-flash (alternative)
flash-stlink: $(BUILD_DIR)/$(TARGET).bin
	@echo "\n=== Flashing firmware (st-flash) ==="
	st-flash write $(BUILD_DIR)/$(TARGET).bin 0x08000000

# Clean build
clean:
	@rm -rf $(BUILD_DIR)
	@echo "Cleaned build directory"

# Reset target
reset:
	@openocd -f interface/stlink.cfg -f target/stm32f4x.cfg \
		-c "init; reset run; exit"

# Debug server
debug:
	@openocd -f interface/stlink.cfg -f target/stm32f4x.cfg

# Build UART communication test firmware
uart-test:
	$(MAKE) clean TARGET=uart_comm_test
	$(MAKE) TARGET=uart_comm_test

# Help
help:
	@echo "STM32F411CEU6 Firmware Build System"
	@echo "==================================="
	@echo ""
	@echo "Available targets:"
	@echo "  make              - Build default firmware (verify_hardware)"
	@echo "  make TARGET=xxx   - Build specific target"
	@echo "  make uart-test    - Build UART communication test firmware"
	@echo "  make flash        - Flash firmware via OpenOCD"
	@echo "  make flash-stlink - Flash firmware via st-flash"
	@echo "  make reset        - Reset target via OpenOCD"
	@echo "  make size         - Display firmware size"
	@echo "  make clean        - Clean build directory"
	@echo "  make debug        - Start OpenOCD debug server"
	@echo "  make help         - Show this help"
	@echo ""
	@echo "Available TARGET values:"
	@echo "  verify_hardware   - Hardware verification firmware (default)"
	@echo "  uart_comm_test    - UART communication test with ESP32"
	@echo ""
	@echo "Examples:"
	@echo "  make uart-test && make flash"
	@echo "  make TARGET=uart_comm_test flash"
