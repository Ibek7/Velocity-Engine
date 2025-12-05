#include "profiler/DebugVisualization.h"
#include <iostream>
#include <sstream>
#include <iomanip>

namespace JJM {
namespace Debug {

DebugRenderer& DebugRenderer::getInstance() {
    static DebugRenderer instance;
    return instance;
}

void DebugRenderer::drawLine(const Math::Vector2D& start, const Math::Vector2D& end, const Graphics::Color& color) {
    if (!enabled) return;
    lines.push_back({start, end, color});
}

void DebugRenderer::drawCircle(const Math::Vector2D& center, float radius, const Graphics::Color& color, int segments) {
    if (!enabled) return;
    circles.push_back({center, radius, color, segments});
}

void DebugRenderer::drawRect(const Math::Vector2D& position, const Math::Vector2D& size, const Graphics::Color& color, bool filled) {
    if (!enabled) return;
    rects.push_back({position, size, color, filled});
}

void DebugRenderer::drawText(const Math::Vector2D& position, const std::string& text, const Graphics::Color& color) {
    if (!enabled) return;
    texts.push_back({position, text, color});
}

void DebugRenderer::drawPoint(const Math::Vector2D& position, const Graphics::Color& color, float size) {
    if (!enabled) return;
    Math::Vector2D halfSize(size / 2.0f, size / 2.0f);
    drawRect(Math::Vector2D(position.x - halfSize.x, position.y - halfSize.y), 
             Math::Vector2D(size, size), color, true);
}

void DebugRenderer::drawArrow(const Math::Vector2D& start, const Math::Vector2D& end, const Graphics::Color& color) {
    if (!enabled) return;
    
    drawLine(start, end, color);
    
    // Calculate arrow head
    Math::Vector2D dir = end - start;
    dir.normalize();
    float arrowSize = 10.0f;
    
    Math::Vector2D perp(-dir.y, dir.x);
    Math::Vector2D arrowBase = end - dir * arrowSize;
    Math::Vector2D arrowLeft = arrowBase + perp * (arrowSize * 0.5f);
    Math::Vector2D arrowRight = arrowBase - perp * (arrowSize * 0.5f);
    
    drawLine(end, arrowLeft, color);
    drawLine(end, arrowRight, color);
}

void DebugRenderer::render() {
    if (!enabled) return;
    
    // In a real implementation, would render using actual graphics API
    if (!lines.empty()) {
        std::cout << "Rendering " << lines.size() << " debug lines" << std::endl;
    }
    if (!circles.empty()) {
        std::cout << "Rendering " << circles.size() << " debug circles" << std::endl;
    }
    if (!rects.empty()) {
        std::cout << "Rendering " << rects.size() << " debug rectangles" << std::endl;
    }
    if (!texts.empty()) {
        std::cout << "Rendering " << texts.size() << " debug texts" << std::endl;
    }
}

void DebugRenderer::clear() {
    lines.clear();
    circles.clear();
    rects.clear();
    texts.clear();
}

DebugOverlay& DebugOverlay::getInstance() {
    static DebugOverlay instance;
    return instance;
}

void DebugOverlay::addStat(const std::string& name, const std::string& value) {
    if (!enabled) return;
    stats.push_back({name, value});
}

void DebugOverlay::addStat(const std::string& name, float value) {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(2) << value;
    addStat(name, oss.str());
}

void DebugOverlay::addStat(const std::string& name, int value) {
    addStat(name, std::to_string(value));
}

void DebugOverlay::render() {
    if (!enabled || stats.empty()) return;
    
    std::cout << "=== Debug Overlay ===" << std::endl;
    for (const auto& [name, value] : stats) {
        std::cout << name << ": " << value << std::endl;
    }
}

void DebugOverlay::clear() {
    stats.clear();
}

} // namespace Debug
} // namespace JJM
