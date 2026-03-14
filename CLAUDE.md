# kimi-fly Coding Standards

Coding standards for the kimi-fly drone project. All agents must follow these rules.

## Language Standards

### C (Embedded)

- **Standard**: C11
- **Style**: Kernel-style with modifications
- **Indent**: 4 spaces (no tabs)
- **Max line length**: 100 characters
- **Braces**: K&R style, opening brace on same line

```c
// Good
static int motor_init(struct motor_ctx *ctx, uint8_t id)
{
    if (!ctx || id >= MOTOR_MAX_COUNT) {
        return -EINVAL;
    }
    // ...
    return 0;
}

// Bad
static int motor_init (struct motor_ctx *ctx, uint8_t id) {
  if(!ctx)return -1;
  ...
}
```

### C++ (when needed)

- **Standard**: C++17
- **Style**: Google C++ Style Guide
- **No exceptions** in embedded code
- **No RTTI**
- Prefer `constexpr` over macros

## Naming Conventions

| Type | Pattern | Example |
|------|---------|---------|
| Functions | `module_verb_noun` | `pwm_set_duty()`, `imu_read_accel()` |
| Types | `module_noun_t` | `motor_ctx_t`, `sensor_data_t` |
| Constants | `MODULE_NAME` | `PWM_MAX_CHANNELS`, `I2C_BUS_ID` |
| Macros | `MODULE_VERB` | `ASSERT(cond)`, `ARRAY_SIZE(arr)` |
| File scope | `_prefix_name` | `_internal_buffer` |

## Embedded-Specific Rules

### Memory

- **No dynamic allocation** after initialization
- Stack usage: Document max stack depth for ISRs
- Prefer `static const` over `#define` for constants
- Use `static_assert` for compile-time checks

```c
// Good
static const uint16_t PWM_FREQ_HZ = 400;
static_assert(PWM_FREQ_HZ > 0, "PWM frequency must be positive");

// Bad
#define PWM_FREQ 400  // No type safety
```

### Interrupt Safety

- ISRs must be short and fast (< 10us typical)
- Use `volatile` for shared memory
- Use atomic operations or disable interrupts for critical sections
- Document ISR priority requirements

```c
// Good: ISR-safe flag pattern
volatile bool data_ready = false;

void ISR_TIM2(void)
{
    data_ready = true;
}

void main_loop(void)
{
    if (__atomic_load_n(&data_ready, __ATOMIC_RELAXED)) {
        __atomic_store_n(&data_ready, false, __ATOMIC_RELAXED);
        process_data();
    }
}
```

### Error Handling

- Return `int` for status: `0` = success, negative = error code
- Use `errno.h` style error codes (`-EINVAL`, `-EIO`, `-ENOMEM`)
- Check all return values
- Use `ASSERT()` for programming errors, not runtime failures

```c
// Good
int rc = i2c_write(bus, addr, data, len);
if (rc < 0) {
    LOG_ERR("I2C write failed: %d", rc);
    return rc;
}

// Bad
i2c_write(bus, addr, data, len);  // Ignored return value
```

## Hardware Abstraction Layer (HAL)

### Layer Structure

```
application/
├── services/       # High-level logic (attitude control, navigation)
├── drivers/        # Hardware drivers (sensor drivers, motor drivers)
├── hal/            # Hardware abstraction (STM32 HAL, ESP-IDF wrappers)
└── platform/       # Board-specific configuration
```

### Rules

1. **Drivers depend on HAL, not directly on registers**
2. **Use LL (Low-Level) drivers for time-critical paths**
3. **Use HAL for complex initialization**
4. **All hardware access through abstraction layer**

## Testing Requirements

- Unit tests for all driver logic (host-based with mocks)
- HIL tests for hardware integration
- >80% code coverage for critical paths (flight control, safety)
- Tests must be deterministic

## Documentation

- Document **why**, not what (code shows what)
- Document hardware dependencies and timing requirements
- Document units (degrees vs radians, m/s vs km/h)
- Document safety implications

```c
/**
 * @brief Set motor throttle
 *
 * @param id Motor ID (0-3)
 * @param throttle Throttle value [0.0, 1.0]
 * @return 0 on success, -EINVAL if out of range
 *
 * @note This function is ISR-safe. Updates take effect on next PWM cycle.
 * @warning Throttle > 0.8 requires propeller guards per safety spec.
 */
int motor_set_throttle(uint8_t id, float throttle);
```

## Prohibited Patterns

- `malloc()`/`free()` in flight code
- Busy-wait loops longer than 1ms
- Floating-point in ISRs (on F4 without FPU)
- Magic numbers without named constants
- Global variables without `static` or documentation
- Recursive functions

## Tooling

- **Formatter**: `clang-format` (config in `.clang-format`)
- **Linter**: `clang-tidy` with embedded checks
- **Static analysis**: `cppcheck` for MISRA-C compliance
- **All checks must pass in CI**

---

*When in doubt, prefer clarity over cleverness. The next agent (or you in 6 months) will thank you.*
