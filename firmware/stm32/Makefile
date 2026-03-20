# STM32F411CEU6 Sensor Test Makefile

TARGET   = sensor_test
BUILD_DIR = build

# 工具链
PREFIX   = arm-none-eabi-
CC       = $(PREFIX)gcc
OBJCOPY  = $(PREFIX)objcopy
SIZE     = $(PREFIX)size

# 芯片参数 (STM32F411CEU6 - Cortex-M4)
CPU      = -mcpu=cortex-m4
FPU      = -mfpu=fpv4-sp-d16
FLOAT    = -mfloat-abi=hard
MCU      = $(CPU) -mthumb $(FPU) $(FLOAT)

# PlatformIO STM32Cube 框架路径
STM32CUBE_DIR = /Users/ll/.platformio/packages/framework-stm32cubef4
HAL_DIR       = $(STM32CUBE_DIR)/Drivers/STM32F4xx_HAL_Driver
CMSIS_DIR     = $(STM32CUBE_DIR)/Drivers/CMSIS
CMSIS_DEVICE  = $(CMSIS_DIR)/Device/ST/STM32F4xx

# 头文件路径
INCS     = -Isrc \
           -I$(HAL_DIR)/Inc \
           -I$(CMSIS_DIR)/Include \
           -I$(CMSIS_DEVICE)/Include

# HAL 源文件
HAL_SRCS = $(HAL_DIR)/Src/stm32f4xx_hal.c \
           $(HAL_DIR)/Src/stm32f4xx_hal_cortex.c \
           $(HAL_DIR)/Src/stm32f4xx_hal_gpio.c \
           $(HAL_DIR)/Src/stm32f4xx_hal_i2c.c \
           $(HAL_DIR)/Src/stm32f4xx_hal_spi.c \
           $(HAL_DIR)/Src/stm32f4xx_hal_uart.c \
           $(HAL_DIR)/Src/stm32f4xx_hal_rcc.c \
           $(HAL_DIR)/Src/stm32f4xx_hal_rcc_ex.c \
           $(HAL_DIR)/Src/stm32f4xx_hal_pwr.c \
           $(HAL_DIR)/Src/stm32f4xx_hal_pwr_ex.c \
           $(HAL_DIR)/Src/stm32f4xx_hal_dma.c

# 应用源文件
APP_SRCS = src/main.c

# 系统文件
SYS_SRCS = $(CMSIS_DEVICE)/Source/Templates/system_stm32f4xx.c

# 所有C源文件
C_SRCS   = $(APP_SRCS) $(HAL_SRCS) $(SYS_SRCS)

# 启动文件
ASM_SRCS = $(CMSIS_DEVICE)/Source/Templates/gcc/startup_stm32f411xe.s

# 对象文件
OBJS     = $(patsubst %.c,$(BUILD_DIR)/%.o,$(notdir $(C_SRCS))) \
           $(patsubst %.s,$(BUILD_DIR)/%.o,$(notdir $(ASM_SRCS)))

# 编译标志
CFLAGS   = $(MCU) $(INCS) -Wall -Og -g3 -gdwarf-2 \
           -ffunction-sections -fdata-sections \
           -DSTM32F411xE -DUSE_HAL_DRIVER

# 链接标志
LDFLAGS  = $(MCU) --specs=nano.specs -lc -lm -lnosys \
           -Wl,--gc-sections \
           -Wl,-Map=$(BUILD_DIR)/$(TARGET).map,-T$(BUILD_DIR)/stm32f411.ld

# VPATH for source files
VPATH = src:$(HAL_DIR)/Src:$(CMSIS_DEVICE)/Source/Templates:$(CMSIS_DEVICE)/Source/Templates/gcc

# 默认目标
all: $(BUILD_DIR)/stm32f411.ld $(BUILD_DIR)/$(TARGET).elf
	@echo "Building $(TARGET)..."
	$(SIZE) $(BUILD_DIR)/$(TARGET).elf

$(BUILD_DIR)/$(TARGET).elf: $(OBJS)
	@mkdir -p $(dir $@)
	$(CC) $(OBJS) $(LDFLAGS) -o $@
	$(OBJCOPY) -O ihex   $@ $(BUILD_DIR)/$(TARGET).hex
	$(OBJCOPY) -O binary $@ $(BUILD_DIR)/$(TARGET).bin

$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: %.s
	@mkdir -p $(dir $@)
	$(CC) $(MCU) -x assembler-with-cpp -c $< -o $@

# 烧录目标
flash: all
	st-flash write $(BUILD_DIR)/$(TARGET).bin 0x08000000

flash-hex: all
	openocd -f interface/stlink.cfg -f target/stm32f4x.cfg \
		-c "program $(BUILD_DIR)/$(TARGET).hex verify reset exit"

# 创建链接脚本
$(BUILD_DIR)/stm32f411.ld:
	@mkdir -p $(BUILD_DIR)
	@echo "Creating linker script..."
	@echo 'MEMORY {' > $@
	@echo '  FLASH (rx) : ORIGIN = 0x08000000, LENGTH = 512K' >> $@
	@echo '  RAM (rwx)  : ORIGIN = 0x20000000, LENGTH = 128K' >> $@
	@echo '}' >> $@
	@echo 'SECTIONS {' >> $@
	@echo '  .text : { *(.text*) *(.rodata*) } > FLASH' >> $@
	@echo '  .data : { *(.data*) } > RAM AT > FLASH' >> $@
	@echo '  .bss : { *(.bss*) *(COMMON) } > RAM' >> $@
	@echo '}' >> $@

# 清理
clean:
	rm -rf $(BUILD_DIR)

.PHONY: all flash flash-hex clean
