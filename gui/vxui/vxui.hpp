// VXUI — Retained Operating System UI Component Framework
// Multi-layered retained widget tree, automated layout engine, focus manager,
// text editing core with selection/clipboard support, design system integration,
// and rich reusable control gallery.
#ifndef VXUI_HPP
#define VXUI_HPP

#include <stdint.h>
#include <stddef.h>
#include "vxui_theme.hpp"
#include "../vxrender/vxrender.hpp"
#include "../vxrender/vxsurface.hpp"

// Freestanding C++ delete operators
inline void operator delete(void* ptr, unsigned long size) noexcept { (void)ptr; (void)size; }
inline void operator delete(void* ptr) noexcept { (void)ptr; }

// Global drawing convenience wrappers
static inline void vxui_draw_rounded_rect(int x, int y, int w, int h, int radius, uint32_t color) {
    vxr_rounded_rect(x, y, w, h, radius, color);
}

static inline void vxui_draw_shadow(int x, int y, int w, int h, int depth) {
    vxr_shadow(x, y, w, h, depth);
}

// System Abstract Character & Font Drawing
extern "C" void draw_abstract_char(int x, int y, char c, uint32_t color);

#ifdef __cplusplus
extern "C++" {
#endif

// System Clipboard Model for OS-wide copy, cut, and paste
struct VxClipboard {
    static const int MAX_CLIPBOARD_LEN = 512;
    static char buffer[MAX_CLIPBOARD_LEN];
    static int length;

    static void set_text(const char* text, int len = -1) {
        if (!text) { buffer[0] = '\0'; length = 0; return; }
        if (len < 0) { for (len = 0; text[len]; len++); }
        if (len >= MAX_CLIPBOARD_LEN) len = MAX_CLIPBOARD_LEN - 1;
        for (int i = 0; i < len; i++) buffer[i] = text[i];
        buffer[len] = '\0';
        length = len;
    }

    static const char* get_text() { return buffer; }
    static int get_length() { return length; }
};

// Global static storage for clipboard
inline char VxClipboard::buffer[512] = {0};
inline int VxClipboard::length = 0;

#ifdef __cplusplus
}
#endif

// Unified System Event Subsystem
enum VxEventType {
    VX_EV_NONE = 0,
    VX_EV_MOUSE_PRESS,
    VX_EV_MOUSE_RELEASE,
    VX_EV_MOUSE_MOVE,
    VX_EV_MOUSE_SCROLL,
    VX_EV_KEY_DOWN,
    VX_EV_KEY_UP,
    VX_EV_KEY_CHAR,
    VX_EV_FOCUS_GAINED,
    VX_EV_FOCUS_LOST,
    VX_EV_RESIZE
};

struct VxEvent {
    VxEventType type;
    int mouse_x;
    int mouse_y;
    uint8_t mouse_button;
    int scroll_delta;
    uint8_t scancode;
    char key_char;
    bool shift;
    bool ctrl;
    bool alt;
};

// Retained Base Widget Node
class VxWidget {
public:
    int x, y, w, h;
    bool visible;
    bool enabled;
    bool focused;
    bool hovered;
    bool pressed;
    bool focusable;
    VxWidget* parent;

    VxWidget()
        : x(0), y(0), w(0), h(0),
          visible(true), enabled(true), focused(false),
          hovered(false), pressed(false), focusable(false), parent(nullptr) {}

    virtual ~VxWidget() {}

    void set_bounds(int bx, int by, int bw, int bh) {
        x = bx; y = by; w = bw; h = bh;
        layout();
    }

    bool contains(int mx, int my) const {
        return visible && mx >= x && mx < x + w && my >= y && my < y + h;
    }

    virtual void measure(int parent_w, int parent_h, int& out_w, int& out_h) {
        (void)parent_w; (void)parent_h;
        out_w = w; out_h = h;
    }

    virtual void layout() {}
    virtual void draw() {}

    virtual void render(VxSurfaceLayer& surface, const VxClipRect& clip) {
        (void)surface; (void)clip;
        if (!visible) return;
        draw();
    }

