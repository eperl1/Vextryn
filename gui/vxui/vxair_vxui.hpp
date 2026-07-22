#ifndef VXAIR_VXUI_HPP
#define VXAIR_VXUI_HPP

#include <string>
#include <vector>
#include <memory>
#include "vxair_font.hpp"
#include "../compositor/vxair_vxcomp.hpp"

namespace Vx {
namespace Gui {

class Widget {
public:
    Widget(int x, int y, int w, int h) : x_(x), y_(y), width_(w), height_(h) {}
    virtual ~Widget() = default;
    
    virtual void draw(uint32_t* buffer, int buf_w, int buf_h, FontRenderer& font) = 0;
    virtual void on_click(int mx, int my) = 0;

    bool contains(int mx, int my) const {
        return mx >= x_ && mx < x_ + width_ && my >= y_ && my < y_ + height_;
    }

protected:
    int x_, y_, width_, height_;
};

class Button : public Widget {
public:
    Button(const std::string& label, int x, int y, int w, int h);
    void draw(uint32_t* buffer, int buf_w, int buf_h, FontRenderer& font) override;
    void on_click(int mx, int my) override;
    
    void set_on_click(void (*callback)()) { callback_ = callback; }

private:
    std::string label_;
    bool pressed_;
    void (*callback_)();
};

class Label : public Widget {
public:
    Label(const std::string& text, int x, int y);
    void draw(uint32_t* buffer, int buf_w, int buf_h, FontRenderer& font) override;
    void on_click(int mx, int my) override {} // No-op
private:
    std::string text_;
};

class UIWindow {
public:
    UIWindow(const std::string& title, int x, int y, int w, int h);
    
    void add_widget(std::shared_ptr<Widget> widget);
    void draw(FontRenderer& font);
    void handle_click(int mx, int my);
    
    std::shared_ptr<Vx::Window> get_comp_window() const { return comp_window_; }

private:
    std::string title_;
    std::shared_ptr<Vx::Window> comp_window_;
    std::vector<std::shared_ptr<Widget>> widgets_;
};

} // namespace Gui
} // namespace Vx

#endif // VXAIR_VXUI_HPP
