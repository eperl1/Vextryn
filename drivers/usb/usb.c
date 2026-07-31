#include "usb.h"
#include "../../kernel/core/include/vxair_log.h" // Assuming printk or similar is mapped here
#include "../../kernel/hal/hal_timer.h"
#include "rtl8852_dle.h"
#include <string.h>

extern void vxair_xhci_submit_control_transfer(uint8_t slot_id, vxair_usb_setup_t* setup, void* data, int dir_in);
extern int vxair_xhci_configure_endpoint(uint8_t slot_id, uint8_t ep_addr, uint16_t max_packet, uint8_t type);
extern int vxair_xhci_queue_bulk_trb(uint8_t slot_id, uint8_t ep_addr, void* data, uint32_t len);

uint32_t vxair_mac_read32(uint8_t slot_id, uint32_t addr) {
    vxair_usb_setup_t req;
    req.bmRequestType = 0xC0; // Vendor Request, IN (Device to Host)
    req.bRequest = 0x05;      // Realtek USB Read Request
    req.wValue = addr & 0xFFFF;
    req.wIndex = (addr >> 16) & 0xFFFF;
    req.wLength = 4;
    
    // Using a safe static physical address for this early init phase
    uint32_t phys_addr = 0x01300000;
    uint8_t* vendor_buf = (uint8_t*)(0xFFFFFFFF80000000ull + phys_addr);
    for (int i=0; i<4; i++) vendor_buf[i] = 0;
    
    __asm__ volatile("clflush (%0)" :: "r"(vendor_buf) : "memory");
    vxair_xhci_submit_control_transfer(slot_id, &req, (void*)(uintptr_t)phys_addr, 1);
    __asm__ volatile("clflush (%0)" :: "r"(vendor_buf) : "memory");
    
    return ((uint32_t)vendor_buf[3] << 24) |
           ((uint32_t)vendor_buf[2] << 16) |
           ((uint32_t)vendor_buf[1] << 8)  |
           ((uint32_t)vendor_buf[0]);
}

void vxair_mac_write32(int slot_id, uint16_t addr, uint32_t val) {
    vxair_usb_setup_t req;
    req.bmRequestType = 0x40; // Vendor Request, OUT (Host to Device)
    req.bRequest = 0x05;      // Realtek USB Write Request
    req.wValue = addr & 0xFFFF;
    req.wIndex = (addr >> 16) & 0xFFFF;
    req.wLength = 4;
    
    uint32_t phys_addr = 0x01300000;
    uint8_t* vendor_buf = (uint8_t*)(0xFFFFFFFF80000000ull + phys_addr);
    vendor_buf[0] = (val & 0xFF);
    vendor_buf[1] = ((val >> 8) & 0xFF);
    vendor_buf[2] = ((val >> 16) & 0xFF);
    vendor_buf[3] = ((val >> 24) & 0xFF);
    
    __asm__ volatile("clflush (%0)" :: "r"(vendor_buf) : "memory");
    vxair_xhci_submit_control_transfer(slot_id, &req, (void*)(uintptr_t)phys_addr, 0);
}

int vxair_mac_poll32(uint8_t slot_id, uint32_t addr, uint32_t mask, uint32_t target, uint32_t timeout_us, uint32_t delay_us) {
    uint32_t elapsed = 0;
    while (elapsed < timeout_us) {
        uint32_t val = vxair_mac_read32(slot_id, addr);
        if ((val & mask) == target) {
            return 0; // Success
        }
        
        // Calibrated HPET microsecond delay
        vxair_hal_timer_sleep_us(delay_us);
        elapsed += delay_us;
    }
    return -1; // Timeout
}

void vxair_usb_core_init(void) {
    vxair_log_info("[USB Core] Initialized.\n");
}

int vxair_usb_get_descriptor(uint8_t slot_id, uint8_t desc_type, uint8_t desc_index, void* buffer, uint32_t data_phys, uint16_t length) {
    vxair_usb_setup_t setup;
    setup.bmRequestType = 0x80; // Device to Host, Standard, Device
    setup.bRequest = USB_REQ_GET_DESCRIPTOR;
    setup.wValue = (desc_type << 8) | desc_index;
    setup.wIndex = 0;
    setup.wLength = length;
    
    // Call into xHCI to process the control transfer (blocking for now)
    vxair_xhci_submit_control_transfer(slot_id, &setup, (void*)(uintptr_t)data_phys, 1);
    
    return 0; // Assume success for this test pass
}

