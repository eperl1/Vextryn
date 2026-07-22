#ifndef VXAIR_VXCOMP_HPP
#define VXAIR_VXCOMP_HPP

#include <vector>
#include <memory>
#include <cstdint>

namespace Vx {

/**
 * @brief Window representation for the compositor
 */
class Window {
public:
    Window(int x, int y, int width, int height);
    virtual ~Window();

    int get_x() const { return x_; }
    int get_y() const { return y_; }
    int get_width() const { return width_; }
    int get_height() const { return height_; }
    bool is_visible() const { return visible_; }
    
    void set_position(int x, int y);
    void set_visible(bool visible);
    
    uint32_t* get_buffer() { return buffer_.data(); }

private:
    int x_, y_;
    int width_, height_;
    bool visible_;
    std::vector<uint32_t> buffer_;
};

/**
 * @brief Display compositor handling window management and rendering
 */
class Compositor {
public:
    Compositor();
    ~Compositor();

    void init();
    void render_frame();
    void add_window(std::shared_ptr<Window> window);
    void remove_window(std::shared_ptr<Window> window);
    
    void set_background_color(uint32_t color) { bg_color_ = color; }

private:
    void blend_pixel(int x, int y, uint32_t color);

    std::vector<std::shared_ptr<Window>> windows_;
    uint32_t bg_color_;
    
    struct Rect { int x, y, w, h; };
    std::vector<Rect> damage_rects_;
};

} // namespace Vx

#endif // VXAIR_VXCOMP_HPP
