#ifndef COLOR_H
#define COLOR_H

#include <cstdint>

namespace JJM {
namespace Graphics {

struct Color {
    uint8_t r, g, b, a;

    Color();
    Color(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255);
    
    // Predefined colors
    static Color White();
    static Color Black();
    static Color Red();
    static Color Green();
    static Color Blue();
    static Color Yellow();
    static Color Cyan();
    static Color Magenta();
    static Color Transparent();
    
    // Color blending
    Color blend(const Color& other, float t) const;
    
    // Comparison
    bool operator==(const Color& other) const;
    bool operator!=(const Color& other) const;
};

} // namespace Graphics
} // namespace JJM

#endif // COLOR_H