    virtual bool on_event(const VxEvent& ev) {
        if (!visible || !enabled) return false;

        if (ev.type == VX_EV_MOUSE_MOVE) {
            bool prev_hover = hovered;
            hovered = contains(ev.mouse_x, ev.mouse_y);
            return hovered != prev_hover;
        } else if (ev.type == VX_EV_MOUSE_PRESS) {
            if (contains(ev.mouse_x, ev.mouse_y)) {
                pressed = true;
                return true;
            }
        } else if (ev.type == VX_EV_MOUSE_RELEASE) {
            if (pressed) {
                pressed = false;
                return true;
            }
        }
        return false;
    }

    void draw_focus_ring(uint32_t color = VxTheme::ACCENT_GLOW) const {
        if (focused && visible) {
            vxr_fill_rect(x - 2, y - 2, w + 4, 2, color);
            vxr_fill_rect(x - 2, y + h, w + 4, 2, color);
            vxr_fill_rect(x - 2, y - 2, 2, h + 4, color);
            vxr_fill_rect(x + w, y - 2, 2, h + 4, color);
        }
    }
};

// Composite Widget Container
class VxContainer : public VxWidget {
public:
    static const int MAX_CHILDREN = 64;
    VxWidget* children[MAX_CHILDREN];
    int child_count;

    VxContainer() : child_count(0) {
        for (int i = 0; i < MAX_CHILDREN; i++) children[i] = nullptr;
    }

    void add_child(VxWidget* child) {
        if (!child || child_count >= MAX_CHILDREN) return;
        child->parent = this;
        children[child_count++] = child;
        layout();
    }

    void remove_all() {
        child_count = 0;
    }

    virtual void render(VxSurfaceLayer& surface, const VxClipRect& clip) override {
        if (!visible) return;
        draw();
        VxClipRect container_clip = {x, y, w, h};
        VxClipRect intersected = clip.intersect(container_clip);
        if (!intersected.valid()) return;

        for (int i = 0; i < child_count; i++) {
            if (children[i] && children[i]->visible) {
                children[i]->render(surface, intersected);
            }
        }
    }

    virtual bool on_event(const VxEvent& ev) override {
        if (!visible || !enabled) return false;

        for (int i = child_count - 1; i >= 0; i--) {
            if (children[i] && children[i]->on_event(ev)) {
                return true;
            }
        }
        return VxWidget::on_event(ev);
    }
};

// Horizontal / Vertical Flex Layout Manager
class VxBoxLayout : public VxContainer {
public:
    enum Direction { HORIZONTAL, VERTICAL };
    Direction dir;
    int spacing;
    int padding;

    VxBoxLayout(Direction d = VERTICAL, int sp = VxTheme::SP_SM, int pad = VxTheme::SP_SM)
        : dir(d), spacing(sp), padding(pad) {}

    virtual void layout() override {
        int cur_x = x + padding;
        int cur_y = y + padding;

        for (int i = 0; i < child_count; i++) {
            if (!children[i] || !children[i]->visible) continue;
            children[i]->x = cur_x;
            children[i]->y = cur_y;

            if (dir == HORIZONTAL) {
                cur_x += children[i]->w + spacing;
            } else {
                cur_y += children[i]->h + spacing;
            }
            children[i]->layout();
        }
    }
};

// Grid Layout Manager
class VxGridLayout : public VxContainer {
public:
    int cols;
    int cell_w;
    int cell_h;
    int gap;
    int padding;

    VxGridLayout(int columns = 4, int cw = 80, int ch = 80, int g = 8, int pad = 12)
        : cols(columns), cell_w(cw), cell_h(ch), gap(g), padding(pad) {}

    virtual void layout() override {
        for (int i = 0; i < child_count; i++) {
            if (!children[i]) continue;
            int r = i / cols;
            int c = i % cols;
            children[i]->x = x + padding + c * (cell_w + gap);
            children[i]->y = y + padding + r * (cell_h + gap);
            children[i]->w = cell_w;
            children[i]->h = cell_h;
            children[i]->layout();
        }
    }
};

// Focus Manager for Tab Navigation & Focus Routing
class VxFocusManager {
public:
    static const int MAX_FOCUSABLE = 64;
    VxWidget* focusable_widgets[MAX_FOCUSABLE];
    int count;
    int current_idx;

