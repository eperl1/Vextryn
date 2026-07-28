#include "hal_acpi.h"
#include <stddef.h>

static vxair_acpi_rsdp_t* g_rsdp = NULL;
static vxair_acpi_header_t* g_rsdt = NULL;
static vxair_acpi_header_t* g_xsdt = NULL;

static bool vxair_hal_acpi_validate_checksum(const void* table, uint32_t length) {
    const uint8_t* bytes = (const uint8_t*)table;
    uint8_t sum = 0;
    for (uint32_t i = 0; i < length; i++) {
        sum += bytes[i];
    }
    return sum == 0;
}

/**
 * @brief Scan for RSDP signature in BIOS memory area
 */
void* vxair_hal_acpi_scan_rsdp(void) {
    // Scan physical memory from 0xE0000 to 0xFFFFF for the RSDP signature "RSD PTR "
    // The RSDP is always 16-byte aligned in EBDA or the BIOS ROM area.
    for (uint64_t addr = 0xE0000; addr <= 0xFFFF0; addr += 16) {
        volatile const char *sig = (volatile const char *)(uintptr_t)addr;
        if (sig[0] == 'R' && sig[1] == 'S' && sig[2] == 'D' &&
            sig[3] == ' ' && sig[4] == 'P' && sig[5] == 'T' &&
            sig[6] == 'R' && sig[7] == ' ') {
            // Validate v1 checksum (first 20 bytes of RSDP)
            uint8_t sum = 0;
            for (int i = 0; i < 20; i++) sum += ((const uint8_t *)addr)[i];
            if (sum == 0) return (void *)(uintptr_t)addr;
            // v2 RSDP is 36 bytes; validate extended checksum
            // Use the revision field (byte 15) to determine RSDP version.
            // Byte 20 contains the length field for v2 RSDP.
            uint8_t revision = ((const uint8_t *)addr)[15];
            uint32_t len = (revision >= 2) ? 36 : 20;
            sum = 0;
            for (uint32_t i = 0; i < len; i++) sum += ((const uint8_t *)addr)[i];
            if (sum == 0) return (void *)(uintptr_t)addr;
        }
    }
    return NULL;
}

void vxair_hal_acpi_init(void* rsdp_addr) {
    if (!rsdp_addr) {
        return;
    }

    g_rsdp = (vxair_acpi_rsdp_t*)rsdp_addr;
    
    // Check RSDP revision
    if (g_rsdp->revision >= 2 && g_rsdp->xsdt_address != 0) {
        g_xsdt = (vxair_acpi_header_t*)(uintptr_t)g_rsdp->xsdt_address;
        if (!vxair_hal_acpi_validate_checksum(g_xsdt, g_xsdt->length)) {
            g_xsdt = NULL;
        }
    }
    
    if (g_rsdp->rsdt_address != 0) {
        g_rsdt = (vxair_acpi_header_t*)(uintptr_t)g_rsdp->rsdt_address;
        if (!vxair_hal_acpi_validate_checksum(g_rsdt, g_rsdt->length)) {
            g_rsdt = NULL;
        }
    }
}

void* vxair_hal_acpi_find_table(const char* signature) {
    if (g_xsdt) {
        uint32_t entries = (g_xsdt->length - sizeof(vxair_acpi_header_t)) / 8;
        uint64_t* pointers = (uint64_t*)((uint8_t*)g_xsdt + sizeof(vxair_acpi_header_t));
        for (uint32_t i = 0; i < entries; i++) {
            vxair_acpi_header_t* header = (vxair_acpi_header_t*)(uintptr_t)pointers[i];
            if (header->signature[0] == signature[0] &&
                header->signature[1] == signature[1] &&
                header->signature[2] == signature[2] &&
                header->signature[3] == signature[3]) {
                if (vxair_hal_acpi_validate_checksum(header, header->length)) {
                    return header;
                }
            }
        }
    } else if (g_rsdt) {
        uint32_t entries = (g_rsdt->length - sizeof(vxair_acpi_header_t)) / 4;
        uint32_t* pointers = (uint32_t*)((uint8_t*)g_rsdt + sizeof(vxair_acpi_header_t));
        for (uint32_t i = 0; i < entries; i++) {
            vxair_acpi_header_t* header = (vxair_acpi_header_t*)(uintptr_t)pointers[i];
            if (header->signature[0] == signature[0] &&
                header->signature[1] == signature[1] &&
                header->signature[2] == signature[2] &&
                header->signature[3] == signature[3]) {
                if (vxair_hal_acpi_validate_checksum(header, header->length)) {
                    return header;
                }
            }
        }
    }
    
    return NULL;
}
