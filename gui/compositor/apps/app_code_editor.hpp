#pragma once

#include "../../vxui/vxui.hpp"

// Code / Text Editor App (vxedit)
struct CodeEditorState {
    char file_name[64];
    char text[4096];
    int text_len;
    int cursor_line;
    int cursor_col;
    bool modified;
    int font_size;
    bool show_line_numbers;
};

static CodeEditorState g_code_editor = {
    "untitled.cpp",
    "// Vextryn Air OS Native Code Editor\n"
    "#include <stdio.h>\n\n"
    "int main() {\n"
    "    printf(\"Hello, Vextryn Air OS!\\n\");\n"
    "    return 0;\n"
    "}\n",
    124,
    1,
    1,
    false,
    12,
    true
};

static void draw_app_code_editor(VxWindow& w, uint64_t frame, int mouse_x, int mouse_y, bool clicked) {
    (void)frame; (void)clicked;
    // Window header bar
    vxr_fill_rect(w.x, w.y + 28, w.w, 32, VxTheme::SURFACE_HIGH);
    vxr_fill_rect(w.x, w.y + 59, w.w, 1, VxTheme::BORDER_STRONG);

    // Title / File info
    draw_abstract_char(w.x + 12, w.y + 38, 'F', VxTheme::TEXT_PRIMARY);
    draw_abstract_char(w.x + 20, w.y + 38, 'i', VxTheme::TEXT_PRIMARY);
    draw_abstract_char(w.x + 28, w.y + 38, 'l', VxTheme::TEXT_PRIMARY);
    draw_abstract_char(w.x + 36, w.y + 38, 'e', VxTheme::TEXT_PRIMARY);
    draw_abstract_char(w.x + 44, w.y + 38, ':', VxTheme::TEXT_PRIMARY);

    for (int i = 0; g_code_editor.file_name[i]; i++) {
        draw_abstract_char(w.x + 60 + i * 8, w.y + 38, g_code_editor.file_name[i], VxTheme::ACCENT_GLOW);
    }

    // Editor main area
    int editor_x = w.x;
    int editor_y = w.y + 60;
    int editor_w = w.w;
    int editor_h = w.h - 90;

    vxr_fill_rect(editor_x, editor_y, editor_w, editor_h, VxTheme::BASE_DEEP);

    // Line number gutter
    int gutter_w = 40;
    vxr_fill_rect(editor_x, editor_y, gutter_w, editor_h, VxTheme::SURFACE);
    vxr_fill_rect(editor_x + gutter_w - 1, editor_y, 1, editor_h, VxTheme::BORDER_SUBTLE);

    // Render code text line by line
    int line_num = 1;
    int char_col = 0;
    int cur_x = editor_x + gutter_w + 10;
    int cur_y = editor_y + 10;

    // Draw line number 1
    draw_abstract_char(editor_x + 16, cur_y, '1', VxTheme::TEXT_MUTED);

    for (int i = 0; i < g_code_editor.text_len && g_code_editor.text[i]; i++) {
        char c = g_code_editor.text[i];
        if (c == '\n') {
            line_num++;
            char_col = 0;
            cur_x = editor_x + gutter_w + 10;
            cur_y += 18;

            if (cur_y + 18 > editor_y + editor_h) break;

            // Draw line number
            if (line_num < 10) {
                draw_abstract_char(editor_x + 16, cur_y, '0' + line_num, VxTheme::TEXT_MUTED);
            } else if (line_num < 100) {
                draw_abstract_char(editor_x + 8, cur_y, '0' + (line_num / 10), VxTheme::TEXT_MUTED);
                draw_abstract_char(editor_x + 16, cur_y, '0' + (line_num % 10), VxTheme::TEXT_MUTED);
            }
        } else {
            // Syntax coloring highlight rules
            uint32_t syntax_col = VxTheme::TEXT_PRIMARY;
            if (c == '#' || c == '<' || c == '>') syntax_col = 0xFFF85149; // Preprocessor / Includes
            else if (c == '(' || c == ')' || c == '{' || c == '}') syntax_col = 0xFFD29922; // Brackets
            else if (c == '"') syntax_col = 0xFF3FB950; // String quotes

            draw_abstract_char(cur_x, cur_y, c, syntax_col);
            cur_x += 8;
            char_col++;
        }
    }

    // Status bar at bottom
    int status_y = w.y + w.h - 30;
    vxr_fill_rect(w.x, status_y, w.w, 30, VxTheme::SURFACE_HIGH);
    vxr_fill_rect(w.x, status_y, w.w, 1, VxTheme::BORDER_STRONG);

    const char* status_text = "UTF-8 | C++ | Ready";
    for (int i = 0; status_text[i]; i++) {
        draw_abstract_char(w.x + 12 + i * 8, status_y + 9, status_text[i], VxTheme::TEXT_SECONDARY);
    }
}