    VxFocusManager() : count(0), current_idx(-1) {}

    void register_widget(VxWidget* w) {
        if (w && w->focusable && count < MAX_FOCUSABLE) {
            focusable_widgets[count++] = w;
        }
    }

    void reset() {
        count = 0;
        current_idx = -1;
    }

    void set_focus(VxWidget* target) {
        for (int i = 0; i < count; i++) {
            if (focusable_widgets[i] == target) {
                if (current_idx != -1 && current_idx < count) {
                    focusable_widgets[current_idx]->focused = false;
                }
                current_idx = i;
                focusable_widgets[i]->focused = true;
                return;
            }
        }
    }

    void advance_focus(bool reverse = false) {
        if (count == 0) return;
        if (current_idx != -1 && current_idx < count) {
            focusable_widgets[current_idx]->focused = false;
        }

        if (reverse) {
            current_idx = (current_idx <= 0) ? count - 1 : current_idx - 1;
        } else {
            current_idx = (current_idx + 1) % count;
        }

        if (current_idx >= 0 && current_idx < count) {
            focusable_widgets[current_idx]->focused = true;
        }
    }

    bool handle_key_event(const VxEvent& ev) {
        if (ev.type == VX_EV_KEY_DOWN && ev.key_char == '\t') {
            advance_focus(ev.shift);
            return true;
        }
        return false;
    }
};

// Button Style Variants
enum VxButtonVariant {
    VX_BTN_DEFAULT,
    VX_BTN_PRIMARY,
    VX_BTN_SECONDARY,
    VX_BTN_ACTION,
    VX_BTN_DANGER,
    VX_BTN_DIGIT,
    VX_BTN_OPERATOR,
    VX_BTN_UTILITY
};

// Interactive Button Control
struct VxButton : public VxWidget {
    const char* label;
    VxButtonVariant variant;
    bool is_hovered;
    bool is_pressed;
    bool is_focused;
    bool is_disabled;
    void (*on_click_cb)(VxButton* btn, void* user_data);
    void* user_data;

    VxButton(int bx = 0, int by = 0, int bw = 80, int bh = 32, const char* txt = "", VxButtonVariant var = VX_BTN_DEFAULT, bool hov = false, bool prs = false, bool foc = false, bool dis = false)
        : label(txt), variant(var), is_hovered(hov), is_pressed(prs), is_focused(foc), is_disabled(dis), on_click_cb(nullptr), user_data(nullptr) {
        x = bx; y = by; w = bw; h = bh;
        hovered = hov; pressed = prs; focused = foc; enabled = !dis;
        focusable = true;
    }

    VxButton(const char* txt, VxButtonVariant var = VX_BTN_DEFAULT, int bx = 0, int by = 0, int bw = 80, int bh = 32)
        : label(txt), variant(var), is_hovered(false), is_pressed(false), is_focused(false), is_disabled(false), on_click_cb(nullptr), user_data(nullptr) {
        x = bx; y = by; w = bw; h = bh;
        focusable = true;
    }

    void check_hover(int mx, int my) {
        is_hovered = !is_disabled && contains(mx, my);
        hovered = is_hovered;
    }

    bool handle_click(int mx, int my) {
        if (is_disabled) return false;
        if (contains(mx, my)) {
            is_pressed = true;
            pressed = true;
            return true;
        }
        return false;
    }

    void release() { is_pressed = false; pressed = false; }

    void set_click_handler(void (*cb)(VxButton*, void*), void* data = nullptr) {
        on_click_cb = cb;
        user_data = data;
    }

