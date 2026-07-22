#ifndef VXAIR_HAL_TIMER_H
#define VXAIR_HAL_TIMER_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize hardware timers (HPET)
 */
void vxair_hal_timer_init(void);

/**
 * @brief Get the current system uptime in milliseconds
 * @return Uptime in milliseconds
 */
uint64_t vxair_hal_timer_get_uptime_ms(void);

/**
 * @brief Sleep for the specified number of milliseconds
 * @param ms Milliseconds to sleep
 */
void vxair_hal_timer_sleep_ms(uint64_t ms);

#ifdef __cplusplus
}
#endif

#endif // VXAIR_HAL_TIMER_H
