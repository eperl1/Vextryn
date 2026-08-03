#include "bus_xhci.h"
#include "bus_pci.h"
#include "../../kernel/hal/hal_pci.h"
#include "../../kernel/hal/hal_dma.h"
#include "../usb/usb.h"
#include <stddef.h>
#include "../../kernel/core/include/vxair_log.h"
#include <string.h>

#define PCI_CLASS_SERIAL_BUS 0x0C
#define PCI_SUBCLASS_USB     0x03
#define PCI_PROGIF_XHCI      0x30

// Basic xHCI Structures
typedef volatile struct {
    uint32_t param_low;
    uint32_t param_high;
    uint32_t status;
    uint32_t control;
} xhci_trb_t;

typedef volatile struct {
    uint64_t ring_segment_base_address;
    uint16_t ring_segment_size;
    uint16_t rsvd[3];
} xhci_erst_entry_t;

// Contexts
typedef struct {
    uint32_t drop_context_flags;
    uint32_t add_context_flags;
    uint32_t rsvd[6];
} xhci_input_control_ctx_t;

typedef struct {
    uint32_t info[4];
    uint32_t rsvd[4];
} xhci_slot_ctx_t;

typedef struct {
    uint32_t info[2];
    uint64_t tr_dequeue_ptr;
    uint32_t tx_info;
    uint32_t rsvd[3];
} xhci_ep_ctx_t;

// Pointers to mapped regions
static volatile uint8_t* xhci_cap_regs = NULL;
static volatile uint8_t* xhci_op_regs = NULL;
static volatile uint8_t* xhci_rt_regs = NULL;
static volatile uint32_t* xhci_db_regs = NULL;

static uint32_t* dcbaa = NULL;
static xhci_trb_t* cmd_ring = NULL;
static uint32_t cmd_ring_enqueue = 0;
static uint32_t cmd_ring_cycle = 1;

static xhci_trb_t* event_ring = NULL;
static volatile xhci_erst_entry_t* erst = NULL;

static uint64_t cmd_ring_phys = 0;
static uint32_t event_ring_dequeue = 0;
static uint32_t event_ring_cycle = 1;

static xhci_trb_t* ep0_ring = NULL;
static uint64_t ep0_ring_phys = 0;
static uint32_t ep0_enqueue = 0;
static uint32_t ep0_cycle = 1;

static void* input_ctx = NULL;
static void* output_ctx = NULL;
static uint64_t xhci_input_ctx_phys = 0;
static uint8_t xhci_ctx_size = 32;

static xhci_trb_t* ep_rings[32] = {NULL};
static uint32_t ep_enqueues[32] = {0};
static uint32_t ep_cycles[32] = {0};

static uint32_t read32(volatile uint8_t* base, uint32_t offset) {
    return *(volatile uint32_t*)(base + offset);
}

static void write32(volatile uint8_t* base, uint32_t offset, uint32_t val) {
    *(volatile uint32_t*)(base + offset) = val;
}

static void write64(volatile uint8_t* base, uint32_t offset, uint64_t val) {
    *(volatile uint32_t*)(base + offset) = (uint32_t)val;
    *(volatile uint32_t*)(base + offset + 4) = (uint32_t)(val >> 32);
}

static void xhci_delay(volatile int count) {
    while(count--) __asm__ volatile("pause");
}

static void ring_doorbell(uint8_t doorbell, uint8_t target) {
    xhci_db_regs[doorbell] = target;
}

