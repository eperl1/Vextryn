#pragma once
// V5 About — Premium system info / about screen
#include <stdint.h>

static void draw_app_about(VxWindow& w, uint64_t frame, int mouse_x, int mouse_y, bool clicked) {
    uint32_t accent = VxTheme::accent();
    int ax = w.x + 24;
    int ay = w.y + 50;
    int aw = w.w - 48;
    int ah = w.h - 70;
    if (aw <= 0 || ah <= 0) return;

    // Hero section: large logo circle + OS name
    int logo_cx = ax + 40;
    int logo_cy = ay + 40;
    // Logo circle
    for (int dy = -32; dy <= 32; dy++) {
        int half = 0;
        for (int dx = -32; dx <= 32; dx++) {
            if (dx * dx + dy * dy <= 32 * 32) half = dx;
        }
        if (half >= 0)
            vxr_fill_rect(logo_cx - half, logo_cy + dy, half * 2 + 1, 1, VxTheme::accent_soft());
    }
    for (int dy = -28; dy <= 28; dy++) {
        int half = 0;
        for (int dx = -28; dx <= 28; dx++) {
            if (dx * dx + dy * dy <= 28 * 28) half = dx;
        }
        if (half >= 0)
            vxr_fill_rect(logo_cx - half, logo_cy + dy, half * 2 + 1, 1, VxTheme::SURFACE);
    }
    // "V" inside logo
    const char* v = "V";
    for (int i = 0; v[i]; i++)
        draw_abstract_char(logo_cx - 4 + i * 10, logo_cy - 8, v[i], accent);

    // OS name and version
    const char* os_name = "Vextryn Air";
    for (int i = 0; os_name[i]; i++)
        draw_abstract_char(ax + 90 + i * 10, ay + 16, os_name[i], VxTheme::TEXT_PRIMARY);
    const char* version = "V5  Build 2026.07.28";
    for (int i = 0; version[i]; i++)
        draw_abstract_char(ax + 90 + i * 8, ay + 36, version[i], VxTheme::TEXT_SECONDARY);
    const char* tagline = "Lightweight. Premium. Yours.";
    for (int i = 0; tagline[i]; i++)
        draw_abstract_char(ax + 90 + i * 8, ay + 52, tagline[i], VxTheme::TEXT_MUTED);

    // Divider
    vxr_fill_rect(ax, ay + 84, aw, 1, VxTheme::BORDER_SUBTLE);

    // Spec cards — 2×3 grid
    struct SpecRow { const char* label; const char* value; };
    SpecRow specs[6] = {
        {"KERNEL",    "Vextryn Air x86_64"},
        {"ARCH",      "x86_64 / UEFI"},
        {"MEMORY",    "512 MB"},
        {"CPU",       "QEMU qemu64 / 4 cores"},
        {"DISPLAY",   "VGA Framebuffer"},
        {"STORAGE",   "ATA / VxAirFS"},
    };
    int card_w = (aw - 16) / 2;
    int card_h = 56;
    for (int i = 0; i < 6; i++) {
        int r = i / 2;
        int c = i % 2;
        int cx = ax + c * (card_w + 16);
        int cy = ay + 100 + r * (card_h + 12);
        if (cy + card_h > w.y + w.h - 8) break;

        VxPanel card{cx, cy, card_w, card_h, 0, VxTheme::SURFACE_HIGH};
        card.draw();
        // Accent dot
        vxr_fill_rect(cx + 10, cy + 10, 6, 6, accent);
        // Label
        for (int j = 0; specs[i].label[j]; j++)
            draw_abstract_char(cx + 24 + j * 8, cy + 10, specs[i].label[j], VxTheme::TEXT_MUTED);
        // Value
        for (int j = 0; specs[i].value[j]; j++)
            draw_abstract_char(cx + 14 + j * 10, cy + 30, specs[i].value[j], VxTheme::TEXT_PRIMARY);
    }

    // Footer
    int fy = w.y + w.h - 24;
    const char* footer = "Built with VXUI Framework";
    for (int i = 0; footer[i]; i++)
        draw_abstract_char(ax + i * 8, fy, footer[i], VxTheme::TEXT_MUTED);
    // Accent dot at end
    vxr_fill_rect(ax + 26 * 8, fy + 4, 4, 4, accent);
}