void vxair_usb_set_configuration(uint8_t slot_id, uint8_t config_val) {
    vxair_usb_setup_t setup;
    setup.bmRequestType = 0x00; // Host to Device, Standard, Device
    setup.bRequest = 9; // SET_CONFIGURATION
    setup.wValue = config_val;
    setup.wIndex = 0;
    setup.wLength = 0;
    
    vxair_xhci_submit_control_transfer(slot_id, &setup, NULL, 0); // No data stage, dir=out
}

extern void* vxair_hal_dma_alloc(uint32_t size, uint32_t* phys);

void vxair_usb_handle_device_connect(uint8_t port_id, uint8_t slot_id, uint32_t speed) {
    vxair_log_info("[USB Core] Device connected on port %u, assigned slot %u, speed %u\n", port_id, slot_id, speed);
    
    // Read Device Descriptor
    uint32_t phys;
    vxair_usb_device_desc_t* desc = vxair_hal_dma_alloc(sizeof(vxair_usb_device_desc_t), &phys);
    memset(desc, 0, sizeof(*desc));
    
    vxair_log_info("[USB Core] Requesting Device Descriptor (18 bytes)...\n");
    vxair_usb_get_descriptor(slot_id, USB_DESC_DEVICE, 0, desc, phys, sizeof(*desc));
    
    vxair_log_info("=========================================\n");
    vxair_log_info("[USB DEVICE ENUMERATED]\n");
    vxair_log_info("  Port: %u\n", port_id);
    vxair_log_info("  Speed: %u\n", speed);
    vxair_log_info("  VID:PID: %04X:%04X\n", desc->idVendor, desc->idProduct);
    vxair_log_info("  Class: %02X, SubClass: %02X, Protocol: %02X\n", desc->bDeviceClass, desc->bDeviceSubClass, desc->bDeviceProtocol);
    vxair_log_info("  MaxPacketSize0: %u\n", desc->bMaxPacketSize0);
    
    vxair_log_info("  Raw Bytes: ");
    uint8_t* raw = (uint8_t*)desc;
    for (int i = 0; i < 18; i++) {
        vxair_log_info("%02X ", raw[i]);
    }
    vxair_log_info("\n");
    vxair_log_info("=========================================\n");
    
    // Read Configuration Descriptor (request a large enough chunk to avoid short/babble issues on real devices)
    uint32_t conf_phys;
    uint8_t* conf_desc = vxair_hal_dma_alloc(4096, &conf_phys); // Allocate a large buffer for the full config
    memset(conf_desc, 0, 4096);
    
    vxair_log_info("[USB Core] Requesting Full Configuration Descriptor...\n");
    vxair_usb_get_descriptor(slot_id, 2, 0, conf_desc, conf_phys, 256); // Type 2 is Configuration
    
    uint16_t total_len = conf_desc[2] | (conf_desc[3] << 8);
    vxair_log_info("[USB Core] Configuration Total Length: %u bytes\n", total_len);
    
    if (total_len > 0 && total_len <= 4096) {
        
        vxair_log_info("=========================================\n");
        
        // 5. Send SET_CONFIGURATION request
        uint8_t config_val = conf_desc[5]; // bConfigurationValue
        vxair_log_info("[USB Core] Activating Device with SET_CONFIGURATION (Val: %u)...\n", config_val);
        vxair_usb_set_configuration(slot_id, config_val);
        vxair_log_info("[USB Core] Device is now in CONFIGURED state.\n");
        
        // 6. Configure Bulk Endpoints for D-Link DWA-X1850 B1 (PID 332c)
        if (desc->idVendor == 0x2001 && desc->idProduct == 0x332c) {
            vxair_log_info("[USB Core] Initializing D-Link Endpoints 0x84 (IN), 0x05 (OUT), 0x07 (OUT)...\n");
            // EP Type: 6 (Bulk IN)
            int ret1 = vxair_xhci_configure_endpoint(slot_id, 0x84, 512, 6);
            // EP Type: 2 (Bulk OUT)
            int ret2 = vxair_xhci_configure_endpoint(slot_id, 0x05, 512, 2);
            int ret3 = vxair_xhci_configure_endpoint(slot_id, 0x07, 512, 2);
            
            if (ret1 == 0 && ret2 == 0 && ret3 == 0) {
                vxair_log_info("[USB Core] D-Link endpoints configured successfully.\n");
            } else {
                vxair_log_info("[USB Core] Failed to configure D-Link endpoints.\n");
            }
        }
        
        vxair_log_info("[USB CONFIGURATION PARSED]\n");
        vxair_log_info("  Raw Bytes:\n");
        for (int i = 0; i < total_len; i++) {
            vxair_log_info("0x%x ", conf_desc[i]);
            if ((i + 1) % 16 == 0) vxair_log_info("\n");
        }
        vxair_log_info("\n");
        
        // --- VALIDATE MAC ACCESS LAYER ---
        vxair_log_info("[RTL8852] Testing MAC Access Layer Abstraction...\n");
        
        // 1. Validation Read (using previously proven register: R_AX_SYS_CFG1 0x00F0)
        uint32_t sys_cfg1 = vxair_mac_read32(slot_id, 0x00F0);
        vxair_log_info("[RTL8852] Validation Read: 0x00F0 = 0x%08X\n", sys_cfg1);
        
        // 2. Validation Write+Readback (using previously proven register: R_AX_HCI_FUNC_EN 0x8380)
        uint32_t hci_func_en = vxair_mac_read32(slot_id, 0x8380);
        hci_func_en |= 0x00000003; // Enable TXDMA/RXDMA
        vxair_mac_write32(slot_id, 0x8380, hci_func_en);
        uint32_t hci_func_en_rb = vxair_mac_read32(slot_id, 0x8380);
        vxair_log_info("[RTL8852] Validation Write+Readback: 0x8380 = 0x%08X (Expected: 0x%08X)\n", hci_func_en_rb, hci_func_en);
        
        // 3. Validation Poll (using a safe already-known condition)
        // Since we know 0x00F0 currently holds `sys_cfg1`, we can poll for that exact value.
        vxair_log_info("[RTL8852] Validation Poll: Polling 0x00F0 for 0x%08X...\n", sys_cfg1);
        int poll_res = vxair_mac_poll32(slot_id, 0x00F0, 0xFFFFFFFF, sys_cfg1, 2000, 1);
        if (poll_res == 0) {
            vxair_log_info("[RTL8852] Validation Poll: SUCCESS.\n");
        } else {
            vxair_log_info("[RTL8852] Validation Poll: FAILED (TIMEOUT).\n");
        }
        // 4. DLE Data Execution (USB2 High-Speed Profile)
        vxair_log_info("[RTL8852] --- EXECUTING DLE HARDWARE INIT ---\n");
        uint32_t val32 = 0;
        
        // 4a. Program WDE_PKTBUF_CFG
        val32 = vxair_mac_read32(slot_id, 0x8C08); // R_AX_WDE_PKTBUF_CFG
        val32 &= ~(0x3); // clear WDE_PAGE_SEL
        val32 |= dle_wde_size25.pge_size; // SET WDE_PAGE_SEL
        val32 &= ~(0x3F << 8); // clear WDE_START_BOUND
        val32 |= (0 << 8); // bound = 0
        val32 &= ~(0x1FFF << 16); // clear WDE_FREE_PAGE_NUM
        val32 |= ((uint32_t)dle_wde_size25.lnk_pge_num << 16);
        vxair_mac_write32(slot_id, 0x8C08, val32);

        // 4b. Program PLE_PKTBUF_CFG
        val32 = vxair_mac_read32(slot_id, 0x9008); // R_AX_PLE_PKTBUF_CFG
        val32 &= ~(0x3); // clear PLE_PAGE_SEL
        val32 |= dle_ple_size27.pge_size; // SET PLE_PAGE_SEL
        uint32_t ple_bound = (dle_wde_size25.lnk_pge_num + dle_wde_size25.unlnk_pge_num) * 64 / 8192;
        val32 &= ~(0x3F << 8); // clear PLE_START_BOUND
        val32 |= ((uint32_t)ple_bound << 8);
        val32 &= ~(0x1FFF << 16); // clear PLE_FREE_PAGE_NUM
        val32 |= ((uint32_t)dle_ple_size27.lnk_pge_num << 16);
        vxair_mac_write32(slot_id, 0x9008, val32);

        #define SET_QUOTA(min, max) ((((min) & 0xFFF)) | (((max) & 0xFFF) << 16))

        // 4c. Program WDE Quotas (QTA0, QTA1, QTA3, QTA4)
        vxair_mac_write32(slot_id, 0x8C40, SET_QUOTA(dle_wde_qt25.hif, dle_wde_qt25.hif));
        vxair_mac_write32(slot_id, 0x8C44, SET_QUOTA(dle_wde_qt25.wcpu, dle_wde_qt25.wcpu));
        vxair_mac_write32(slot_id, 0x8C4C, SET_QUOTA(dle_wde_qt25.pkt_in, dle_wde_qt25.pkt_in));
        vxair_mac_write32(slot_id, 0x8C50, SET_QUOTA(dle_wde_qt25.cpu_io, dle_wde_qt25.cpu_io));

        // 4d. Program PLE Quotas (QTA0 to QTA10)
        vxair_mac_write32(slot_id, 0x9040, SET_QUOTA(dle_ple_qt61.cma0_tx, dle_ple_qt62.cma0_tx));
        vxair_mac_write32(slot_id, 0x9044, SET_QUOTA(dle_ple_qt61.cma1_tx, dle_ple_qt62.cma1_tx));
        vxair_mac_write32(slot_id, 0x9048, SET_QUOTA(dle_ple_qt61.c2h, dle_ple_qt62.c2h));
        vxair_mac_write32(slot_id, 0x904C, SET_QUOTA(dle_ple_qt61.h2c, dle_ple_qt62.h2c));
        vxair_mac_write32(slot_id, 0x9050, SET_QUOTA(dle_ple_qt61.wcpu, dle_ple_qt62.wcpu));
        vxair_mac_write32(slot_id, 0x9054, SET_QUOTA(dle_ple_qt61.mpdu_proc, dle_ple_qt62.mpdu_proc));
        vxair_mac_write32(slot_id, 0x9058, SET_QUOTA(dle_ple_qt61.cma0_dma, dle_ple_qt62.cma0_dma));
        vxair_mac_write32(slot_id, 0x905C, SET_QUOTA(dle_ple_qt61.cma1_dma, dle_ple_qt62.cma1_dma));
        vxair_mac_write32(slot_id, 0x9060, SET_QUOTA(dle_ple_qt61.bb_rpt, dle_ple_qt62.bb_rpt));
        vxair_mac_write32(slot_id, 0x9064, SET_QUOTA(dle_ple_qt61.wd_rel, dle_ple_qt62.wd_rel));
        vxair_mac_write32(slot_id, 0x9068, SET_QUOTA(dle_ple_qt61.cpu_io, dle_ple_qt62.cpu_io));

        // 4e. Poll for Initialization Ready
        vxair_log_info("[RTL8852] Polling WDE_INI_STATUS (0x8D00) for WDE_MGN_INI_RDY (0x3)...\n");
        int wde_ready = vxair_mac_poll32(slot_id, 0x8D00, 0x3, 0x3, 50000, 10);
        if (wde_ready == 0) {
            vxair_log_info("[RTL8852] WDE_MGN_INI_RDY Reached!\n");
        } else {
            vxair_log_info("[RTL8852] ERROR: WDE_MGN_INI_RDY Timeout. Status=0x%08X\n", vxair_mac_read32(slot_id, 0x8D00));
        }

        vxair_log_info("[RTL8852] Polling PLE_INI_STATUS (0x9100) for PLE_MGN_INI_RDY (0x3)...\n");
        int ple_ready = vxair_mac_poll32(slot_id, 0x9100, 0x3, 0x3, 50000, 10);
        if (ple_ready == 0) {
            vxair_log_info("[RTL8852] PLE_MGN_INI_RDY Reached!\n");
        } else {
            vxair_log_info("[RTL8852] ERROR: PLE_MGN_INI_RDY Timeout. Status=0x%08X\n", vxair_mac_read32(slot_id, 0x9100));
        }

        // --- POST-DLE PRE-FW BOUNDARY INITIALIZATION ---
        vxair_log_info("[RTL8852] --- EXECUTING POST-DLE PRE-FW BOUNDARY ---\n");
        
        // 1. usb_pre_init_8852a Equivalent
        val32 = vxair_mac_read32(slot_id, 0x1078); // R_AX_USB_HOST_REQUEST_2
        val32 |= (1 << 4); // B_AX_R_USBIO_MODE
        vxair_mac_write32(slot_id, 0x1078, val32);

        // 1.5. usb_pre_init_8852a Endpoint Mapping and Bulk Size
        {
            uint32_t v32;
            v32 = vxair_mac_read32(slot_id, 0x8908); // R_AX_RXDMA_SETTING
            v32 = (v32 & ~0xFF) | 0x1;               // USB2_BULKSIZE
            vxair_mac_write32(slot_id, 0x8908, v32);

            // Map endpoints 5, 6, 7, 9, 10, 11, 12
            uint8_t eps[] = {5, 6, 7, 9, 10, 11, 12};
            for (int i = 0; i < 7; i++) {
                uint8_t ep = eps[i];
                
                v32 = vxair_mac_read32(slot_id, 0x1060); // R_AX_USB_ENDPOINT_0
                uint8_t v8 = v32 & 0xFF;
                v8 = (v8 & ~0xF) | (ep & 0xF);       // B_AX_EP_IDX
                v32 = (v32 & ~0xFF) | v8;
                vxair_mac_write32(slot_id, 0x1060, v32);
                
                v32 = vxair_mac_read32(slot_id, 0x1068); // R_AX_USB_ENDPOINT_2
                v32 = (v32 & ~(0xFF << 8)) | (1 << 8); // Write 1 to byte 1 (0x1069, NUMP)
                vxair_mac_write32(slot_id, 0x1068, v32);
            }
        }

        val32 = vxair_mac_read32(slot_id, 0x1174); // R_AX_USB_WLAN0_1
        val32 &= ~((1 << 8) | (1 << 9)); // Clear B_AX_USBTX_RST | B_AX_USBRX_RST
        vxair_mac_write32(slot_id, 0x1174, val32);

        val32 = vxair_mac_read32(slot_id, 0x8380); // R_AX_HCI_FUNC_EN
        val32 &= ~((1 << 0) | (1 << 1)); // Clear B_AX_HCI_TXDMA_EN | B_AX_HCI_RXDMA_EN
        vxair_mac_write32(slot_id, 0x8380, val32);
        val32 |= ((1 << 0) | (1 << 1)); // Set them back
        vxair_mac_write32(slot_id, 0x8380, val32);

        // 2. WCPU Reset and Enable (mac_disable_cpu + mac_enable_cpu)
        
        // --- mac_disable_cpu ---
        val32 = vxair_mac_read32(slot_id, 0x0088); // R_AX_PLATFORM_ENABLE
        vxair_mac_write32(slot_id, 0x0088, val32 & ~(1 << 1)); // Clear B_AX_WCPU_EN

        val32 = vxair_mac_read32(slot_id, 0x01E0); // R_AX_WCPU_FW_CTRL
        vxair_mac_write32(slot_id, 0x01E0, val32 & ~((1 << 0) | (1 << 1) | (1 << 2))); // Clear FWDL_EN, H2C_PATH_RDY, FWDL_PATH_RDY

        val32 = vxair_mac_read32(slot_id, 0x0008); // R_AX_SYS_CLK_CTRL
        vxair_mac_write32(slot_id, 0x0008, val32 & ~(1 << 14)); // Clear B_AX_CPU_CLK_EN

        val32 = vxair_mac_read32(slot_id, 0x0088); // R_AX_PLATFORM_ENABLE
        vxair_mac_write32(slot_id, 0x0088, val32 & ~(1 << 0)); // Clear B_AX_PLATFORM_EN
        vxair_mac_write32(slot_id, 0x0088, val32 | (1 << 0));  // Set B_AX_PLATFORM_EN

        // --- mac_enable_cpu (dlfw = 1) ---
        vxair_mac_write32(slot_id, 0x01E8, 0); // R_AX_LDM (0x01E8)
        vxair_mac_write32(slot_id, 0x0160, 0); // R_AX_HALT_H2C_CTRL (0x0160)
        vxair_mac_write32(slot_id, 0x0164, 0); // R_AX_HALT_C2H_CTRL (0x0164)

        val32 = vxair_mac_read32(slot_id, 0x0008); // R_AX_SYS_CLK_CTRL
        vxair_mac_write32(slot_id, 0x0008, val32 | (1 << 14)); // Set B_AX_CPU_CLK_EN

        val32 = vxair_mac_read32(slot_id, 0x01E0); // R_AX_WCPU_FW_CTRL
        val32 &= ~((1 << 0) | (1 << 1) | (1 << 2));
        val32 &= ~(0x7 << 5); // Clear B_AX_WCPU_FWDL_STS (bits 5-7) - FWDL_INITIAL_STATE is 0
        val32 |= (1 << 0); // B_AX_WCPU_FWDL_EN
        vxair_mac_write32(slot_id, 0x01E0, val32);

        uint32_t boot_reason_32 = vxair_mac_read32(slot_id, 0x01E4); // R_AX_RPWM (0x01E4) and R_AX_BOOT_REASON (0x01E6)
        boot_reason_32 &= ~(0x7 << 16); // Clear B_AX_BOOT_REASON
        // reason = AX_BOOT_REASON_PWR_ON (0)
        vxair_mac_write32(slot_id, 0x01E4, boot_reason_32);

        val32 = vxair_mac_read32(slot_id, 0x0088); // R_AX_PLATFORM_ENABLE
        vxair_mac_write32(slot_id, 0x0088, val32 | (1 << 1)); // Set B_AX_WCPU_EN

        vxair_log_info("  Total Length: %u\n", total_len);
        vxair_log_info("  Num Interfaces: %u\n", conf_desc[4]);
        
        uint32_t offset = 9; // Skip Config Header
        while (offset < total_len) {
            uint8_t len = conf_desc[offset];
            uint8_t type = conf_desc[offset + 1];
            
            if (len == 0) break; // corrupted descriptor
            
            if (type == 4) { // Interface Descriptor
                vxair_log_info("  [Interface] Number: %u, AltSetting: %u, Endpoints: %u, Class: %02X, SubClass: %02X, Protocol: %02X\n",
                               conf_desc[offset + 2], conf_desc[offset + 3], conf_desc[offset + 4],
                               conf_desc[offset + 5], conf_desc[offset + 6], conf_desc[offset + 7]);
            } else if (type == 5) { // Endpoint Descriptor
                uint8_t ep_addr = conf_desc[offset + 2];
                uint8_t ep_attr = conf_desc[offset + 3];
                uint16_t max_packet = conf_desc[offset + 4] | (conf_desc[offset + 5] << 8);
                const char* dir = (ep_addr & 0x80) ? "IN" : "OUT";
                
                const char* type_str = "Control";
                if ((ep_attr & 3) == 1) type_str = "Isochronous";
                else if ((ep_attr & 3) == 2) type_str = "Bulk";
                else if ((ep_attr & 3) == 3) type_str = "Interrupt";
                
                vxair_log_info("    [Endpoint] Address: 0x%02X (%s), Type: %s, MaxPacket: %u, Interval: %u\n",
                               ep_addr, dir, type_str, max_packet, conf_desc[offset + 6]);
            }
            offset += len;
        }

        // --- PRE-FIRMWARE USB/DMA CONFIGURATION ---
        vxair_log_info("[RTL8852] --- CONFIGURING USB DMA PATH ---\n");

        // 1. Set B_AX_R_USBIO_MODE (BIT 4) in R_AX_USB_HOST_REQUEST_2 (0x1078)
        val32 = vxair_mac_read32(slot_id, 0x1078);
        vxair_mac_write32(slot_id, 0x1078, val32 | (1 << 4));

        // 2. Clear B_AX_USBRX_RST (BIT 9) and B_AX_USBTX_RST (BIT 8) in R_AX_USB_WLAN0_1 (0x1174)
        val32 = vxair_mac_read32(slot_id, 0x1174);
        vxair_mac_write32(slot_id, 0x1174, val32 & ~((1 << 9) | (1 << 8)));

        // 3. Enable B_AX_HCI_TXDMA_EN (BIT 0) and B_AX_HCI_RXDMA_EN (BIT 1) in R_AX_HCI_FUNC_EN (0x8380)
        val32 = vxair_mac_read32(slot_id, 0x8380);
        val32 &= ~3;
        vxair_mac_write32(slot_id, 0x8380, val32);
        val32 |= 3;
        vxair_mac_write32(slot_id, 0x8380, val32);

        // --- FIRMWARE HEADER DOWNLOAD PROBE (PHASE 1) ---
        vxair_log_info("[RTL8852] --- PROBING FIRMWARE HEADER DOWNLOAD ---\n");
        extern const uint8_t rtl8852a_fw_u2_nic[];
        extern const uint32_t rtl8852a_fw_u2_nic_len;

        vxair_log_info("[RTL8852] Polling R_AX_WCPU_FW_CTRL (0x01E0) for B_AX_H2C_PATH_RDY (BIT 1)...\n");
        int h2c_rdy = vxair_mac_poll32(slot_id, 0x01E0, (1 << 1), (1 << 1), 50000, 10);
        if (h2c_rdy == 0) {
            vxair_log_info("[RTL8852] B_AX_H2C_PATH_RDY Reached!\n");
        } else {
        }

        uint32_t fw_buf_phys;
        uint8_t* fw_buf = vxair_hal_dma_alloc(64, &fw_buf_phys);
        memset(fw_buf, 0, 64);
        
        // 1. wd_body_t (24 bytes)
        uint32_t *hdr32 = (uint32_t *)fw_buf;
        hdr32[0] = 0x000C0000; // MAC_AX_DMA_H2C (0xC << 16), NO AX_TXD_FWDL_EN
        hdr32[1] = 0;
        hdr32[2] = 40;         // pktlen = 8 (fwcmd_hdr) + 32 (fw header payload)
        hdr32[3] = 0;
        hdr32[4] = 0;
        hdr32[5] = 0;
        
        // 2. fwcmd_hdr (8 bytes)
        // type = FWCMD_TYPE_H2C (0x0) << 16
        // cat = FWCMD_H2C_CAT_MAC (0x1) << 0
        // class = FWCMD_H2C_CL_FWDL (0x3) << 2
        // func = FWCMD_H2C_FUNC_FWHDR_DL (0x0) << 8
        hdr32[6] = (0x0 << 16) | (0x1 << 0) | (0x3 << 2) | (0x0 << 8);
        hdr32[7] = 40; // H2C_HDR_TOTAL_LEN = 40 (payload + fwcmd_hdr length)(8 bytes)
        
        // 3. Firmware header payload (32 bytes)
        memcpy(fw_buf + 32, rtl8852a_fw_u2_nic, 32);
        
        for (int i=0; i<64; i+=64) __asm__ volatile("clflush (%0)" :: "r"(fw_buf + i) : "memory");
        
        vxair_log_info("[RTL8852] Queuing Firmware Header Command (payload 32, total 64) to EP 0x07...\n");
        int ret = vxair_xhci_queue_bulk_trb(slot_id, 0x07, (void*)(uintptr_t)fw_buf_phys, 64);
        vxair_log_info("[RTL8852] Header transfer result: %d\n", ret);

        vxair_log_info("[RTL8852] Polling R_AX_WCPU_FW_CTRL (0x01E0) for B_AX_FWDL_PATH_RDY (BIT 2)...\n");
        int fwdl_rdy = vxair_mac_poll32(slot_id, 0x01E0, (1 << 2), (1 << 2), 50000, 10);
        if (fwdl_rdy == 0) {
            vxair_log_info("[RTL8852] B_AX_FWDL_PATH_RDY Reached!\n");
        } else {
            vxair_log_info("[RTL8852] ERROR: B_AX_FWDL_PATH_RDY Timeout. Status=0x%08X\n", vxair_mac_read32(slot_id, 0x01E0));
        }
        vxair_log_info("[RTL8852] (No payload chunks will be sent in this pass)\n");
        // ----------------------------------------------
        vxair_log_info("=========================================\n");
    }
}