static void send_cmd(uint32_t p_low, uint32_t p_high, uint32_t status, uint32_t control) {
    cmd_ring[cmd_ring_enqueue].param_low = p_low;
    cmd_ring[cmd_ring_enqueue].param_high = p_high;
    cmd_ring[cmd_ring_enqueue].status = status;
    
    __asm__ volatile("mfence" ::: "memory");
    
    // Cycle bit must be written last
    uint32_t ctl = control | (cmd_ring_cycle & 1);
    cmd_ring[cmd_ring_enqueue].control = ctl;
    
    cmd_ring_enqueue++;
    if (cmd_ring_enqueue == 255) { // Link TRB at end
        cmd_ring[255].param_low = (uint32_t)cmd_ring_phys; 
        cmd_ring[255].param_high = (uint32_t)(cmd_ring_phys >> 32);
        cmd_ring[255].status = 0;
        __asm__ volatile("mfence" ::: "memory");
        cmd_ring[255].control = (6 << 10) | 2 | (cmd_ring_cycle & 1); // TRB Type 6 (Link), Toggle Cycle
        cmd_ring_cycle ^= 1;
        cmd_ring_enqueue = 0;
    }
    
    __asm__ volatile("mfence" ::: "memory");
    ring_doorbell(0, 0); // Ring Command Doorbell
}

static xhci_trb_t* wait_event(uint32_t trb_type) {
    // Polling event ring (Wait up to ~1-2 seconds in QEMU)
    for (int i = 0; i < 5000000; i++) {
        uint32_t ctl = event_ring[event_ring_dequeue].control;
        if ((ctl & 1) == (event_ring_cycle & 1)) { // Cycle bit matches
            uint32_t type = (ctl >> 10) & 0x3F;
            xhci_trb_t* ret = &event_ring[event_ring_dequeue];
            
            event_ring_dequeue++;
            if (event_ring_dequeue == 256) {
                event_ring_dequeue = 0;
                event_ring_cycle ^= 1;
            }
            // Update ERDP with physical address
            uint64_t erdp_phys = (uint64_t)(uintptr_t)&event_ring[event_ring_dequeue] - 0xFFFFFFFF80000000ull;
            write64(xhci_rt_regs, 0x20 + 0x18, erdp_phys | 8);
            
            if (type == trb_type) {
                uint8_t comp_code = (ret->status >> 24) & 0xFF;
                uint8_t ep_id = (ret->control >> 16) & 0x1F;
                if (comp_code != 1) { // 1 = Success
                    vxair_log_info("[xHCI] Event %u on EP %u completed with code %u (control: 0x%08X, status: 0x%08X, param: 0x%08X)\n", type, ep_id, comp_code, ret->control, ret->status, ret->param_low);
                }
                // For EP0, short packet (13) and whatever 15 is might be expected, and we wait for Status TRB.
                // But for Bulk, there is no Status TRB! So return the event!
                if ((comp_code == 13 || comp_code == 15) && trb_type == 32) {
                    if (ep_id == 1) { // EP0 (DCI=1)
                        vxair_log_info("[xHCI] Ignoring non-fatal code %u for EP0, waiting for final status...\n", comp_code);
                        continue;
                    }
                }
                return ret;
            }
        }
        xhci_delay(100);
    }
    vxair_log_info("[xHCI] Timeout waiting for event %u\n", trb_type);
    return NULL;
}

