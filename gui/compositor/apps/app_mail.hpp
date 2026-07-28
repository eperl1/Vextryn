#pragma once

#include "../../vxui/vxui_advanced.hpp"

static const char* mail_sidebar_items[] = { "Inbox", "Sent", "Drafts", "Trash" };
static const char* mail_list_items[] = { 
    "Meeting at 10 AM", 
    "Project Update", 
    "Welcome to Vextryn", 
    "Weekly Report", 
    "Server Alert", 
    "Lunch?", 
    "Invoice #1029", 
    "New Login Seen" 
};

static int mail_sidebar_sel = 0;
static int mail_list_sel = 2;
static int mail_list_scroll = 0;

static void draw_app_mail(VxWindow& w, uint64_t frame, int mouse_x, int mouse_y, bool clicked) {
    // Top-level layout
    vxr_fill_rect(w.x, w.y + 28, w.w, w.h - 28, VxTheme::BASE_DEEP);

    // Sidebar
    VxSidebar sidebar;
    sidebar.x = w.x;
    sidebar.y = w.y + 28;
    sidebar.w = 140;
    sidebar.h = w.h - 28;
    sidebar.items = mail_sidebar_items;
    sidebar.item_count = 4;
    sidebar.selected_index = mail_sidebar_sel;
    
    // Check sidebar clicks
    if (clicked && mouse_x >= sidebar.x && mouse_x < sidebar.x + sidebar.w && mouse_y >= sidebar.y) {
        int clicked_idx = (mouse_y - sidebar.y - 10) / 40;
        if (clicked_idx >= 0 && clicked_idx < sidebar.item_count) {
            mail_sidebar_sel = clicked_idx;
        }
    }
    
    sidebar.draw_better();
    
    // List View
    VxListView list_view;
    list_view.x = w.x + 140;
    list_view.y = w.y + 28;
    list_view.w = 200;
    list_view.h = w.h - 28;
    list_view.items = mail_list_items;
    list_view.item_count = 8;
    list_view.selected_index = mail_list_sel;
    list_view.scroll_y = mail_list_scroll;
    
    // List view hover and click
    list_view.hover_index = -1;
    if (mouse_x >= list_view.x && mouse_x < list_view.x + list_view.w && mouse_y >= list_view.y) {
        int hover = (mouse_y - list_view.y + list_view.scroll_y) / 48;
        if (hover >= 0 && hover < list_view.item_count) {
            list_view.hover_index = hover;
            if (clicked) {
                mail_list_sel = hover;
            }
        }
    }
    
    list_view.draw();
    
    // Detail View (Message center)
    int detail_x = list_view.x + list_view.w;
    int detail_y = w.y + 28;
    int detail_w = w.w - 340;
    int detail_h = w.h - 28;
    
    vxr_fill_rect(detail_x, detail_y, detail_w, detail_h, VxTheme::BASE_DEEP);
    vxr_fill_rect(detail_x, detail_y, 1, detail_h, VxTheme::BORDER_SUBTLE);
    
    if (mail_list_sel >= 0 && mail_list_sel < 8) {
        // Toolbar area in detail
        vxr_fill_rect(detail_x, detail_y, detail_w, 60, VxTheme::SURFACE);
        vxr_fill_rect(detail_x, detail_y + 59, detail_w, 1, VxTheme::BORDER_SUBTLE);
        
        // Subject
        const char* subject = mail_list_items[mail_list_sel];
        int subj_len = 0; for(; subject[subj_len]; subj_len++);
        for (int i=0; i<subj_len; i++) {
            draw_abstract_char(detail_x + 30 + i*8, detail_y + 20, subject[i], VxTheme::TEXT_PRIMARY);
        }
        
        // Content body mockup
        const char* body1 = "Hi there,";
        const char* body2 = "This is a mockup message for the Vextryn Air OS Mail app.";
        const char* body3 = "We are currently testing the new VxUI advanced components.";
        const char* body4 = "Regards,";
        const char* body5 = "Vextryn Team";
        
        auto draw_str = [&](int dx, int dy, const char* str, uint32_t col) {
            for (int i = 0; str[i]; i++) {
                draw_abstract_char(dx + i*8, dy, str[i], col);
            }
        };
        
        draw_str(detail_x + 30, detail_y + 90, body1, VxTheme::TEXT_SECONDARY);
        draw_str(detail_x + 30, detail_y + 120, body2, VxTheme::TEXT_SECONDARY);
        draw_str(detail_x + 30, detail_y + 140, body3, VxTheme::TEXT_SECONDARY);
        draw_str(detail_x + 30, detail_y + 180, body4, VxTheme::TEXT_SECONDARY);
        draw_str(detail_x + 30, detail_y + 200, body5, VxTheme::TEXT_PRIMARY);
        
        // Action buttons
        VxButton reply_btn = { detail_x + 30, detail_y + detail_h - 60, 100, 36, "Reply", VX_BTN_PRIMARY, false, false, false, false };
        reply_btn.check_hover(mouse_x, mouse_y);
        if (reply_btn.handle_click(mouse_x, mouse_y)) {
            // logic
        }
        reply_btn.draw();
        
        VxButton forward_btn = { detail_x + 140, detail_y + detail_h - 60, 100, 36, "Forward", VX_BTN_SECONDARY, false, false, false, false };
        forward_btn.check_hover(mouse_x, mouse_y);
        if (forward_btn.handle_click(mouse_x, mouse_y)) {
            // logic
        }
        forward_btn.draw();
    } else {
        const char* msg = "No message selected";
        int msg_len = 0; for(; msg[msg_len]; msg_len++);
        for (int i=0; i<msg_len; i++) {
            draw_abstract_char(detail_x + (detail_w - msg_len*8)/2 + i*8, detail_y + detail_h/2, msg[i], VxTheme::TEXT_MUTED);
        }
    }
}