    virtual void draw() override {
        if (!visible) return;
        uint32_t bg = VxTheme::SURFACE;
        uint32_t text_col = VxTheme::TEXT_PRIMARY;
        uint32_t border_col = VxTheme::BORDER_STRONG;
        int radius = VxTheme::RADIUS_SM;

        bool active_hov = is_hovered || hovered;
        bool active_prs = is_pressed || pressed;
        bool active_foc = is_focused || focused;

        if (is_disabled || !enabled) {
            bg = VxTheme::SURFACE;
            text_col = VxTheme::TEXT_MUTED;
            border_col = VxTheme::BORDER_SUBTLE;
        } else {
            switch (variant) {
            case VX_BTN_PRIMARY:
            case VX_BTN_ACTION:
                bg = active_prs ? VxTheme::ACCENT_DIM : (active_hov ? VxTheme::ACCENT_GLOW : VxTheme::ACCENT);
                text_col = 0xFFFFFFFF;
                border_col = VxTheme::ACCENT_GLOW;
                radius = VxTheme::RADIUS_MD;
                break;
            case VX_BTN_DANGER:
                bg = active_prs ? 0xFFC53030 : (active_hov ? 0xFFE53E3E : VxTheme::DANGER);
                text_col = 0xFFFFFFFF;
                border_col = 0xFFFC8181;
                radius = VxTheme::RADIUS_MD;
                break;
            case VX_BTN_OPERATOR:
                bg = active_prs ? VxTheme::ACCENT_SOFT : (active_hov ? VxTheme::SURFACE_HIGH : VxTheme::SURFACE);
                text_col = VxTheme::ACCENT_GLOW;
                border_col = VxTheme::BORDER_BRIGHT;
                radius = VxTheme::RADIUS_LG;
                break;
            case VX_BTN_DIGIT:
                bg = active_prs ? VxTheme::SURFACE_HIGH : (active_hov ? VxTheme::OVERLAY : VxTheme::SURFACE);
                text_col = VxTheme::TEXT_PRIMARY;
                border_col = VxTheme::BORDER_STRONG;
                radius = VxTheme::RADIUS_MD;
                break;
            case VX_BTN_SECONDARY:
            case VX_BTN_DEFAULT:
            default:
                bg = active_prs ? VxTheme::BORDER_SUBTLE : (active_hov ? VxTheme::SURFACE_HIGH : VxTheme::SURFACE);
                text_col = active_hov ? VxTheme::TEXT_PRIMARY : VxTheme::TEXT_SECONDARY;
                border_col = active_hov ? VxTheme::BORDER_BRIGHT : VxTheme::BORDER_STRONG;
                radius = VxTheme::RADIUS_SM;
                break;
            }
        }

        vxr_rounded_rect(x, y, w, h, radius, bg);
        vxr_fill_rect(x, y, w, 1, active_foc ? VxTheme::ACCENT_GLOW : border_col);
        vxr_fill_rect(x, y + h - 1, w, 1, border_col);
        vxr_fill_rect(x, y, 1, h, border_col);
        vxr_fill_rect(x + w - 1, y, 1, h, border_col);

        if (label && label[0]) {
            int len = 0; for (; label[len]; len++);
            int tx = x + (w - len * VxTheme::FONT_BODY) / 2;
            int ty = y + (h - 12) / 2 + 1;
            for (int i = 0; label[i]; i++) {
                draw_abstract_char(tx + i * VxTheme::FONT_BODY, ty, label[i], text_col);
            }
        }

        if (active_foc) draw_focus_ring();
    }

    virtual bool on_event(const VxEvent& ev) override {
        if (!visible || !enabled || is_disabled) return false;
        bool handled = VxWidget::on_event(ev);

        if (ev.type == VX_EV_MOUSE_RELEASE && (hovered || is_hovered)) {
            if (on_click_cb) on_click_cb(this, user_data);
            return true;
        }

        if ((focused || is_focused) && (ev.type == VX_EV_KEY_DOWN || ev.type == VX_EV_KEY_CHAR)) {
            if (ev.key_char == '\n' || ev.key_char == ' ') {
                if (on_click_cb) on_click_cb(this, user_data);
                return true;
            }
        }
        return handled;
    }
};

// System Text Label Control
struct VxLabel : public VxWidget {
    const char* text;
    uint32_t color;
    int font_size;

    VxLabel(const char* txt = "", int lx = 0, int ly = 0, uint32_t col = VxTheme::TEXT_PRIMARY, int size = VxTheme::FONT_BODY)
        : text(txt), color(col), font_size(size) {
        x = lx; y = ly; w = 120; h = 20;
    }

