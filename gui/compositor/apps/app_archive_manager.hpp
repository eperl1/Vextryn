#pragma once

#include "../../vxui/vxui.hpp"

// Archive Manager App (vxzip)
struct ArchiveEntry {
    char name[32];
    int uncompressed_size;
    int compressed_size;
};

static ArchiveEntry g_archive_entries[4] = {
    {"kernel_sys.bin", 245760, 98304},
    {"desktop_assets.tar", 524288, 209715},
    {"user_config.cfg", 2048, 512},
    {"system_logs.txt", 16384, 4096}
};

static void draw_app_archive_manager(VxWindow& w, uint64_t frame, int mouse_x, int mouse_y, bool clicked) {
    (void)frame; (void)clicked;
    // Toolbar
    vxr_fill_rect(w.x, w.y + 28, w.w, 40, VxTheme::SURFACE_HIGH);
    vxr_fill_rect(w.x, w.y + 67, w.w, 1, VxTheme::BORDER_STRONG);

    VxButton extract_btn = { w.x + 10, w.y + 32, 90, 32, "Extract All", VX_BTN_PRIMARY, false, false, false, false };
    extract_btn.check_hover(mouse_x, mouse_y);
    extract_btn.draw();

    VxButton add_btn = { w.x + 110, w.y + 32, 80, 32, "Add Files", VX_BTN_SECONDARY, false, false, false, false };
    add_btn.check_hover(mouse_x, mouse_y);
    add_btn.draw();

    // Table view header
    int table_y = w.y + 68;
    vxr_fill_rect(w.x, table_y, w.w, 28, VxTheme::SURFACE);
    vxr_fill_rect(w.x, table_y + 27, w.w, 1, VxTheme::BORDER_STRONG);

    const char* h1 = "File Name";
    const char* h2 = "Size";
    const char* h3 = "Packed";

    for (int i = 0; h1[i]; i++) draw_abstract_char(w.x + 16 + i * 8, table_y + 8, h1[i], VxTheme::TEXT_SECONDARY);
    for (int i = 0; h2[i]; i++) draw_abstract_char(w.x + 200 + i * 8, table_y + 8, h2[i], VxTheme::TEXT_SECONDARY);
    for (int i = 0; h3[i]; i++) draw_abstract_char(w.x + 300 + i * 8, table_y + 8, h3[i], VxTheme::TEXT_SECONDARY);

    // List of archive items
    for (int r = 0; r < 4; r++) {
        int ry = table_y + 28 + r * 34;
        if (ry + 34 > w.y + w.h - 30) break;

        uint32_t bg = (r % 2 == 0) ? VxTheme::BASE_DEEP : VxTheme::SURFACE;
        vxr_fill_rect(w.x, ry, w.w, 34, bg);

        for (int c = 0; g_archive_entries[r].name[c]; c++) {
            draw_abstract_char(w.x + 16 + c * 8, ry + 11, g_archive_entries[r].name[c], VxTheme::TEXT_PRIMARY);
        }

        // Draw sizes
        draw_abstract_char(w.x + 200, ry + 11, '0' + ((g_archive_entries[r].uncompressed_size / 1000) % 10), VxTheme::TEXT_MUTED);
        draw_abstract_char(w.x + 208, ry + 11, 'K', VxTheme::TEXT_MUTED);
        draw_abstract_char(w.x + 216, ry + 11, 'B', VxTheme::TEXT_MUTED);

        draw_abstract_char(w.x + 300, ry + 11, '0' + ((g_archive_entries[r].compressed_size / 1000) % 10), VxTheme::ACCENT_GLOW);
        draw_abstract_char(w.x + 308, ry + 11, 'K', VxTheme::ACCENT_GLOW);
        draw_abstract_char(w.x + 316, ry + 11, 'B', VxTheme::ACCENT_GLOW);

        vxr_fill_rect(w.x + 10, ry + 33, w.w - 20, 1, VxTheme::BORDER_SUBTLE);
    }

    // Status bar at bottom
    int status_y = w.y + w.h - 30;
    vxr_fill_rect(w.x, status_y, w.w, 30, VxTheme::SURFACE_HIGH);
    vxr_fill_rect(w.x, status_y, w.w, 1, VxTheme::BORDER_STRONG);

    const char* info = "4 items, 786 KB (Compression ratio 62%)";
    for (int i = 0; info[i]; i++) {
        draw_abstract_char(w.x + 12 + i * 8, status_y + 9, info[i], VxTheme::TEXT_SECONDARY);
    }
}
