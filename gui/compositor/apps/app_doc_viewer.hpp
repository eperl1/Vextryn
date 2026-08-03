#pragma once

#include "../../vxui/vxui.hpp"

// Document / PDF Viewer App (vxdoc)
static int g_doc_current_page = 1;
static int g_doc_total_pages = 4;
static int g_doc_zoom = 100;

static void draw_app_doc_viewer(VxWindow& w, uint64_t frame, int mouse_x, int mouse_y, bool clicked) {
    (void)frame; (void)clicked;
    // Toolbar
    vxr_fill_rect(w.x, w.y + 28, w.w, 36, VxTheme::SURFACE_HIGH);
    vxr_fill_rect(w.x, w.y + 63, w.w, 1, VxTheme::BORDER_STRONG);

    // Page navigation buttons
    VxButton prev_btn = { w.x + 10, w.y + 32, 60, 28, "< Prev", VX_BTN_SECONDARY, false, false, false, false };
    prev_btn.check_hover(mouse_x, mouse_y);
    if (prev_btn.handle_click(mouse_x, mouse_y) && g_doc_current_page > 1) {
        g_doc_current_page--;
    }
    prev_btn.draw();

    VxButton next_btn = { w.x + 80, w.y + 32, 60, 28, "Next >", VX_BTN_SECONDARY, false, false, false, false };
    next_btn.check_hover(mouse_x, mouse_y);
    if (next_btn.handle_click(mouse_x, mouse_y) && g_doc_current_page < g_doc_total_pages) {
        g_doc_current_page++;
    }
    next_btn.draw();

    // Page counter text
    const char* pstr = "Page 1 of 4";
    if (g_doc_current_page == 2) pstr = "Page 2 of 4";
    else if (g_doc_current_page == 3) pstr = "Page 3 of 4";
    else if (g_doc_current_page == 4) pstr = "Page 4 of 4";

    for (int i = 0; pstr[i]; i++) {
        draw_abstract_char(w.x + 160 + i * 8, w.y + 40, pstr[i], VxTheme::TEXT_PRIMARY);
    }

    // Document canvas
    int canvas_x = w.x;
    int canvas_y = w.y + 64;
    int canvas_w = w.w;
    int canvas_h = w.h - 64;

    vxr_fill_rect(canvas_x, canvas_y, canvas_w, canvas_h, VxTheme::BASE_DEEP);

    // Document page paper card (White page)
    int page_w = canvas_w - 80;
    int page_h = canvas_h - 40;
    if (page_w < 100) page_w = 100;
    if (page_h < 100) page_h = 100;
    int page_x = canvas_x + (canvas_w - page_w) / 2;
    int page_y = canvas_y + 20;

    vxr_shadow(page_x, page_y, page_w, page_h, 12);
    vxr_fill_rect(page_x, page_y, page_w, page_h, 0xFFFFFFFF); // Paper white

    // Paper content lines & text
    const char* doc_title = "Vextryn Air OS Architecture Whitepaper";
    for (int i = 0; doc_title[i]; i++) {
        draw_abstract_char(page_x + 30 + i * 8, page_y + 30, doc_title[i], 0xFF101B2B);
    }

    vxr_fill_rect(page_x + 30, page_y + 50, page_w - 60, 2, 0xFF2F81F7);

    const char* body_lines[] = {
        "1. Executive Overview: Modular Operating System Platform Architecture",
        "2. Offscreen Surface Compositing and Damage Tracker Engine",
        "3. Unified Retained UI Component Framework and Focus Routing",
        "4. System-Wide Keyboard Accessibility and Text Editing Engine",
        "5. Desktop Shell Environment, Taskbar, Launcher & Control Center",
        "6. First-Party Desktop Application Ecosystem & System Services"
    };

    for (int r = 0; r < 6; r++) {
        int ly = page_y + 70 + r * 28;
        if (ly + 20 > page_y + page_h) break;
        for (int c = 0; body_lines[r][c]; c++) {
            draw_abstract_char(page_x + 30 + c * 8, ly, body_lines[r][c], 0xFF4A5568);
        }
    }
}
