with open("gui/compositor/apps/app_browser.hpp", "r") as f:
    code = f.read()

new_code = code.replace("static bool url_focused = false;", "static bool url_focused = false;\n    static int current_page = 0; // 0=home, 1=google")

# Hook back button
new_code = new_code.replace("""
    draw_btn(back_x, 0xFFABB2BF); // Back""", """
    draw_btn(back_x, 0xFFABB2BF); // Back
    if (clicked && mouse_x >= back_x && mouse_x < back_x + btn_w && mouse_y >= btn_y && mouse_y < btn_y + btn_h) {
        current_page = 0;
    }""")

# Hook the first card to go to Google
card_logic_old = """            bool hover = (mouse_x >= cx && mouse_x < cx + card_w && mouse_y >= grid_y && mouse_y < grid_y + card_h);
            fill(cx, grid_y, card_w, card_h, hover ? 0xFFE4E6EB : 0xFFFFFFFF);"""

card_logic_new = """            bool hover = (mouse_x >= cx && mouse_x < cx + card_w && mouse_y >= grid_y && mouse_y < grid_y + card_h);
            fill(cx, grid_y, card_w, card_h, hover ? 0xFFE4E6EB : 0xFFFFFFFF);
            if (hover && clicked && i == 0) {
                current_page = 1;
            }"""
new_code = new_code.replace(card_logic_old, card_logic_new)

# Add the google page rendering
content_render_old = """    fill(start_x, content_y, width, content_h, 0xFFF0F2F5);

    // Hero panel"""

content_render_new = """    fill(start_x, content_y, width, content_h, 0xFFFFFFFF);

    if (current_page == 1) {
        // Google mockup
        int logo_w = 200, logo_h = 60;
        int logo_x = start_x + (width - logo_w) / 2;
        int logo_y = content_y + content_h / 3 - 40;
        if (logo_y > content_y) {
            // "G" blue, "o" red, "o" yellow, "g" blue, "l" green, "e" red abstract
            fill(logo_x, logo_y, 40, 60, 0xFF4285F4);
            fill(logo_x + 45, logo_y + 20, 30, 40, 0xFFEA4335);
            fill(logo_x + 80, logo_y + 20, 30, 40, 0xFFFBBC05);
            fill(logo_x + 115, logo_y + 20, 30, 40, 0xFF4285F4);
            fill(logo_x + 150, logo_y, 10, 60, 0xFF34A853);
            fill(logo_x + 165, logo_y + 20, 30, 40, 0xFFEA4335);
            
            // Search box
            int search_w = width / 2;
            int search_h = 40;
            int search_x = start_x + (width - search_w) / 2;
            int search_y = logo_y + logo_h + 30;
            fill(search_x, search_y, search_w, search_h, 0xFFFFFFFF);
            // Search border
            fill(search_x, search_y, search_w, 2, 0xFFDFE1E5);
            fill(search_x, search_y + search_h - 2, search_w, 2, 0xFFDFE1E5);
            fill(search_x, search_y, 2, search_h, 0xFFDFE1E5);
            fill(search_x + search_w - 2, search_y, 2, search_h, 0xFFDFE1E5);
            
            // Two buttons
            int btn_w2 = 120, btn_h2 = 36;
            fill(search_x + search_w/2 - btn_w2 - 10, search_y + search_h + 30, btn_w2, btn_h2, 0xFFF8F9FA);
            fill(search_x + search_w/2 + 10, search_y + search_h + 30, btn_w2, btn_h2, 0xFFF8F9FA);
        }
    } else {
        fill(start_x, content_y, width, content_h, 0xFFF0F2F5);
        // Hero panel"""

new_code = new_code.replace(content_render_old, content_render_new)

# Add closing brace for else block
new_code = new_code.replace("""            // Card text
            fill(cx + (card_w - 50) / 2, grid_y + 50, 50, 4, 0xFF9CA3AF);
        }
    }
}""", """            // Card text
            fill(cx + (card_w - 50) / 2, grid_y + 50, 50, 4, 0xFF9CA3AF);
        }
    }
    } // end else
}""")

with open("gui/compositor/apps/app_browser.hpp", "w") as f:
    f.write(new_code)
print("Mockup updated")
