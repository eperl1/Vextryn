#ifndef VXAIR_HAL_PM_H
#define VXAIR_HAL_PM_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize the power management subsystem
 */
void vxair_hal_pm_init(void);

/**
 * @brief Perform a system shutdown via ACPI
 */
void vxair_hal_pm_shutdown(void);

/**
 * @brief Reboot the system using keyboard controller or ACPI
 */
void vxair_hal_pm_reboot(void);

/**
 * @brief Suspend the system to RAM (S3)
 */
void vxair_hal_pm_suspend(void);

#ifdef __cplusplus
}
#endif

#endif // VXAIR_HAL_PM_H
