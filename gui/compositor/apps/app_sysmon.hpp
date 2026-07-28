#pragma once

#include "../../vxui/vxui_advanced.hpp"

static int sysmon_tab = 0;
static const char* sysmon_tabs[] = { "Processes", "Performance" };
static int sysmon_scroll = 0;

static void draw_app_sysmon(VxWindow& w, uint64_t frame, int mouse_x, int mouse_y, bool clicked) {
    vxr_fill_rect(w.x, w.y + 28, w.w, w.h - 28, VxTheme::BASE_DEEP);

    // Sidebar/TabBar equivalent
    VxSegmentedControl seg;
    seg.x = w.x + 20;
    seg.y = w.y + 28 + 20;
    seg.w = 240;
    seg.h = 32;
    seg.segments = sysmon_tabs;
    seg.segment_count = 2;
    seg.selected_index = sysmon_tab;
    
    if (clicked && mouse_x >= seg.x && mouse_x < seg.x + seg.w && mouse_y >= seg.y && mouse_y < seg.y + seg.h) {
        int clicked_idx = (mouse_x - seg.x) / (seg.w / seg.segment_count);
        if (clicked_idx >= 0 && clicked_idx < seg.segment_count) {
            sysmon_tab = clicked_idx;
        }
    }
    
    seg.draw();
    
    int content_y = seg.y + seg.h + 20;

    if (sysmon_tab == 0) {
        // Processes (Mock list for UI)
        static const char* mock_procs[] = {
            "kernel_task    | PID 0    | 12% CPU",
            "compositor     | PID 1    |  8% CPU",
            "sysmon         | PID 4    |  2% CPU",
            "vxweb          | PID 12   |  0% CPU",
            "idle           | PID -    | 78% CPU"
        };
        
        VxListView list;
        list.x = w.x + 20;
        list.y = content_y;
        list.w = w.w - 40;
        list.h = w.h - (content_y - w.y) - 20;
        list.items = mock_procs;
        list.item_count = 5;
        list.selected_index = -1;
        list.hover_index = -1;
        list.scroll_y = sysmon_scroll;
        
        list.draw();
    } else {
        // Performance
        auto draw_str = [&](int dx, int dy, const char* str, uint32_t col) {
            for (int i = 0; str[i]; i++) draw_abstract_char(dx + i*8, dy, str[i], col);
        };
        
        draw_str(w.x + 20, content_y, "CPU Usage", VxTheme::TEXT_PRIMARY);
        VxProgressBar cpu_bar;
        cpu_bar.x = w.x + 20;
        cpu_bar.y = content_y + 20;
        cpu_bar.w = w.w - 40;
        cpu_bar.h = 16;
        cpu_bar.progress_pct = 22; // Mock 22%
        cpu_bar.draw();
        
        draw_str(w.x + 20, content_y + 60, "RAM Usage", VxTheme::TEXT_PRIMARY);
        VxProgressBar ram_bar;
        ram_bar.x = w.x + 20;
        ram_bar.y = content_y + 80;
        ram_bar.w = w.w - 40;
        ram_bar.h = 16;
        ram_bar.progress_pct = 45; // Mock 45%
        ram_bar.draw();
        
        draw_str(w.x + 20, content_y + 120, "Disk I/O", VxTheme::TEXT_PRIMARY);
        VxProgressBar disk_bar;
        disk_bar.x = w.x + 20;
        disk_bar.y = content_y + 140;
        disk_bar.w = w.w - 40;
        disk_bar.h = 16;
        disk_bar.progress_pct = 5; // Mock 5%
        disk_bar.draw();
        
        draw_str(w.x + 20, content_y + 200, "Dark Mode", VxTheme::TEXT_PRIMARY);
        static bool dark_mode = true;
        VxToggle toggle;
        toggle.x = w.x + w.w - 80;
        toggle.y = content_y + 195;
        toggle.w = 50;
        toggle.h = 24;
        toggle.is_on = dark_mode;
        
        if (clicked && mouse_x >= toggle.x && mouse_x < toggle.x + toggle.w && mouse_y >= toggle.y && mouse_y < toggle.y + toggle.h) {
            dark_mode = !dark_mode;
        }
        toggle.draw();
    }
}
