#ifndef HAL_COMMON_H
#define HAL_COMMON_H

#include <stdint.h>
#include <stdbool.h>

/* HAL Error codes */
typedef enum {
    HAL_OK = 0,
    HAL_ERROR = 1,
    HAL_BUSY = 2,
    HAL_TIMEOUT = 3
} hal_status_t;

/* Common macros */
#define HAL_NULL    0
#define HAL_SET     1
#define HAL_RESET   0

#endif /* HAL_COMMON_H */
