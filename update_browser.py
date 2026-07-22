import re

with open("gui/compositor/vxair_vxcomp.cpp", "r") as f:
    text = f.read()

# Include app_browser
if '#include "apps/app_browser.hpp"' not in text:
    text = text.replace('#include "apps/app_snake.hpp"', '#include "apps/app_snake.hpp"\n    #include "apps/app_browser.hpp"')

# Add VX_APP_BROWSER
if 'VX_APP_BROWSER' not in text:
    text = text.replace('VX_APP_SNAKE\n    };', 'VX_APP_SNAKE,\n        VX_APP_BROWSER\n    };')

# Increase window arrays and loops
text = re.sub(r'VxWindow windows\[7\];', 'VxWindow windows[8];', text)
text = re.sub(r'g_z_order\[7\] = \{0, 1, 2, 3, 4, 5, 6\};', 'g_z_order[8] = {0, 1, 2, 3, 4, 5, 6, 7};', text)
text = re.sub(r'g_window_clicked\[7\]', 'g_window_clicked[8]', text)
text = re.sub(r'app_ids\[7\] = \{VX_APP_CALCULATOR, VX_APP_NOTES, VX_APP_SYSMON, VX_APP_FILES, VX_APP_SETTINGS, VX_APP_TERMINAL, VX_APP_SNAKE\};',
              'app_ids[8] = {VX_APP_CALCULATOR, VX_APP_NOTES, VX_APP_SYSMON, VX_APP_FILES, VX_APP_SETTINGS, VX_APP_TERMINAL, VX_APP_SNAKE, VX_APP_BROWSER};', text)
text = re.sub(r'app_names\[7\] = \{"Calculator", "Notes", "SysMon", "Files", "Settings", "Terminal", "Snake"\};',
              'app_names[8] = {"Calculator", "Notes", "SysMon", "Files", "Settings", "Terminal", "Snake", "Browser"};', text)
text = re.sub(r'menu_h = 7 \* 40', 'menu_h = 8 * 40', text)

# Update loops (change 7 to 8, 6 to 7 for z-order)
text = re.sub(r'for \(int i=0; i<7; i\+\+\)', 'for (int i=0; i<8; i++)', text)
text = re.sub(r'for \(int i = 0; i < 7; i\+\+\)', 'for (int i = 0; i < 8; i++)', text)
text = re.sub(r'for \(int z = 6; z >= 0; z--\)', 'for (int z = 7; z >= 0; z--)', text)
text = re.sub(r'for \(int i = pos; i < 6; i\+\+\)', 'for (int i = pos; i < 7; i++)', text)
text = re.sub(r'g_z_order\[6\] = window_idx;', 'g_z_order[7] = window_idx;', text)

# Update dragging loops which are bounded by 5
text = re.sub(r'for \(int i=0; i<5; i\+\+\)', 'for (int i=0; i<8; i++)', text)

# Add drawing logic
if 'draw_app_browser(' not in text:
    text = text.replace('} else if (w.app == VX_APP_SETTINGS) {\n            draw_app_settings(w, g_frame, g_state.mouse_x, g_state.mouse_y, clicked);\n        }', 
                        '} else if (w.app == VX_APP_SETTINGS) {\n            draw_app_settings(w, g_frame, g_state.mouse_x, g_state.mouse_y, clicked);\n        } else if (w.app == VX_APP_BROWSER) {\n            draw_app_browser(w, g_frame, g_state.mouse_x, g_state.mouse_y, clicked);\n        }')

# Add initial window state
if 'g_state.windows[7]' not in text:
    text = text.replace('g_state.windows[6] = {false, VX_APP_SNAKE, 200, 200, 400, 428, false, 0, 0, false};',
                        'g_state.windows[6] = {false, VX_APP_SNAKE, 200, 200, 400, 428, false, 0, 0, false};\n        g_state.windows[7] = {false, VX_APP_BROWSER, 80, 80, 640, 480, false, 0, 0, false};')

with open("gui/compositor/vxair_vxcomp.cpp", "w") as f:
    f.write(text)

print("Done")
