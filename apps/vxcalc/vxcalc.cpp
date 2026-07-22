#include "../../gui/vxui/vxui_widgets.hpp"
#include "../../userspace/libc/include/vxlibc.h"
namespace Vx {
class VxCalc {
private:
    double value; double stored; char op; bool fresh; char display[32];
    void update_display() {
        if (value == (long long)value) vxlibc_snprintf(display, sizeof(display), "%lld", (long long)value);
        else vxlibc_snprintf(display, sizeof(display), "%.6g", value);
    }
public:
    VxCalc() : value(0), stored(0), op(0), fresh(true) { vxlibc_strcpy(display, "0"); }
    void press_digit(int d) { if (fresh) { value = d; fresh = false; } else { value = value * 10 + d; } update_display(); }
    void press_op(char o) { stored = value; op = o; fresh = true; }
    void press_equals() {
        if (!op) return;
        double result = stored;
        switch(op) {
        case '+': result += value; break;
        case '-': result -= value; break;
        case '*': result *= value; break;
        case '/': if (value == 0) { vxlibc_strcpy(display, "Error"); op = 0; return; } result /= value; break;
        case '%': result = (long long)stored % (long long)value; break;
        }
        value = result; op = 0; fresh = true; update_display();
    }
    void press_clear() { value = 0; stored = 0; op = 0; fresh = true; vxlibc_strcpy(display, "0"); }
    const char* get_display() { return display; }
};
}
extern "C" int vxcalc_main(int argc, char** argv) {
    Vx::VxCalc calc;
    vxlibc_printf("=== vxcalc ===\n");
    char buf[64];
    while (1) {
        vxlibc_printf("[%s]> ", calc.get_display());
        if (!vxlibc_fgets(buf, sizeof(buf), 0)) break;
        char c = buf[0];
        if (c >= '0' && c <= '9') calc.press_digit(c - '0');
        else if (c=='+' || c=='-' || c=='*' || c=='/' || c=='%') calc.press_op(c);
        else if (c == '=') calc.press_equals();
        else if (c=='c' || c=='C') calc.press_clear();
        else if (c=='q') break;
    }
    return 0;
}
