/**
 * @file uefi_bootloader.c
 * @brief UEFI bootloader for Vextryn Air OS
 */
#include <efi.h>
#include <efilib.h>
#include "vxair_boot_info.h"

#define PAGE_SIZE 4096
#define PML4_ENTRY_COUNT 512
#define PDPT_ENTRY_COUNT 512
#define PD_ENTRY_COUNT 512
#define PT_ENTRY_COUNT 512

/* Paging flags */
#define PAGE_PRESENT (1ULL << 0)
#define PAGE_WRITE   (1ULL << 1)
#define PAGE_USER    (1ULL << 2)

/* Global boot info passed to kernel */
static struct vxair_boot_info g_boot_info;

/* Global memory regions array */
#define MAX_MEM_REGIONS 512
static struct vxair_memory_region g_mem_regions[MAX_MEM_REGIONS];

/**
 * @brief Get the Graphics Output Protocol (GOP) and populate framebuffer info
 */
static EFI_STATUS vxair_init_gop(struct vxair_framebuffer* fb) {
    EFI_STATUS status;
    EFI_GUID gop_guid = EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID;
    EFI_GRAPHICS_OUTPUT_PROTOCOL* gop = NULL;

    status = uefi_call_wrapper(BS->LocateProtocol, 3, &gop_guid, NULL, (void**)&gop);
    if (EFI_ERROR(status) || !gop) {
        Print(L"Could not locate GOP\n");
        return status;
    }

    EFI_GRAPHICS_OUTPUT_MODE_INFORMATION* info;
    UINTN size_of_info, num_modes, native_mode;
    
    status = uefi_call_wrapper(gop->QueryMode, 4, gop, gop->Mode==NULL ? 0 : gop->Mode->Mode, &size_of_info, &info);
    if (status == EFI_NOT_STARTED) {
        status = uefi_call_wrapper(gop->SetMode, 2, gop, 0);
    }
    
    if (EFI_ERROR(status)) {
        Print(L"GOP QueryMode/SetMode failed\n");
        return status;
    }

    fb->address = (uint64_t)gop->Mode->FrameBufferBase;
    fb->width = gop->Mode->Info->HorizontalResolution;
    fb->height = gop->Mode->Info->VerticalResolution;
    fb->pitch = gop->Mode->Info->PixelsPerScanLine * 4;
    fb->bpp = 32;

    return EFI_SUCCESS;
}

/**
 * @brief Find the ACPI RSDP configuration table
 */
static void* vxair_find_rsdp(EFI_SYSTEM_TABLE* SystemTable) {
    EFI_GUID acpi_20_table_guid = ACPI_20_TABLE_GUID;
    EFI_GUID acpi_table_guid = ACPI_TABLE_GUID;

    for (UINTN i = 0; i < SystemTable->NumberOfTableEntries; i++) {
        if (CompareGuid(&acpi_20_table_guid, &SystemTable->ConfigurationTable[i].VendorGuid) == 0) {
            return SystemTable->ConfigurationTable[i].VendorTable;
        }
    }
    
    for (UINTN i = 0; i < SystemTable->NumberOfTableEntries; i++) {
        if (CompareGuid(&acpi_table_guid, &SystemTable->ConfigurationTable[i].VendorGuid) == 0) {
            return SystemTable->ConfigurationTable[i].VendorTable;
        }
    }
    return NULL;
}

/**
 * @brief Retrieve memory map from UEFI
 */