    VxLabel(int lx, int ly, const char* txt, uint32_t col = VxTheme::TEXT_PRIMARY, int size = VxTheme::FONT_BODY)
        : text(txt), color(col), font_size(size) {
        x = lx; y = ly; w = 120; h = 20;
    }

    virtual void draw() override {
        if (!visible || !text) return;
        for (int i = 0; text[i]; i++) {
            draw_abstract_char(x + i * font_size, y, text[i], color);
        }
    }
};

// Panel Surface Container
struct VxPanel : public VxContainer {
    int elevation;
    uint32_t bg_color;

    VxPanel(int px = 0, int py = 0, int pw = 200, int ph = 150, int elev = 1, uint32_t bg = VxTheme::SURFACE)
        : elevation(elev), bg_color(bg) {
        x = px; y = py; w = pw; h = ph;
    }

    virtual void draw() override {
        if (!visible) return;
        uint32_t bg = bg_color ? bg_color : VxTheme::SURFACE;
        if (elevation > 0) {
            vxr_shadow(x, y, w, h, elevation == 2 ? 12 : 4);
        }
        vxr_fill_rect(x, y, w, h, bg);
        vxr_fill_rect(x, y, w, 1, VxTheme::BORDER_BRIGHT);
        vxr_fill_rect(x, y + h - 1, w, 1, VxTheme::BORDER_STRONG);
        vxr_fill_rect(x, y, 1, h, VxTheme::BORDER_STRONG);
        vxr_fill_rect(x + w - 1, y, 1, h, VxTheme::BORDER_STRONG);
    }
};

// Text Field Control with Caret, Selection Range, Navigation & Clipboard
struct VxTextField : public VxWidget {
    char text_buf[256];
    int length;
    int caret_pos;
    int sel_start;
    int sel_end;
    const char* placeholder;

    VxTextField(int bx = 0, int by = 0, int bw = 200, int bh = 34, const char* initial_text = "")
        : length(0), caret_pos(0), sel_start(-1), sel_end(-1), placeholder("Enter text...") {
        x = bx; y = by; w = bw; h = bh;
        focusable = true;
        set_text(initial_text);
    }

    void set_text(const char* txt) {
        length = 0;
        if (txt) {
            for (; txt[length] && length < 255; length++) {
                text_buf[length] = txt[length];
            }
        }
        text_buf[length] = '\0';
        caret_pos = length;
        sel_start = sel_end = -1;
    }

    void insert_char(char c) {
        if (length >= 255) return;
        if (has_selection()) delete_selection();
        for (int i = length; i > caret_pos; i--) {
            text_buf[i] = text_buf[i - 1];
        }
        text_buf[caret_pos] = c;
        length++;
        caret_pos++;
        text_buf[length] = '\0';
    }

    void backspace() {
        if (has_selection()) { delete_selection(); return; }
        if (caret_pos <= 0) return;
        for (int i = caret_pos - 1; i < length - 1; i++) {
            text_buf[i] = text_buf[i + 1];
        }
        length--;
        caret_pos--;
        text_buf[length] = '\0';
    }

    bool has_selection() const {
        return sel_start >= 0 && sel_end >= 0 && sel_start != sel_end;
    }

    void delete_selection() {
        if (!has_selection()) return;
        int start = sel_start < sel_end ? sel_start : sel_end;
        int end = sel_start < sel_end ? sel_end : sel_start;
        int count = end - start;

        for (int i = start; i < length - count; i++) {
            text_buf[i] = text_buf[i + count];
        }
        length -= count;
        text_buf[length] = '\0';
        caret_pos = start;
        sel_start = sel_end = -1;
    }

    void select_all() {
        sel_start = 0;
        sel_end = length;
        caret_pos = length;
    }

    void copy_to_clipboard() {
        if (has_selection()) {
            int start = sel_start < sel_end ? sel_start : sel_end;
            int end = sel_start < sel_end ? sel_end : sel_start;
            VxClipboard::set_text(text_buf + start, end - start);
        } else {
            VxClipboard::set_text(text_buf, length);
        }
    }

