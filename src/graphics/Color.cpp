#include "graphics/Color.h"
#include <algorithm>

namespace JJM {
namespace Graphics {

Color::Color() : r(255), g(255), b(255), a(255) {}

Color::Color(uint8_t r, uint8_t g, uint8_t b, uint8_t a) 
    : r(r), g(g), b(b), a(a) {}

Color Color::White() { return Color(255, 255, 255, 255); }
Color Color::Black() { return Color(0, 0, 0, 255); }
Color Color::Red() { return Color(255, 0, 0, 255); }
Color Color::Green() { return Color(0, 255, 0, 255); }
Color Color::Blue() { return Color(0, 0, 255, 255); }
Color Color::Yellow() { return Color(255, 255, 0, 255); }
Color Color::Cyan() { return Color(0, 255, 255, 255); }
Color Color::Magenta() { return Color(255, 0, 255, 255); }
Color Color::Transparent() { return Color(0, 0, 0, 0); }

Color Color::blend(const Color& other, float t) const {
    t = std::max(0.0f, std::min(1.0f, t));
    return Color(
        static_cast<uint8_t>(r + (other.r - r) * t),
        static_cast<uint8_t>(g + (other.g - g) * t),
        static_cast<uint8_t>(b + (other.b - b) * t),
        static_cast<uint8_t>(a + (other.a - a) * t)
    );
}

bool Color::operator==(const Color& other) const {
    return r == other.r && g == other.g && b == other.b && a == other.a;
}

bool Color::operator!=(const Color& other) const {
    return !(*this == other);
}

} // namespace Graphics
} // namespace JJM