void vxair_xhci_submit_control_transfer(uint8_t slot_id, vxair_usb_setup_t* setup, void* data, int dir_in) {
    if (ep0_enqueue + 3 > 255) {
        ep0_ring[ep0_enqueue].param_low = (uint32_t)ep0_ring_phys;
        ep0_ring[ep0_enqueue].param_high = (uint32_t)(ep0_ring_phys >> 32);
        ep0_ring[ep0_enqueue].status = 0;
        __asm__ volatile("mfence" ::: "memory");
        ep0_ring[ep0_enqueue].control = (6 << 10) | 2 | (ep0_cycle & 1); // Type 6 (Link), Toggle Cycle
        ep0_enqueue = 0;
        ep0_cycle ^= 1;
    }

    // 1. Setup Stage TRB
    uint32_t trt = 0; // No Data Stage
    if (setup->wLength > 0) {
        trt = dir_in ? 3 : 2; // In Data : Out Data
    }
    ep0_ring[ep0_enqueue].param_low = *(uint32_t*)setup;
    ep0_ring[ep0_enqueue].param_high = *((uint32_t*)setup + 1);
    ep0_ring[ep0_enqueue].status = 8; // Transfer Length is 8
    ep0_ring[ep0_enqueue].control = (2 << 10) | (trt << 16) | 64 | (ep0_cycle & 1); // Type 2 (Setup), TRT, IDT
    ep0_enqueue++;
    
    // 2. Data Stage TRB
    if (setup->wLength > 0) {
        uint64_t data_phys = (uint64_t)(uintptr_t)data;
        if (data_phys >= 0xFFFFFFFF80000000ull) data_phys -= 0xFFFFFFFF80000000ull;
        
        ep0_ring[ep0_enqueue].param_low = (uint32_t)data_phys;
        ep0_ring[ep0_enqueue].param_high = (uint32_t)(data_phys >> 32);
        ep0_ring[ep0_enqueue].status = setup->wLength; // Transfer Length is bits 0:16
        ep0_ring[ep0_enqueue].control = (3 << 10) | (dir_in ? (1 << 16) : 0) | (ep0_cycle & 1); // Type 3 (Data)
        ep0_enqueue++;
    }
    
    // 3. Status Stage TRB
    ep0_ring[ep0_enqueue].param_low = 0;
    ep0_ring[ep0_enqueue].param_high = 0;
    ep0_ring[ep0_enqueue].status = 0;
    __asm__ volatile("mfence" ::: "memory");
    ep0_ring[ep0_enqueue].control = (4 << 10) | (dir_in ? 0 : (1 << 16)) | 32 | (ep0_cycle & 1); // Type 4 (Status), IOC
    ep0_enqueue++;
    
    __asm__ volatile("mfence" ::: "memory");
    ring_doorbell(slot_id, 1); // EP1 is EP0 control
    
    wait_event(32); // Wait for Transfer Event
}

void vxair_bus_xhci_init(void) {
    vxair_log_info("[xHCI] Driver Initialized\n");
}

int vxair_xhci_configure_endpoint(uint8_t slot_id, uint8_t ep_addr, uint16_t max_packet, uint8_t type) {
    uint8_t ep_num = ep_addr & 0x0F;
    uint8_t dir_in = (ep_addr & 0x80) ? 1 : 0;
    uint8_t dci = (ep_num * 2) + (dir_in ? 1 : 0);
    
    // Allocate Transfer Ring
    uint32_t phys;
    ep_rings[dci] = vxair_hal_dma_alloc(4096, &phys);
    memset(ep_rings[dci], 0, 4096);
    ep_enqueues[dci] = 0;
    ep_cycles[dci] = 1;
    
    // Copy Output Context to Input Context (skipping ICC)
    memcpy((uint8_t*)input_ctx + xhci_ctx_size, (uint8_t*)output_ctx + xhci_ctx_size, xhci_ctx_size * 31);
    
    // Set ICC Add Flags
    xhci_input_control_ctx_t* icc = (xhci_input_control_ctx_t*)input_ctx;
    icc->add_context_flags = (1 << dci) | (1 << 0); // Slot Ctx and Target EP Ctx
    icc->drop_context_flags = 0;
    
    // Update Slot Context entries if needed
    xhci_slot_ctx_t* sc = (xhci_slot_ctx_t*)((uint8_t*)input_ctx + xhci_ctx_size);
    uint32_t entries = (sc->info[0] >> 27) & 0x1F;
    if (dci > entries) {
        sc->info[0] = (sc->info[0] & ~(0x1F << 27)) | (dci << 27);
    }
    
    // Populate Target Endpoint Context
    xhci_ep_ctx_t* ep_ctx = (xhci_ep_ctx_t*)((uint8_t*)input_ctx + xhci_ctx_size * dci);
    ep_ctx->info[0] = 0;
    ep_ctx->info[1] = (3 << 1) | (type << 3) | (max_packet << 16);
    ep_ctx->tr_dequeue_ptr = (uint64_t)phys | 1; // DCS = 1
    ep_ctx->tx_info = (type == 2 || type == 6) ? 512 : 8; // Avg TRB Length

    
    // Issue Configure Endpoint Command
    send_cmd((uint32_t)xhci_input_ctx_phys, (uint32_t)(xhci_input_ctx_phys >> 32), 0, (12 << 10) | (slot_id << 24));
    xhci_trb_t* event = wait_event(33); // Command Completion
    if (!event) return -1;
    
    uint8_t comp_code = (event->status >> 24) & 0xFF;
    vxair_log_info("[xHCI] Configure Endpoint (EP 0x%02X, DCI %u) completed with code %u\n", ep_addr, dci, comp_code);
    return (comp_code == 1) ? 0 : -1;
}

