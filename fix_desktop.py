import sys

with open('/home/ethan/Vextryn_Air/gui/compositor/vxair_vxcomp.cpp', 'r') as f:
    lines = f.readlines()

start_idx = -1
end_idx = -1
for i, line in enumerate(lines):
    if "static void draw_polished_desktop(uint32_t W, uint32_t H) {" in line:
        start_idx = i
    if "        // V2 Final Cursor: crisp, bright, electric blue tip" in line:
        end_idx = i
        break

if start_idx != -1 and end_idx != -1:
    new_code = """    static void draw_polished_desktop(uint32_t W, uint32_t H) {
        // Workspace dark slate background
        for (uint32_t y = 0; y < H; y++) {
            uint32_t color = VxColor::lerp(0xFF090C12, 0xFF121722, y, H);
            vxr_fill_rect(0, y, W, 1, color);
        }

        // Draw windows on stage
        for (int z = 0; z < 16; z++) {
            int i = g_z_order[z];
            if (g_state.windows[i].open && !g_state.windows[i].is_minimized) {
                draw_window(g_state.windows[i], g_window_clicked[i]);
            }
        }

        if (g_state.focus_dim) {
            for (int i = 0; i < 16; i++) {
                VxWindow& ww = g_state.windows[i];
                if (ww.open && !ww.focused) {
                    vxr_fill_rect(ww.x, ww.y, ww.w, ww.h, 0x66000000);
                }
            }
        }

        // ===== FULL-WIDTH BOTTOM TASKBAR (56px) =====
        uint32_t dock_h = 56;
        uint32_t dock_y = H - dock_h;
        vxr_fill_rect(0, dock_y, W, dock_h, 0xFF111111);
        vxr_fill_rect(0, dock_y, W, 1, 0xFF2A3648); // Subtle top border

        // --- ZONE 1: LEFT (Launcher & Apps) ---
        uint32_t lx = 12;
        uint32_t ly = dock_y + 8;
        
        bool launcher_hover = (g_state.mouse_x >= (int)lx && g_state.mouse_x <= (int)lx + 40 && g_state.mouse_y >= (int)ly && g_state.mouse_y <= (int)ly + 40);
        vxui_draw_rounded_rect(lx, ly, 40, 40, 8, launcher_hover ? 0xFF1C2536 : 0xFF111111);
        uint32_t accent = VxTheme::accent();
        vxr_fill_rect(lx + 14, ly + 14, 5, 5, accent);
        vxr_fill_rect(lx + 21, ly + 14, 5, 5, accent);
        vxr_fill_rect(lx + 14, ly + 21, 5, 5, accent);
        vxr_fill_rect(lx + 21, ly + 21, 5, 5, accent);

        uint32_t tx_base = lx + 52;
        for (int i = 0; i < 16; i++) {
            if (g_state.windows[i].open) {
                bool ihover = (g_state.mouse_x >= (int)tx_base && g_state.mouse_x <= (int)tx_base + 40 && g_state.mouse_y >= (int)ly && g_state.mouse_y <= (int)ly + 40);
                uint32_t item_bg = g_state.windows[i].focused ? 0xFF1A2333 : (ihover ? 0xFF161E2C : 0xFF111111);
                vxui_draw_rounded_rect(tx_base, ly, 40, 40, 8, item_bg);
                if (g_state.windows[i].focused) {
                    vxr_fill_rect(tx_base + 12, ly + 36, 16, 2, accent);
                } else if (g_state.windows[i].open) {
                    vxr_fill_rect(tx_base + 16, ly + 36, 8, 2, 0xFF4A5568);
                }
                draw_app_icon_in_cell(tx_base + 4, ly + 4, 32, 32, g_state.windows[i].app - 1, ihover);
                tx_base += 48;
            }
        }

        // --- ZONE 2: CENTER (Workspaces) ---
        uint32_t cx = (W - 36) / 2;
        uint32_t cy = dock_y + 26;
        vxr_circle(cx, cy, 3, accent);
        vxr_circle(cx + 12, cy, 2, 0xFF4A5568);
        vxr_circle(cx + 24, cy, 2, 0xFF4A5568);

        // --- ZONE 3: RIGHT (Tray & Clock) ---
        uint32_t rx = W - 12 - 60 - 80; 
        uint32_t ry = dock_y + 8;
        
        vxr_fill_rect(rx + 10, ry + 16, 10, 8, accent);
        vxr_fill_rect(rx + 26, ry + 16, 8, 8, 0xFF4A5568);
        vxr_fill_rect(rx + 40, ry + 16, 12, 8, 0xFF4A5568);
        
        uint32_t clock_x = W - 12 - 50;
        uint32_t clock_y_text = dock_y + 14;
        const char* clock_str = "14:30";
        for (int i = 0; clock_str[i]; i++) {
            draw_abstract_char(clock_x + i * 8, clock_y_text, clock_str[i], 0xFFFFFFFF);
        }
        const char* date_str = "Aug 3";
        for (int i = 0; date_str[i]; i++) {
            draw_abstract_char(clock_x + i * 8, clock_y_text + 14, date_str[i], 0xFF8A96A8);
        }

        // Launcher Overlay
        if (g_state.launcher_open) {
            VxLauncherLayout L = compute_launcher_layout(W, H, g_state.launcher_search, g_state.launcher_search_len);
            vxui_draw_rounded_rect(L.card.x, L.card.y, L.card.w, L.card.h, 12, 0xFF141C2B);
            vxr_rounded_rect(L.card.x, L.card.y, L.card.w, L.card.h, 12, 0xFF2A3648);
            
            for (int i = 0; i < L.item_count; i++) {
                int ax = L.items[i].x;
                int ay = L.items[i].y;
                bool ihover = L.items[i].contains(g_state.mouse_x, g_state.mouse_y);
                if (ihover) {
                    vxui_draw_rounded_rect(ax, ay, L.items[i].w, L.items[i].h, 8, 0xFF1D2638);
                }
                draw_app_icon_in_cell(L.icon_cells[i].x, L.icon_cells[i].y, L.icon_cells[i].w, L.icon_cells[i].h, g_app_ids[L.app_indices[i]] - 1, ihover);
                
                const char* name = g_app_names[L.app_indices[i]];
                int name_w = 0;
                for (int j = 0; name[j]; j++) name_w += 8;
                int name_x = ax + (L.items[i].w - name_w) / 2;
                for (int j = 0; name[j]; j++) {
                    draw_abstract_char(name_x + j * 8, ay + 64, name[j], 0xFFFFFFFF);
                }
            }
        }
"""
    new_lines = lines[:start_idx] + [new_code + "\n"] + lines[end_idx:]
    with open('/home/ethan/Vextryn_Air/gui/compositor/vxair_vxcomp.cpp', 'w') as f:
        f.writelines(new_lines)
    print("Fixed vxair_vxcomp.cpp")
else:
    print("Could not find bounds")