    void cut_to_clipboard() {
        copy_to_clipboard();
        if (has_selection()) delete_selection();
        else set_text("");
    }

    void paste_from_clipboard() {
        const char* clip = VxClipboard::get_text();
        if (!clip) return;
        for (int i = 0; clip[i]; i++) {
            insert_char(clip[i]);
        }
    }

    virtual void draw() override {
        if (!visible) return;
        uint32_t bg = VxTheme::SURFACE;
        uint32_t bcolor = focused ? VxTheme::ACCENT_GLOW : VxTheme::BORDER_STRONG;

        vxr_fill_rect(x, y, w, h, bg);
        vxr_fill_rect(x, y, w, 1, bcolor);
        vxr_fill_rect(x, y + h - 1, w, 1, bcolor);
        vxr_fill_rect(x, y, 1, h, bcolor);
        vxr_fill_rect(x + w - 1, y, 1, h, bcolor);

        int text_y = y + (h - 12) / 2 + 1;

        if (length == 0 && placeholder && !focused) {
            for (int i = 0; placeholder[i]; i++) {
                draw_abstract_char(x + 8 + i * VxTheme::FONT_BODY, text_y, placeholder[i], VxTheme::TEXT_MUTED);
            }
        } else {
            int s_min = (sel_start < sel_end) ? sel_start : sel_end;
            int s_max = (sel_start < sel_end) ? sel_end : sel_start;

            for (int i = 0; i < length; i++) {
                int char_x = x + 8 + i * VxTheme::FONT_BODY;
                if (has_selection() && i >= s_min && i < s_max) {
                    vxr_fill_rect(char_x, text_y - 2, VxTheme::FONT_BODY, 14, VxTheme::ACCENT_SOFT);
                }
                draw_abstract_char(char_x, text_y, text_buf[i], VxTheme::TEXT_PRIMARY);
            }
        }

        if (focused) {
            int cx = x + 8 + caret_pos * VxTheme::FONT_BODY;
            vxr_fill_rect(cx, y + 4, 2, h - 8, VxTheme::ACCENT_GLOW);
        }

        draw_focus_ring();
    }

    virtual bool on_event(const VxEvent& ev) override {
        if (!visible || !enabled) return false;
        bool handled = VxWidget::on_event(ev);

        if (focused && (ev.type == VX_EV_KEY_CHAR || ev.type == VX_EV_KEY_DOWN)) {
            if (ev.ctrl) {
                if (ev.key_char == 'a' || ev.key_char == 'A') { select_all(); return true; }
                if (ev.key_char == 'c' || ev.key_char == 'C') { copy_to_clipboard(); return true; }
                if (ev.key_char == 'x' || ev.key_char == 'X') { cut_to_clipboard(); return true; }
                if (ev.key_char == 'v' || ev.key_char == 'V') { paste_from_clipboard(); return true; }
            }

            if (ev.key_char == '\b') {
                backspace();
                return true;
            } else if (ev.key_char >= 32 && ev.key_char <= 126) {
                insert_char(ev.key_char);
                return true;
            }
        }
        return handled;
    }
};

// Checkbox Control
struct VxCheckbox : public VxWidget {
    const char* label;
    bool checked;
    void (*on_change_cb)(VxCheckbox* cb, void* user_data);
    void* user_data;

    VxCheckbox(const char* txt = "", bool chk = false, int bx = 0, int by = 0)
        : label(txt), checked(chk), on_change_cb(nullptr), user_data(nullptr) {
        x = bx; y = by; w = 120; h = 24;
        focusable = true;
    }

    virtual void draw() override {
        if (!visible) return;
        int box_sz = 18;
        int box_y = y + (h - box_sz) / 2;

        uint32_t bg = checked ? VxTheme::ACCENT : VxTheme::SURFACE;
        vxr_rounded_rect(x, box_y, box_sz, box_sz, VxTheme::RADIUS_SM, bg);
        vxr_rect_bordered(x, box_y, box_sz, box_sz, bg, VxTheme::BORDER_STRONG);

        if (checked) {
            vxr_fill_rect(x + 4, box_y + 8, 4, 2, 0xFFFFFFFF);
            vxr_fill_rect(x + 7, box_y + 6, 6, 2, 0xFFFFFFFF);
        }

        if (label && label[0]) {
            int tx = x + box_sz + 8;
            int ty = y + (h - 12) / 2 + 1;
            for (int i = 0; label[i]; i++) {
                draw_abstract_char(tx + i * VxTheme::FONT_BODY, ty, label[i], VxTheme::TEXT_PRIMARY);
            }
        }
        draw_focus_ring();
    }

