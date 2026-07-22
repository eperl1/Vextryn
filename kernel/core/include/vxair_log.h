#pragma once
#include "vxair_types.h"
#include <stdarg.h>

/**
 * @file vxair_log.h
 * @brief Kernel logging facilities.
 */

void vxair_log_init(void);
void vxair_log_info(const char* fmt, ...);
void vxair_log_warn(const char* fmt, ...);
void vxair_log_error(const char* fmt, ...);