int vxair_xhci_queue_bulk_trb(uint8_t slot_id, uint8_t ep_addr, void* data, uint32_t len) {
    uint8_t ep_num = ep_addr & 0x7F;
    uint8_t dir_in = (ep_addr & 0x80) ? 1 : 0;
    uint8_t dci = (ep_num * 2) + dir_in;
    
    if (!ep_rings[dci]) return -1;
    
    int enq = ep_enqueues[dci];
    int cyc = ep_cycles[dci];
    
    uint64_t data_phys = (uint64_t)(uintptr_t)data;
    if (data_phys >= 0xFFFFFFFF80000000ull) data_phys -= 0xFFFFFFFF80000000ull;
    
    ep_rings[dci][enq].param_low = (uint32_t)data_phys;
    ep_rings[dci][enq].param_high = (uint32_t)(data_phys >> 32);
    ep_rings[dci][enq].status = len;
    __asm__ volatile("mfence" ::: "memory");
    // TRB Type 1 (Normal), IOC = 1, Cycle bit
    ep_rings[dci][enq].control = (1 << 10) | 32 | (cyc & 1);
    
    ep_enqueues[dci]++;
    // TODO: Add Link TRB handling if enq reaches end of ring
    
    __asm__ volatile("mfence" ::: "memory");
    ring_doorbell(slot_id, dci); // Ring the specific EP doorbell
    
    xhci_trb_t* event = wait_event(32); // Wait for Transfer Event
    if (!event) {
        vxair_log_info("[xHCI] Timeout on EP 0x%02X, sending Stop Endpoint Command to abort TRB...\n", ep_addr);
        // Issue Stop Endpoint Command
        send_cmd(0, 0, 0, (15 << 10) | (dci << 16) | (slot_id << 24));
        xhci_trb_t* cmd_event = wait_event(33); // Wait for Command Completion
        if (cmd_event) {
            vxair_log_info("[xHCI] Stop Endpoint Command completed with code %u\n", (cmd_event->status >> 24) & 0xFF);
        }
        
        // The Stop Endpoint command forces a Transfer Event with 'Stopped' code for the active TRB
        event = wait_event(32); 
        if (!event) return -1;
    }
    
    uint8_t comp_code = (event->status >> 24) & 0xFF;
    vxair_log_info("[xHCI] Bulk Transfer on EP 0x%02X completed with code %u\n", ep_addr, comp_code);
    return (comp_code == 1 || comp_code == 13 || comp_code == 17) ? 0 : -1; // Allow Short Packet (13) and Stopped (17)
}

