#include "vxair_desktop.hpp"
#include "../../drivers/input/vxair_input.h"

namespace Vx {
namespace Gui {

DesktopShell::DesktopShell(std::shared_ptr<Compositor> compositor)
    : compositor_(compositor) {
    compositor_->set_background_color(0xFF008080); // Classic teal background
    
    create_taskbar();
    create_test_window();
}

void DesktopShell::create_taskbar() {
    auto taskbar = std::make_shared<UIWindow>("", 0, 768 - 30, 1024, 30);
    
    auto start_btn = std::make_shared<Button>("Start", 2, 2, 80, 26);
    taskbar->add_widget(start_btn);
    
    ui_windows_.push_back(taskbar);
    compositor_->add_window(taskbar->get_comp_window());
}

void DesktopShell::create_test_window() {
    auto win = std::make_shared<UIWindow>("System Info", 100, 100, 300, 200);
    
    auto label = std::make_shared<Label>("Vextryn Air OS v2.0", 10, 30);
    win->add_widget(label);
    
    auto btn = std::make_shared<Button>("OK", 100, 150, 100, 30);
    win->add_widget(btn);
    
    ui_windows_.push_back(win);
    compositor_->add_window(win->get_comp_window());
}

void DesktopShell::run() {
    // Main desktop event loop (dummy implementation)
    bool running = true;
    while (running) {
        // 1. Process input events
        vxair_input_event_t event;
        while (vxair_input_poll_event(&event)) {
            if (event.type == VXAIR_INPUT_EV_MOUSE_CLICK) {
                // In a real OS we would track global mouse position
                // For now pass 0, 0 as dummy coordinates
                for (auto& win : ui_windows_) {
                    win->handle_click(0, 0); 
                }
            } else if (event.type == VXAIR_INPUT_EV_KEY) {
                if (event.data.key.keycode == 0x01) { // ESC key to exit
                    running = false;
                }
            }
        }
        
        // 2. Update windows / UI state
        for (auto& win : ui_windows_) {
            win->draw(font_);
        }
        
        // 3. Render frame via compositor
        compositor_->render_frame();
        
        // Break for this dummy implementation to avoid infinite loop
        running = false; 
    }
}

} // namespace Gui
} // namespace Vx
