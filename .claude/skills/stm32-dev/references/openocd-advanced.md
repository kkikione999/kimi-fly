# OpenOCD 高级调试参考

---

## GDB 调试工作流

```bash
# 终端 1：启动 OpenOCD（保持运行）
openocd -f interface/stlink.cfg -f target/stm32f4x.cfg

# 终端 2：连接 GDB
arm-none-eabi-gdb build/firmware.elf

# GDB 命令序列
(gdb) target extended-remote :3333  # 连接 OpenOCD
(gdb) monitor reset halt            # 复位并停止
(gdb) load                          # 烧写固件
(gdb) break main                    # 设置断点
(gdb) continue                      # 运行到断点
(gdb) next                          # 单步（不进入函数）
(gdb) step                          # 单步（进入函数）
(gdb) info registers                # 查看寄存器
(gdb) x/10xw 0x20000000            # 查看内存（10个word，hex格式）
(gdb) print variable_name          # 打印变量
(gdb) backtrace                    # 查看调用栈（HardFault 时有用）
(gdb) monitor reset run            # 复位并继续运行
(gdb) quit
```

---

## HardFault 调试技巧

发生 HardFault 时，在 GDB 中：

```
(gdb) backtrace
(gdb) info registers
```

检查 `LR` 寄存器（如为 `0xFFFFFFF9` 表示从线程模式发生）。  
读取 CFSR 寄存器（配置失败状态寄存器）：

```
(gdb) x/1xw 0xE000ED28
```

常见 CFSR 标志位解析：
- `IBUSERR`（bit8）：指令总线错误，通常是跳到非法地址
- `DBUSERR`（bit9）：数据总线错误
- `UNDEFINSTR`（bit16）：未定义指令（如在 Cortex-M0 上执行 M4 指令）
- `UNALIGNED`（bit24）：非对齐访问

---

## OpenOCD Telnet 接口

```bash
# 连接 OpenOCD telnet（OpenOCD 运行中）
telnet localhost 4444

# 常用命令
> halt                  # 暂停 CPU
> resume                # 继续运行
> reset                 # 复位
> flash write_image erase build/firmware.bin 0x08000000
> mdw 0x20000000 16     # 读取内存（word，16个）
> mww 0x20000000 0xDEADBEEF  # 写内存
> reg                   # 显示所有寄存器
> exit
```

---

## RTT（SEGGER Real-Time Transfer）日志输出

OpenOCD 支持 RTT，可替代 UART 做低侵入性日志输出：

```bash
# OpenOCD 侧启动 RTT
openocd -f interface/stlink.cfg -f target/stm32f4x.cfg \
  -c "rtt setup 0x20000000 0x1000 \"SEGGER RTT\"" \
  -c "rtt start" \
  -c "rtt server start 9090 0"

# 主机侧连接（另一终端）
nc localhost 9090
```

固件侧需引入 `SEGGER_RTT.c` 并调用：
```c
SEGGER_RTT_printf(0, "Value: %d\r\n", value);
```

---

## udev 规则（Linux，解决 Permission Denied）

```bash
# 创建规则文件
sudo nano /etc/udev/rules.d/99-stlink.rules

# 写入以下内容（覆盖全部 ST-Link v1/v2/v3）
SUBSYSTEM=="usb", ATTRS{idVendor}=="0483", ATTRS{idProduct}=="3748", MODE="0666", GROUP="plugdev"
SUBSYSTEM=="usb", ATTRS{idVendor}=="0483", ATTRS{idProduct}=="374b", MODE="0666", GROUP="plugdev"
SUBSYSTEM=="usb", ATTRS{idVendor}=="0483", ATTRS{idProduct}=="374e", MODE="0666", GROUP="plugdev"
SUBSYSTEM=="usb", ATTRS{idVendor}=="0483", ATTRS{idProduct}=="374f", MODE="0666", GROUP="plugdev"
SUBSYSTEM=="usb", ATTRS{idVendor}=="0483", ATTRS{idProduct}=="3752", MODE="0666", GROUP="plugdev"

# 重新加载规则
sudo udevadm control --reload-rules && sudo udevadm trigger

# 将当前用户加入 plugdev 组
sudo usermod -aG plugdev $USER
# 重新登录生效
```
