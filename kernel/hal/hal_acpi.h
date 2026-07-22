#ifndef VXAIR_HAL_ACPI_H
#define VXAIR_HAL_ACPI_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief ACPI System Description Table Header
 */
typedef struct {
    char signature[4];
    uint32_t length;
    uint8_t revision;
    uint8_t checksum;
    char oem_id[6];
    char oem_table_id[8];
    uint32_t oem_revision;
    uint32_t creator_id;
    uint32_t creator_revision;
} __attribute__((packed)) vxair_acpi_header_t;

/**
 * @brief Root System Description Pointer (RSDP)
 */
typedef struct {
    char signature[8];
    uint8_t checksum;
    char oem_id[6];
    uint8_t revision;
    uint32_t rsdt_address;
    uint32_t length;
    uint64_t xsdt_address;
    uint8_t extended_checksum;
    uint8_t reserved[3];
} __attribute__((packed)) vxair_acpi_rsdp_t;

/**
 * @brief Initialize the ACPI subsystem
 * @param rsdp_addr Physical address of the RSDP table
 */
void vxair_hal_acpi_init(void* rsdp_addr);

/**
 * @brief Find an ACPI table by its signature
 * @param signature The 4-character signature to search for
 * @return Pointer to the ACPI table header, or NULL if not found
 */
void* vxair_hal_acpi_find_table(const char* signature);

#ifdef __cplusplus
}
#endif

#endif // VXAIR_HAL_ACPI_H
