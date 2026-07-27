#pragma once

#include <stdint.h>

struct VxTextInput {
    char* buffer;
    int* len;
    int max_len;
    int caret_pos;
    int selection_anchor;

    void init(char* buf, int* length_ptr, int max_length) {
        buffer = buf;
        len = length_ptr;
        max_len = max_length;
        caret_pos = *len;
        selection_anchor = caret_pos;
    }

    inline bool sel_active() const { return selection_anchor != caret_pos; }
    inline int sel_min() const { return caret_pos < selection_anchor ? caret_pos : selection_anchor; }
    inline int sel_max() const { return caret_pos > selection_anchor ? caret_pos : selection_anchor; }

    void delete_selection() {
        if (!sel_active() || !buffer || !len) return;
        int s = sel_min();
        int e = sel_max();
        if (s < 0) s = 0;
        if (e > *len) e = *len;
        int count = e - s;
        for (int i = s; i <= *len - count; i++) {
            buffer[i] = buffer[i + count];
        }
        *len -= count;
        caret_pos = s;
        selection_anchor = caret_pos;
    }

    void set_caret(int pos, bool keep_anchor = false) {
        if (pos < 0) pos = 0;
        if (len && pos > *len) pos = *len;
        caret_pos = pos;
        if (!keep_anchor) {
            selection_anchor = caret_pos;
        }
    }

    void select_all() {
        if (!len) return;
        selection_anchor = 0;
        caret_pos = *len;
    }

    // Insert a single character at the caret, replacing any active
    // selection first. Works for ANY insertable char (including '\n' for
    // multi-line fields) so callers don't duplicate the
    // delete-selection / shift / insert / null-terminate sequence.
    // Returns true if the char was inserted, false if the buffer was full
    // (or unbound). The shared edit primitive used by handle_key (for
    // printables) and by app keyboard handlers (for '\n').
    bool insert_char(char c) {
        if (!buffer || !len) return false;
        int current_len = *len;
        if (sel_active()) {
            delete_selection();
            current_len = *len;
        }
        if (current_len < max_len - 1) {
            for (int i = current_len; i > caret_pos; i--) {
                buffer[i] = buffer[i - 1];
            }
            buffer[caret_pos] = c;
            (*len)++;
            caret_pos++;
            selection_anchor = caret_pos;
            buffer[*len] = 0;
            return true;
        }
        return false;
    }

    bool handle_key(char c) {
        if (!buffer || !len) return false;
        int current_len = *len;

        if (c == 17) {
            if (sel_active()) {
                caret_pos = sel_min();
                selection_anchor = caret_pos;
            } else if (caret_pos > 0) {
                caret_pos--;
                selection_anchor = caret_pos;
            }
            return true;
        } else if (c == 18) {
            if (sel_active()) {
                caret_pos = sel_max();
                selection_anchor = caret_pos;
            } else if (caret_pos < current_len) {
                caret_pos++;
                selection_anchor = caret_pos;
            }
            return true;
        } else if (c == 19) {
            if (caret_pos > 0) caret_pos--;
            return true;
        } else if (c == 20) {
            if (caret_pos < current_len) caret_pos++;
            return true;
        } else if (c == 21) {
            caret_pos = 0;
            selection_anchor = 0;
            return true;
        } else if (c == 22) {
            caret_pos = 0;
            return true;
        } else if (c == 23) {
            caret_pos = current_len;
            selection_anchor = current_len;
            return true;
        } else if (c == 24) {
            caret_pos = current_len;
            return true;
        } else if (c == 25) {
            // Ctrl+A → select all text in the buffer
            select_all();
            return true;
        } else if (c == '\b') {
            if (sel_active()) {
                delete_selection();
            } else if (caret_pos > 0) {
                for (int i = caret_pos - 1; i < current_len - 1; i++) {
                    buffer[i] = buffer[i + 1];
                }
                (*len)--;
                caret_pos--;
                selection_anchor = caret_pos;
                buffer[*len] = 0;
            }
            return true;
        } else if (c >= 32 && c <= 126) {
            insert_char(c);
            return true;
        }
        return false;
    }
};

// ─── System clipboard (shared across all text-input apps) ──────────────────
// Session-only in-memory buffer. No per-app duplication: copy/cut/paste
// operate on whichever VxTextInput the app passes.
#define VX_CLIPBOARD_SIZE 512
static char vx_clipboard_buf[VX_CLIPBOARD_SIZE];
static int vx_clipboard_len = 0;

// Copy the active selection from the given VxTextInput into the clipboard.
// If nothing is selected, the clipboard is unchanged.
inline void vx_copy(VxTextInput& input) {
    if (!input.sel_active() || !input.buffer || !input.len) return;
    int s = input.sel_min();
    int e = input.sel_max();
    if (s < 0) s = 0;
    if (e > *input.len) e = *input.len;
    vx_clipboard_len = e - s;
    if (vx_clipboard_len > VX_CLIPBOARD_SIZE - 1) vx_clipboard_len = VX_CLIPBOARD_SIZE - 1;
    for (int i = 0; i < vx_clipboard_len; i++) {
        vx_clipboard_buf[i] = input.buffer[s + i];
    }
    vx_clipboard_buf[vx_clipboard_len] = 0;
}

// Cut: copy selected text to clipboard, then delete the selection.
inline void vx_cut(VxTextInput& input) {
    vx_copy(input);
    input.delete_selection();
}

// Paste: insert clipboard content at the caret of the given VxTextInput.
// Any active selection is replaced first (handled by insert_char).
inline void vx_paste(VxTextInput& input) {
    if (vx_clipboard_len <= 0) return;
    for (int i = 0; i < vx_clipboard_len; i++) {
        input.insert_char(vx_clipboard_buf[i]);
    }
}

// Find the character index in wrapped multi-line text at a mouse position.
// Walks visual lines exactly as the renderers do (char_w advance, wrap at
// wrap_x, '\n' forces a new line), then resolves the column on the target
// row. Shared by every multi-line text field (Notes, Files content editor)
// so click-to-position-caret is consistent and there is zero duplication.
// Returns a value in [0, len]; for len <= 0 returns 0.
inline int vx_wrapped_index_at(const char* buf, int len,
                               int start_x, int start_y, int wrap_x,
                               int line_h, int char_w,
                               int mx, int my) {
    if (!buf || len <= 0) return 0;
    int cx = start_x, cy = start_y;
    int i = 0;
    while (i <= len) {
        int line_end = i;
        int tx = cx;
        while (line_end < len && buf[line_end] != '\n') {
            int next_tx = tx + char_w;
            if (next_tx > wrap_x) break;
            tx = next_tx;
            line_end++;
        }
        if (my >= cy && my < cy + line_h) {
            int col = (mx - cx + char_w / 2) / char_w;
            if (col < 0) col = 0;
            int idx = i + col;
            if (idx > line_end) idx = line_end;
            return idx;
        }
        if (i >= len) break;
        int next_i = line_end;
        if (next_i < len && buf[next_i] == '\n') next_i++;
        if (next_i == i) next_i = i + 1; // safety: always advance
        cx = start_x;
        cy += line_h;
        i = next_i;
    }
    return len;
}
