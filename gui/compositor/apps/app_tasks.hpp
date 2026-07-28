#pragma once
// V5 Tasks — Simple to-do list with clickable checkboxes
#include <stdint.h>

struct VxTask {
    char text[40];
    bool done;
    bool in_use;
};

static VxTask g_tasks[12] = {0};
static int g_task_count = 0;

static void tasks_init() {
    // Pre-populate with sample tasks
    const char* samples[4] = {"Explore Vextryn Air", "Try the Calculator", "Open Settings", "Test Terminal"};
    for (int i = 0; i < 4; i++) {
        g_tasks[i].done = (i >= 2);
        g_tasks[i].in_use = true;
        for (int j = 0; samples[i][j] && j < 39; j++)
            g_tasks[i].text[j] = samples[i][j];
        g_tasks[i].text[39] = 0;
    }
    g_task_count = 4;
}

static void draw_app_tasks(VxWindow& w, uint64_t frame, int mouse_x, int mouse_y, bool clicked) {
    uint32_t accent = VxTheme::accent();
    int ax = w.x + 20;
    int ay = w.y + 50;
    int aw = w.w - 40;
    int ah = w.h - 70;
    if (aw <= 0 || ah <= 0) return;

    // Header
    const char* title = "Tasks";
    for (int i = 0; title[i]; i++)
        draw_abstract_char(ax, ay, title[i], VxTheme::TEXT_PRIMARY);
    vxair_fb_fill_rect(ax, ay + 18, 40, 2, accent);

    // Counter
    int done_count = 0;
    for (int i = 0; i < 12; i++) if (g_tasks[i].in_use && g_tasks[i].done) done_count++;
    char count_str[16];
    const char* cs = "0/0 done";
    // Build "X/Y done"
    int cl = 0;
    if (done_count >= 10) { count_str[cl++] = '0' + done_count / 10; count_str[cl++] = '0' + done_count % 10; }
    else count_str[cl++] = '0' + done_count;
    count_str[cl++] = '/';
    int total = 0;
    for (int i = 0; i < 12; i++) if (g_tasks[i].in_use) total++;
    if (total >= 10) { count_str[cl++] = '0' + total / 10; count_str[cl++] = '0' + total % 10; }
    else count_str[cl++] = '0' + total;
    count_str[cl++] = ' '; count_str[cl++] = 'd'; count_str[cl++] = 'o'; count_str[cl++] = 'n'; count_str[cl++] = 'e';
    count_str[cl] = 0;
    for (int i = 0; count_str[i]; i++)
        draw_abstract_char(ax + aw - cl * 8, ay, count_str[i], VxTheme::TEXT_SECONDARY);

    // Divider
    vxair_fb_fill_rect(ax, ay + 26, aw, 1, VxTheme::BORDER_SUBTLE);

    // Task list
    int item_h = 36;
    int list_y = ay + 36;
    for (int i = 0; i < 12; i++) {
        if (!g_tasks[i].in_use) continue;
        int iy = list_y + i * item_h;
        if (iy + item_h > w.y + w.h - 10) break;

        bool hover = (mouse_x >= ax && mouse_x < ax + aw && mouse_y >= iy && mouse_y < iy + item_h);

        // Row background
        if (hover) vxair_fb_fill_rect(ax, iy, aw, item_h - 4, VxTheme::SURFACE_HIGH);

        // Checkbox (16×16)
        int cb_x = ax + 4;
        int cb_y = iy + 8;
        uint32_t cb_bg = g_tasks[i].done ? accent : VxTheme::BASE_DARK;
        uint32_t cb_border = g_tasks[i].done ? accent : VxTheme::BORDER_BRIGHT;
        vxair_fb_fill_rect(cb_x, cb_y, 18, 18, cb_bg);
        vxair_fb_fill_rect(cb_x, cb_y, 18, 1, cb_border);
        vxair_fb_fill_rect(cb_x, cb_y + 17, 18, 1, cb_border);
        vxair_fb_fill_rect(cb_x, cb_y, 1, 18, cb_border);
        vxair_fb_fill_rect(cb_x + 17, cb_y, 1, 18, cb_border);

        // Checkmark
        if (g_tasks[i].done) {
            for (int j = 0; j < 6; j++) {
                vxair_fb_fill_rect(cb_x + 3 + j, cb_y + 9 + j / 2, 2, 2, VxTheme::TEXT_PRIMARY);
            }
            for (int j = 0; j < 8; j++) {
                vxair_fb_fill_rect(cb_x + 7 + j, cb_y + 12 - j / 2, 2, 2, VxTheme::TEXT_PRIMARY);
            }
        }

        // Task text
        uint32_t text_col = g_tasks[i].done ? VxTheme::TEXT_MUTED : VxTheme::TEXT_PRIMARY;
        for (int j = 0; g_tasks[i].text[j] && j < 35; j++) {
            draw_abstract_char(cb_x + 28 + j * 8, cb_y + 4, g_tasks[i].text[j], text_col);
        }

        // Strikethrough for done
        if (g_tasks[i].done) {
            vxair_fb_fill_rect(cb_x + 28, cb_y + 9, 35 * 8, 1, VxTheme::TEXT_MUTED);
        }

        // Click: toggle done
        if (clicked && hover && mouse_x >= cb_x && mouse_x < cb_x + 18) {
            g_tasks[i].done = !g_tasks[i].done;
        }
    }

    // Add task button at bottom
    int btn_y = w.y + w.h - 36;
    int btn_w = 100;
    int btn_x = ax + aw - btn_w;
    bool add_hover = (mouse_x >= btn_x && mouse_x < btn_x + btn_w && mouse_y >= btn_y && mouse_y < btn_y + 28);
    vxair_fb_fill_rect(btn_x, btn_y, btn_w, 28, add_hover ? VxTheme::accent_soft() : VxTheme::SURFACE_HIGH);
    vxair_fb_fill_rect(btn_x, btn_y, btn_w, 1, VxTheme::BORDER_BRIGHT);
    vxair_fb_fill_rect(btn_x, btn_y + 27, btn_w, 1, VxTheme::BORDER_SUBTLE);
    vxair_fb_fill_rect(btn_x, btn_y, 1, 28, VxTheme::BORDER_SUBTLE);
    vxair_fb_fill_rect(btn_x + btn_w - 1, btn_y, 1, 28, VxTheme::BORDER_SUBTLE);
    const char* add_label = "+ New Task";
    for (int i = 0; add_label[i]; i++)
        draw_abstract_char(btn_x + 14 + i * 8, btn_y + 9, add_label[i], add_hover ? accent : VxTheme::TEXT_SECONDARY);

    if (clicked && add_hover) {
        // Add a new task
        for (int i = 0; i < 12; i++) {
            if (!g_tasks[i].in_use) {
                g_tasks[i].in_use = true;
                g_tasks[i].done = false;
                const char* nt = "New Task";
                for (int j = 0; nt[j] && j < 39; j++) g_tasks[i].text[j] = nt[j];
                g_tasks[i].text[8] = 0;
                break;
            }
        }
    }
}
