#ifndef DEBUG_VISUALIZATION_H
#define DEBUG_VISUALIZATION_H

#include "math/Vector2D.h"
#include "graphics/Color.h"
#include <vector>
#include <string>

namespace JJM {
namespace Debug {

enum class DebugDrawMode {
    Lines,
    Circles,
    Rectangles,
    Text
};

class DebugRenderer {
public:
    static DebugRenderer& getInstance();
    
    void drawLine(const Math::Vector2D& start, const Math::Vector2D& end, const Graphics::Color& color);
    void drawCircle(const Math::Vector2D& center, float radius, const Graphics::Color& color, int segments = 32);
    void drawRect(const Math::Vector2D& position, const Math::Vector2D& size, const Graphics::Color& color, bool filled = false);
    void drawText(const Math::Vector2D& position, const std::string& text, const Graphics::Color& color);
    void drawPoint(const Math::Vector2D& position, const Graphics::Color& color, float size = 3.0f);
    void drawArrow(const Math::Vector2D& start, const Math::Vector2D& end, const Graphics::Color& color);
    
    void setEnabled(bool enabled) { this->enabled = enabled; }
    bool isEnabled() const { return enabled; }
    
    void render();
    void clear();
    
private:
    DebugRenderer() : enabled(true) {}
    bool enabled;
    
    struct LineData {
        Math::Vector2D start, end;
        Graphics::Color color;
    };
    
    struct CircleData {
        Math::Vector2D center;
        float radius;
        Graphics::Color color;
        int segments;
    };
    
    struct RectData {
        Math::Vector2D position, size;
        Graphics::Color color;
        bool filled;
    };
    
    struct TextData {
        Math::Vector2D position;
        std::string text;
        Graphics::Color color;
    };
    
    std::vector<LineData> lines;
    std::vector<CircleData> circles;
    std::vector<RectData> rects;
    std::vector<TextData> texts;
};

class DebugOverlay {
public:
    static DebugOverlay& getInstance();
    
    void addStat(const std::string& name, const std::string& value);
    void addStat(const std::string& name, float value);
    void addStat(const std::string& name, int value);
    
    void render();
    void clear();
    
    void setEnabled(bool enabled) { this->enabled = enabled; }
    bool isEnabled() const { return enabled; }
    
private:
    DebugOverlay() : enabled(true) {}
    bool enabled;
    std::vector<std::pair<std::string, std::string>> stats;
};

} // namespace Debug
} // namespace JJM

#endif
