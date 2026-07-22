#include "vxair_vxui.hpp"

namespace Vx {
namespace Gui {

Button::Button(const std::string& label, int x, int y, int w, int h) 
    : Widget(x, y, w, h), label_(label), pressed_(false), callback_(nullptr) {
}

void Button::draw(uint32_t* buffer, int buf_w, int buf_h, FontRenderer& font) {
    uint32_t bg_color = pressed_ ? 0xFF888888 : 0xFFCCCCCC;
    uint32_t text_color = 0xFF000000;
    
    // Draw button background
    for (int r = 0; r < height_; ++r) {
        for (int c = 0; c < width_; ++c) {
            int px = x_ + c;
            int py = y_ + r;
            if (px >= 0 && px < buf_w && py >= 0 && py < buf_h) {
                buffer[py * buf_w + px] = bg_color;
            }
        }
    }
    
    // Draw text (centered approximation)
    int text_x = x_ + (width_ - label_.length() * 8) / 2;
    int text_y = y_ + (height_ - 8) / 2;
    font.render_text(label_, text_x, text_y, text_color, buffer, buf_w, buf_h);
}

void Button::on_click(int mx, int my) {
    (void)mx; (void)my;
    pressed_ = true;
    if (callback_) {
        callback_();
    }
}

Label::Label(const std::string& text, int x, int y) 
    : Widget(x, y, text.length() * 8, 8), text_(text) {}

void Label::draw(uint32_t* buffer, int buf_w, int buf_h, FontRenderer& font) {
    font.render_text(text_, x_, y_, 0xFFFFFFFF, buffer, buf_w, buf_h);
}

UIWindow::UIWindow(const std::string& title, int x, int y, int w, int h) 
    : title_(title) {
    comp_window_ = std::make_shared<Vx::Window>(x, y, w, h);
}

void UIWindow::add_widget(std::shared_ptr<Widget> widget) {
    widgets_.push_back(widget);
}

void UIWindow::draw(FontRenderer& font) {
    uint32_t* buf = comp_window_->get_buffer();
    int w = comp_window_->get_width();
    int h = comp_window_->get_height();
    
    // Clear background
    for (int i = 0; i < w * h; ++i) {
        buf[i] = 0xFF444444; // Dark gray background
    }
    
    // Draw title bar
    for (int y = 0; y < 20 && y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            buf[y * w + x] = 0xFF222288; // Blue title bar
        }
    }
    font.render_text(title_, 4, 6, 0xFFFFFFFF, buf, w, h);
    
    // Draw widgets
    for (auto& widget : widgets_) {
        widget->draw(buf, w, h, font);
    }
}

void UIWindow::handle_click(int mx, int my) {
    for (auto& widget : widgets_) {
        if (widget->contains(mx, my)) {
            widget->on_click(mx, my);
        }
    }
}

} // namespace Gui
} // namespace Vx