void vxair_bus_xhci_probe(void) {
    uint32_t count = vxair_bus_pci_get_device_count();
    vxair_log_info("[xHCI] Probing... Total PCI devices: %u", count);
    for (uint32_t i = 0; i < count; i++) {
        const vxair_pci_device_t* dev = vxair_bus_pci_get_device(i);
        vxair_log_info("[xHCI] PCI Device %u: Class %02X, Sub %02X, PIF %02X (VID %04X, PID %04X)",
                       i, dev->class_code, dev->subclass, dev->prog_if, dev->vendor_id, dev->device_id);
        if (dev->class_code == PCI_CLASS_SERIAL_BUS &&
            dev->subclass == PCI_SUBCLASS_USB &&
            dev->prog_if == PCI_PROGIF_XHCI) {
            
            // Enable Bus Master and Memory Space
            uint32_t cmd_reg = vxair_hal_pci_read_config(dev->bus, dev->slot, dev->func, 0x04);
            cmd_reg |= (1 << 2) | (1 << 1); // Bus Master, Memory Space
            vxair_hal_pci_write_config(dev->bus, dev->slot, dev->func, 0x04, cmd_reg);
            
            uint32_t bar0 = vxair_hal_pci_read_config(dev->bus, dev->slot, dev->func, 0x10);
            uint32_t bar_addr = bar0 & ~0xF;
            xhci_cap_regs = (volatile uint8_t*)(uintptr_t)bar_addr;
            
            uint8_t caplength = xhci_cap_regs[0];
            xhci_op_regs = xhci_cap_regs + caplength;
            
            uint32_t rtsoff = read32(xhci_cap_regs, 0x18) & ~0x1F;
            xhci_rt_regs = xhci_cap_regs + rtsoff;
            
            uint32_t dboff = read32(xhci_cap_regs, 0x14) & ~0x3;
            xhci_db_regs = (volatile uint32_t*)(xhci_cap_regs + dboff);
            
            uint32_t hcsparams1 = read32(xhci_cap_regs, 0x04);
            uint8_t max_slots = hcsparams1 & 0xFF;
            uint8_t max_ports = (hcsparams1 >> 24) & 0xFF;
            
            uint32_t hccparams1 = read32(xhci_cap_regs, 0x10);
            xhci_ctx_size = (hccparams1 & (1 << 2)) ? 64 : 32;
            
            vxair_log_info("[xHCI] Controller found at BAR0: 0x%X\n", bar0);
            vxair_log_info("[xHCI] MaxSlots: %u, MaxPorts: %u, CtxSize: %u\n", max_slots, max_ports, xhci_ctx_size);
            
            // Wait for CNR
            while (read32(xhci_op_regs, 0x04) & (1 << 11));
            
            // Reset controller
            write32(xhci_op_regs, 0x00, read32(xhci_op_regs, 0x00) & ~1); // Stop
            while ((read32(xhci_op_regs, 0x04) & 1) == 0); // Wait HCHalted
            write32(xhci_op_regs, 0x00, 2); // HCRST
            while (read32(xhci_op_regs, 0x00) & 2);
            while (read32(xhci_op_regs, 0x04) & (1 << 11));
            
            // Set MaxSlotsEn
            write32(xhci_op_regs, 0x38, max_slots);
            
            // Allocations (assume simple 32-bit physical mapping for identity mapped env)
            uint32_t phys;
            dcbaa = vxair_hal_dma_alloc(2048, &phys);
            memset(dcbaa, 0, 2048);
            write64(xhci_op_regs, 0x30, (uint64_t)phys); // DCBAAP
            
            cmd_ring = vxair_hal_dma_alloc(4096, &phys);
            memset((void*)cmd_ring, 0, 4096);
            cmd_ring_phys = phys;
            write64(xhci_op_regs, 0x18, (uint64_t)phys | 1); // CRCR (RCS = 1)
            
            event_ring = vxair_hal_dma_alloc(4096, &phys);
            memset(event_ring, 0, 4096);
            
            erst = vxair_hal_dma_alloc(64, &phys);
            memset(erst, 0, 64);
            erst[0].ring_segment_base_address = (uint64_t)(uintptr_t)event_ring - 0xFFFFFFFF80000000ull;
            erst[0].ring_segment_size = 256;
            
            // Interrupter 0
            write32(xhci_rt_regs, 0x20 + 0x08, 1); // ERSTSZ
            write64(xhci_rt_regs, 0x20 + 0x10, (uint64_t)phys); // ERSTBA
            uint64_t event_ring_phys = (uint64_t)(uintptr_t)event_ring - 0xFFFFFFFF80000000ull;
            write64(xhci_rt_regs, 0x20 + 0x18, event_ring_phys | 8); // ERDP
            
            // Start controller
            write32(xhci_op_regs, 0x00, read32(xhci_op_regs, 0x00) | 1);
            while (read32(xhci_op_regs, 0x04) & 1); // Wait HCHalted == 0
            vxair_log_info("[xHCI] Controller running!\n");
            
            // Check ports
            for (int p = 1; p <= max_ports; p++) {
                uint32_t portsc = read32(xhci_op_regs, 0x400 + (p - 1) * 16);
                if (portsc & 1) { // CCS (Connected)
                    vxair_log_info("[xHCI] Device detected on Port %d\n", p);
                    
                    // Port Reset
                    write32(xhci_op_regs, 0x400 + (p - 1) * 16, portsc | (1 << 4)); // PR
                    while (read32(xhci_op_regs, 0x400 + (p - 1) * 16) & (1 << 4)); // Wait for PR to clear
                    write32(xhci_op_regs, 0x400 + (p - 1) * 16, read32(xhci_op_regs, 0x400 + (p - 1) * 16) | (1 << 21)); // Clear PRC
                    xhci_delay(50000000); // give time for device to settle
                    
                    uint32_t new_portsc = read32(xhci_op_regs, 0x400 + (p - 1) * 16);
                    uint8_t speed = (new_portsc >> 10) & 0xF;
                    vxair_log_info("[xHCI] Port %d Reset Complete. Speed: %u\n", p, speed);
                    
                    // Enable Slot
                    send_cmd(0, 0, 0, 9 << 10); // Enable Slot Cmd
                    xhci_trb_t* event = wait_event(33); // Command Completion Event
                    if (!event) continue;
                    uint8_t slot = (event->control >> 24) & 0xFF;
                    vxair_log_info("[xHCI] Slot %u Enabled\n", slot);
                    
                    // Allocate Contexts
                    input_ctx = vxair_hal_dma_alloc(4096, &phys);
                    memset(input_ctx, 0, 4096);
                    uint64_t input_ctx_phys = phys;
                    xhci_input_ctx_phys = input_ctx_phys;
                    
                    output_ctx = vxair_hal_dma_alloc(4096, &phys);
                    memset(output_ctx, 0, 4096);
                    uint64_t output_ctx_phys = phys;
                    
                    // The DCBAA takes 64-bit physical addresses!
                    ((uint64_t*)dcbaa)[slot] = output_ctx_phys;
                    
                    // Setup Input Context (Add Context 1 for slot, 2 for EP0)
                    xhci_input_control_ctx_t* icc = (xhci_input_control_ctx_t*)input_ctx;
                    icc->add_context_flags = (1 << 0) | (1 << 1);
                    
                    // Slot Context
                    xhci_slot_ctx_t* sc = (xhci_slot_ctx_t*)((uint8_t*)input_ctx + xhci_ctx_size);
                    sc->info[0] = (1 << 27) | (speed << 20); // 1 context entry (EP0)
                    sc->info[1] = p << 16; // Root hub port
                    
                    // EP0 Context
                    ep0_ring = vxair_hal_dma_alloc(4096, &phys);
                    memset(ep0_ring, 0, 4096);
                    ep0_ring_phys = phys;
                    ep0_enqueue = 0;
                    ep0_cycle = 1;
                    
                    xhci_ep_ctx_t* ep0 = (xhci_ep_ctx_t*)((uint8_t*)input_ctx + xhci_ctx_size * 2);
                    ep0->info[1] = (3 << 1) | (4 << 3) | 8; // EP Type 4 (Control), Max Packet 8 (dummy for init)
                    ep0->tr_dequeue_ptr = (uint64_t)phys | 1; // DCS = 1
                    
                    // Address Device Command
                    send_cmd((uint32_t)input_ctx_phys, (uint32_t)(input_ctx_phys >> 32), 0, (11 << 10) | (slot << 24)); // Address Device Cmd
                    wait_event(33);
                    vxair_log_info("[xHCI] Device Addressed successfully on slot %u\n", slot);
                    
                    // Pass to USB Core
                    vxair_usb_handle_device_connect(p, slot, speed);
                    break;
                }
            }
            
            return;
        }
    }
}