static EFI_STATUS vxair_get_memory_map(UINTN* map_key) {
    EFI_STATUS status;
    EFI_MEMORY_DESCRIPTOR* mem_map = NULL;
    UINTN map_size = 0, descriptor_size = 0;
    UINT32 descriptor_version = 0;

    status = uefi_call_wrapper(BS->GetMemoryMap, 5, &map_size, mem_map, map_key, &descriptor_size, &descriptor_version);
    if (status != EFI_BUFFER_TOO_SMALL) {
        return status;
    }

    map_size += 8 * descriptor_size;
    status = uefi_call_wrapper(BS->AllocatePool, 3, EfiLoaderData, map_size, (void**)&mem_map);
    if (EFI_ERROR(status)) {
        return status;
    }

    status = uefi_call_wrapper(BS->GetMemoryMap, 5, &map_size, mem_map, map_key, &descriptor_size, &descriptor_version);
    if (EFI_ERROR(status)) {
        uefi_call_wrapper(BS->FreePool, 1, mem_map);
        return status;
    }

    UINTN num_entries = map_size / descriptor_size;
    g_boot_info.mem_map.regions = g_mem_regions;
    g_boot_info.mem_map.num_regions = 0;

    for (UINTN i = 0; i < num_entries && i < MAX_MEM_REGIONS; i++) {
        EFI_MEMORY_DESCRIPTOR* desc = (EFI_MEMORY_DESCRIPTOR*)((uint8_t*)mem_map + (i * descriptor_size));
        struct vxair_memory_region* region = &g_mem_regions[g_boot_info.mem_map.num_regions++];
        region->base_address = desc->PhysicalStart;
        region->length = desc->NumberOfPages * PAGE_SIZE;

        switch (desc->Type) {
            case EfiLoaderCode:
            case EfiLoaderData:
            case EfiBootServicesCode:
            case EfiBootServicesData:
            case EfiConventionalMemory:
                region->type = VXAIR_MEMMAP_TYPE_AVAILABLE;
                break;
            case EfiACPIReclaimMemory:
                region->type = VXAIR_MEMMAP_TYPE_ACPI_RECLAIM;
                break;
            case EfiACPIMemoryNVS:
                region->type = VXAIR_MEMMAP_TYPE_ACPI_NVS;
                break;
            case EfiUnusableMemory:
                region->type = VXAIR_MEMMAP_TYPE_BAD_MEMORY;
                break;
            default:
                region->type = VXAIR_MEMMAP_TYPE_RESERVED;
                break;
        }
    }

    return EFI_SUCCESS;
}

/**
 * @brief Setup 4-level paging identity map and map kernel
 */
static void vxair_setup_paging(uint64_t* pml4_out) {
    uint64_t* pml4 = NULL;
    uint64_t* pdpt = NULL;
    uint64_t* pd = NULL;
    
    uefi_call_wrapper(BS->AllocatePages, 4, AllocateAnyPages, EfiLoaderData, 1, (EFI_PHYSICAL_ADDRESS*)&pml4);
    uefi_call_wrapper(BS->AllocatePages, 4, AllocateAnyPages, EfiLoaderData, 1, (EFI_PHYSICAL_ADDRESS*)&pdpt);
    uefi_call_wrapper(BS->AllocatePages, 4, AllocateAnyPages, EfiLoaderData, 1, (EFI_PHYSICAL_ADDRESS*)&pd);
    
    BS->SetMem(pml4, PAGE_SIZE, 0);
    BS->SetMem(pdpt, PAGE_SIZE, 0);
    BS->SetMem(pd, PAGE_SIZE, 0);

    /* Identity map first 1GB using 2MB huge pages */
    pml4[0] = (uint64_t)pdpt | PAGE_PRESENT | PAGE_WRITE;
    pdpt[0] = (uint64_t)pd | PAGE_PRESENT | PAGE_WRITE;
    
    for (uint64_t i = 0; i < PD_ENTRY_COUNT; i++) {
        /* 2MB pages: PS flag is bit 7 */
        pd[i] = (i * 0x200000) | PAGE_PRESENT | PAGE_WRITE | (1ULL << 7);
    }
    
    *pml4_out = (uint64_t)pml4;
}

/**
 * @brief GDT setup for 64-bit kernel
 */
struct gdt_entry {
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t base_mid;
    uint8_t access;
    uint8_t granularity;
    uint8_t base_high;
} __attribute__((packed));

struct gdt_ptr {
    uint16_t limit;
    uint64_t base;
} __attribute__((packed));

static struct gdt_entry g_gdt[3];
static struct gdt_ptr g_gdt_ptr;

