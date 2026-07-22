#ifndef VXAIR_DESKTOP_HPP
#define VXAIR_DESKTOP_HPP

#include <memory>
#include <vector>
#include "../compositor/vxair_vxcomp.hpp"
#include "../vxui/vxair_vxui.hpp"

namespace Vx {
namespace Gui {

class DesktopShell {
public:
    DesktopShell(std::shared_ptr<Compositor> compositor);
    void run();

private:
    void create_taskbar();
    void create_test_window();
    
    std::shared_ptr<Compositor> compositor_;
    FontRenderer font_;
    std::vector<std::shared_ptr<UIWindow>> ui_windows_;
};

} // namespace Gui
} // namespace Vx

#endif // VXAIR_DESKTOP_HPP