    virtual bool on_event(const VxEvent& ev) override {
        if (!visible || !enabled) return false;
        bool handled = VxWidget::on_event(ev);

        if (ev.type == VX_EV_MOUSE_RELEASE && hovered) {
            checked = !checked;
            if (on_change_cb) on_change_cb(this, user_data);
            return true;
        }
        return handled;
    }
};

// Slider Control
struct VxSliderWidget : public VxWidget {
    int value;
    int min_val;
    int max_val;
    int value_pct;
    void (*on_change_cb)(VxSliderWidget* slider, void* user_data);
    void* user_data;

    VxSliderWidget(int val = 50, int min_v = 0, int max_v = 100, int bx = 0, int by = 0, int bw = 150)
        : value(val), min_val(min_v), max_val(max_v), value_pct(val), on_change_cb(nullptr), user_data(nullptr) {
        x = bx; y = by; w = bw; h = 24;
        focusable = true;
    }

    bool handle_drag(int mx, int my) {
        if (my >= y - 10 && my <= y + h + 10 && mx >= x && mx <= x + w) {
            value_pct = ((mx - x) * 100) / (w > 0 ? w : 1);
            if (value_pct < 0) value_pct = 0;
            if (value_pct > 100) value_pct = 100;
            value = min_val + (value_pct * (max_val - min_val)) / 100;
            if (on_change_cb) on_change_cb(this, user_data);
            return true;
        }
        return false;
    }

    virtual void draw() override {
        if (!visible) return;
        int track_h = 6;
        int track_y = y + (h - track_h) / 2;

        vxr_fill_rect(x, track_y, w, track_h, VxTheme::SURFACE_HIGH);
        int fill_w = (value - min_val) * w / (max_val > min_val ? (max_val - min_val) : 1);
        if (fill_w > 0) vxr_fill_rect(x, track_y, fill_w, track_h, VxTheme::ACCENT);

        int handle_w = 14, handle_h = 18;
        int handle_x = x + fill_w - handle_w / 2;
        if (handle_x < x) handle_x = x;
        if (handle_x + handle_w > x + w) handle_x = x + w - handle_w;
        int handle_y = y + (h - handle_h) / 2;

        vxr_rounded_rect(handle_x, handle_y, handle_w, handle_h, VxTheme::RADIUS_SM, VxTheme::TEXT_PRIMARY);
        draw_focus_ring();
    }

    virtual bool on_event(const VxEvent& ev) override {
        if (!visible || !enabled) return false;
        if (ev.type == VX_EV_MOUSE_PRESS || (pressed && ev.type == VX_EV_MOUSE_MOVE)) {
            return handle_drag(ev.mouse_x, ev.mouse_y);
        }
        return VxWidget::on_event(ev);
    }
};

// Progress Bar Control
struct VxProgressBarWidget : public VxWidget {
    int progress; // 0 to 100
    int progress_pct;

    VxProgressBarWidget(int prog = 0, int bx = 0, int by = 0, int bw = 200, int bh = 16)
        : progress(prog), progress_pct(prog) {
        x = bx; y = by; w = bw; h = bh;
    }

    virtual void draw() override {
        if (!visible) return;
        int active_pct = progress_pct ? progress_pct : progress;
        vxr_fill_rect(x, y, w, h, VxTheme::SURFACE_HIGH);
        vxr_rect_bordered(x, y, w, h, VxTheme::SURFACE_HIGH, VxTheme::BORDER_STRONG);

        int fill_w = active_pct * w / 100;
        if (fill_w > 0) {
            vxr_fill_rect(x, y, fill_w, h, VxTheme::ACCENT);
        }
    }
};

#endif // VXUI_HPP