static void vxair_setup_gdt(void) {
    /* Null descriptor */
    g_gdt[0] = (struct gdt_entry){0, 0, 0, 0, 0, 0};
    /* 64-bit code segment */
    g_gdt[1] = (struct gdt_entry){0, 0, 0, 0x9A, 0x20, 0};
    /* 64-bit data segment */
    g_gdt[2] = (struct gdt_entry){0, 0, 0, 0x92, 0, 0};

    g_gdt_ptr.limit = sizeof(g_gdt) - 1;
    g_gdt_ptr.base = (uint64_t)&g_gdt;

    /* Inline assembly to load GDT */
    __asm__ volatile("lgdt %0" : : "m"(g_gdt_ptr));
}

/**
 * @brief Entry point for UEFI application
 */
EFI_STATUS EFIAPI efi_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable) {
    InitializeLib(ImageHandle, SystemTable);
    
    Print(L"Vextryn Air OS UEFI Bootloader Starting...\n");
    
    /* Initialize Boot Info */
    BS->SetMem(&g_boot_info, sizeof(g_boot_info), 0);
    
    /* Get GOP (Graphics Output Protocol) for framebuffer info */
    EFI_STATUS status = vxair_init_gop(&g_boot_info.framebuffer);
    if (EFI_ERROR(status)) {
        Print(L"Failed to initialize GOP\n");
    }
    
    /* Find ACPI RSDP via UEFI configuration tables */
    void* rsdp = vxair_find_rsdp(SystemTable);
    if (rsdp) {
        g_boot_info.rsdp_address = (uint64_t)rsdp;
        Print(L"Found ACPI RSDP at %lx\n", g_boot_info.rsdp_address);
    } else {
        Print(L"ACPI RSDP not found\n");
    }
    
    /* Load kernel ELF image from file system into memory */
    /* Note: simplified loading, in reality we'd parse ELF */
    EFI_FILE_PROTOCOL* root = NULL;
    EFI_LOADED_IMAGE_PROTOCOL* loaded_image = NULL;
    
    uefi_call_wrapper(BS->HandleProtocol, 3, ImageHandle, &LoadedImageProtocol, (void**)&loaded_image);
    uefi_call_wrapper(BS->HandleProtocol, 3, loaded_image->DeviceHandle, &FileSystemProtocol, (void**)&root);
    /* In a real implementation we'd open the kernel.elf and read it into memory */
    g_boot_info.kernel_physical_base = 0x100000; 
    g_boot_info.kernel_virtual_base = 0xFFFFFFFF80000000;
    
    /* Set up page tables for kernel higher-half mapping */
    uint64_t pml4_addr = 0;
    vxair_setup_paging(&pml4_addr);
    __asm__ volatile("mov %0, %%cr3" : : "r"(pml4_addr));
    
    /* Setup GDT */
    vxair_setup_gdt();
    
    /* Get memory map from UEFI (GetMemoryMap) right before exit */
    UINTN map_key = 0;
    status = vxair_get_memory_map(&map_key);
    if (EFI_ERROR(status)) {
        Print(L"Failed to get memory map\n");
        return status;
    }
    
    /* Exit boot services (ExitBootServices) */
    status = uefi_call_wrapper(BS->ExitBootServices, 2, ImageHandle, map_key);
    if (EFI_ERROR(status)) {
        /* Retry getting memory map and exiting */
        vxair_get_memory_map(&map_key);
        status = uefi_call_wrapper(BS->ExitBootServices, 2, ImageHandle, map_key);
        if (EFI_ERROR(status)) {
            Print(L"Failed to exit boot services\n");
            return status;
        }
    }
    
    /* Disable interrupts */
    __asm__ volatile("cli");
    
    /* Jump to kernel long mode entry point, passing vxair_boot_info */
    /* void (*kernel_entry)(struct vxair_boot_info*) = (void*)g_boot_info.kernel_physical_base; */
    /* kernel_entry(&g_boot_info); */
    
    extern void vxair_long_mode_start(void);
    
    /* Move boot info to rdi (System V AMD64 ABI 1st param) */
    __asm__ volatile(
        "mov %0, %%rdi\n"
        "jmp vxair_long_mode_start\n"
        : : "r"(&g_boot_info) : "rdi"
    );
    
    /* Halt just in case */
    while (1) {
        __asm__ volatile("hlt");
    }

    return EFI_SUCCESS;
}
