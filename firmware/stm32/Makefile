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

# 头文件路径
INCS     = -Ihal \
           -Idrivers \
           -Imain \
           -Ialgorithm \
           -Icomm

# 源文件
C_SRCS   = main/sensor_test.c \
           hal/gpio.c \
           hal/uart.c \
           hal/i2c.c \
           hal/spi.c \
           hal/pwm.c \
           drivers/icm42688.c \
           drivers/lps22hb.c \
           drivers/qmc5883p.c

# 启动文件和系统文件
# 使用CMSIS标准启动文件
ASM_SRCS =

OBJS     = $(patsubst %.c,$(BUILD_DIR)/%.o,$(C_SRCS))

# 编译标志
CFLAGS   = $(MCU) $(INCS) -Wall -Og -g3 -gdwarf-2 \
           -ffunction-sections -fdata-sections \
           -DSTM32F411xE -DUSE_HAL_DRIVER

# 链接标志 (使用默认链接脚本)
LDFLAGS  = $(MCU) --specs=nano.specs -lc -lm -lnosys \
           -Wl,--gc-sections \
           -Wl,-Map=$(BUILD_DIR)/$(TARGET).map,-Tstm32f411.ld

# 默认目标
all: $(BUILD_DIR)/$(TARGET).elf
	@echo "Building $(TARGET)..."
	$(SIZE) $<

$(BUILD_DIR)/$(TARGET).elf: $(OBJS)
	@mkdir -p $(dir $@)
	$(CC) $(OBJS) $(LDFLAGS) -o $@
	$(OBJCOPY) -O ihex   $@ $(BUILD_DIR)/$(TARGET).hex
	$(OBJCOPY) -O binary $@ $(BUILD_DIR)/$(TARGET).bin

$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

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
