#include "debug/DebugVisualization.h"
#include <cstdarg>
#include <cstdio>
#include <algorithm>
#include <chrono>

namespace JJM {
namespace Debug {

// DebugDraw implementation
DebugDraw& DebugDraw::getInstance() {
    static DebugDraw instance;
    return instance;
}

DebugDraw::DebugDraw() : enabled(true), depthTest(false) {
}

DebugDraw::~DebugDraw() {
}

void DebugDraw::drawLine(const Math::Vector2D& start, const Math::Vector2D& end,
                        const Graphics::Color& color, float thickness, float duration) {
    if (!enabled) return;
    
    DebugPrimitive prim;
    prim.type = DebugPrimitiveType::Line;
    prim.points = {start, end};
    prim.color = color;
    prim.thickness = thickness;
    prim.duration = duration;
    prim.timeRemaining = duration;
    primitives.push_back(prim);
}

void DebugDraw::drawCircle(const Math::Vector2D& center, float radius,
                          const Graphics::Color& color, float thickness,
                          bool filled, float duration) {
    if (!enabled) return;
    
    DebugPrimitive prim;
    prim.type = DebugPrimitiveType::Circle;
    prim.points = {center, Math::Vector2D(radius, 0)};
    prim.color = color;
    prim.thickness = thickness;
    prim.filled = filled;
    prim.duration = duration;
    prim.timeRemaining = duration;
    primitives.push_back(prim);
}

void DebugDraw::drawRectangle(const Math::Vector2D& min, const Math::Vector2D& max,
                             const Graphics::Color& color, float thickness,
                             bool filled, float duration) {
    if (!enabled) return;
    
    DebugPrimitive prim;
    prim.type = DebugPrimitiveType::Rectangle;
    prim.points = {min, max};
    prim.color = color;
    prim.thickness = thickness;
    prim.filled = filled;
    prim.duration = duration;
    prim.timeRemaining = duration;
    primitives.push_back(prim);
}

void DebugDraw::drawPolygon(const std::vector<Math::Vector2D>& points,
                           const Graphics::Color& color, float thickness,
                           bool filled, float duration) {
    if (!enabled) return;
    
    DebugPrimitive prim;
    prim.type = DebugPrimitiveType::Polygon;
    prim.points = points;
    prim.color = color;
    prim.thickness = thickness;
    prim.filled = filled;
    prim.duration = duration;
    prim.timeRemaining = duration;
    primitives.push_back(prim);
}

void DebugDraw::drawText(const Math::Vector2D& position, const std::string& text,
                        const Graphics::Color& color, float duration) {
    if (!enabled) return;
    
    DebugPrimitive prim;
    prim.type = DebugPrimitiveType::Text;
    prim.points = {position};
    prim.text = text;
    prim.color = color;
    prim.duration = duration;
    prim.timeRemaining = duration;
    primitives.push_back(prim);
}

void DebugDraw::drawArrow(const Math::Vector2D& start, const Math::Vector2D& end,
                         const Graphics::Color& color, float thickness, float duration) {
    if (!enabled) return;
    
    DebugPrimitive prim;
    prim.type = DebugPrimitiveType::Arrow;
    prim.points = {start, end};
    prim.color = color;
    prim.thickness = thickness;
    prim.duration = duration;
    prim.timeRemaining = duration;
    primitives.push_back(prim);
}

void DebugDraw::drawCross(const Math::Vector2D& center, float size,
                         const Graphics::Color& color, float thickness, float duration) {
    if (!enabled) return;
    
    DebugPrimitive prim;
    prim.type = DebugPrimitiveType::Cross;
    prim.points = {center, Math::Vector2D(size, 0)};
    prim.color = color;
    prim.thickness = thickness;
    prim.duration = duration;
    prim.timeRemaining = duration;
    primitives.push_back(prim);
}

void DebugDraw::drawGrid(const Math::Vector2D& origin, float cellSize,
                        int width, int height, const Graphics::Color& color, float thickness) {
    if (!enabled) return;
    
    DebugPrimitive prim;
    prim.type = DebugPrimitiveType::Grid;
    prim.points = {origin, Math::Vector2D(cellSize, 0),
                   Math::Vector2D(static_cast<float>(width), static_cast<float>(height))};
    prim.color = color;
    prim.thickness = thickness;
    prim.duration = 0.0f;
    prim.timeRemaining = 0.0f;
    primitives.push_back(prim);
}

void DebugDraw::drawAABB(const Math::Vector2D& min, const Math::Vector2D& max,
                        const Graphics::Color& color) {
    drawRectangle(min, max, color);
}

void DebugDraw::drawRay(const Math::Vector2D& origin, const Math::Vector2D& direction,
                       float length, const Graphics::Color& color) {
    Math::Vector2D end = origin + direction * length;
    drawArrow(origin, end, color);
}

void DebugDraw::drawVelocity(const Math::Vector2D& position, const Math::Vector2D& velocity,
                            const Graphics::Color& color) {
    Math::Vector2D end = position + velocity;
    drawArrow(position, end, color);
}

void DebugDraw::update(float deltaTime) {
    // Remove expired primitives
    primitives.erase(
        std::remove_if(primitives.begin(), primitives.end(),
            [deltaTime](DebugPrimitive& prim) {
                if (prim.duration > 0.0f) {
                    prim.timeRemaining -= deltaTime;
                    return prim.timeRemaining <= 0.0f;
                }
                return false;
            }),
        primitives.end()
    );
}

void DebugDraw::render() {
    if (!enabled) return;
    // Stub implementation - would render all primitives
}

void DebugDraw::clear() {
    primitives.clear();
}

void DebugDraw::setEnabled(bool enabled) {
    this->enabled = enabled;
}

bool DebugDraw::isEnabled() const {
    return enabled;
}

void DebugDraw::setDepthTest(bool enabled) {
    depthTest = enabled;
}

bool DebugDraw::getDepthTest() const {
    return depthTest;
}

// DebugTextOverlay implementation
DebugTextOverlay& DebugTextOverlay::getInstance() {
    static DebugTextOverlay instance;
    return instance;
}

DebugTextOverlay::DebugTextOverlay() : enabled(true) {
}

DebugTextOverlay::~DebugTextOverlay() {
}

void DebugTextOverlay::addText(const std::string& text, const Math::Vector2D& position,
                              const Graphics::Color& color) {
    if (!enabled) return;
    frameText.push_back({text, position, color});
}

void DebugTextOverlay::addTextFormat(const Math::Vector2D& position,
                                    const Graphics::Color& color,
                                    const char* format, ...) {
    if (!enabled) return;
    
    char buffer[1024];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    
    addText(buffer, position, color);
}

void DebugTextOverlay::addPersistentText(const std::string& key, const std::string& text,
                                        const Math::Vector2D& position,
                                        const Graphics::Color& color) {
    if (!enabled) return;
    persistentText[key] = {text, position, color};
}

void DebugTextOverlay::removePersistentText(const std::string& key) {
    persistentText.erase(key);
}

void DebugTextOverlay::render() {
    if (!enabled) return;
    // Stub implementation - would render all text
}

void DebugTextOverlay::clear() {
    frameText.clear();
}

void DebugTextOverlay::clearPersistent() {
    persistentText.clear();
}

void DebugTextOverlay::setEnabled(bool enabled) {
    this->enabled = enabled;
}

bool DebugTextOverlay::isEnabled() const {
    return enabled;
}

// PerformanceGraph implementation
PerformanceGraph::PerformanceGraph(const std::string& name, size_t maxSamples)
    : name(name), maxSamples(maxSamples), rangeMin(0.0f), rangeMax(100.0f),
      autoScale(true), color(0, 255, 0) {
}

PerformanceGraph::~PerformanceGraph() {
}

void PerformanceGraph::addSample(float value) {
    samples.push_back(value);
    if (samples.size() > maxSamples) {
        samples.erase(samples.begin());
    }
    
    if (autoScale && !samples.empty()) {
        rangeMin = *std::min_element(samples.begin(), samples.end());
        rangeMax = *std::max_element(samples.begin(), samples.end());
    }
}

void PerformanceGraph::render(const Math::Vector2D& position, const Math::Vector2D& size) {
    // Stub implementation - would render graph
    (void)position;
    (void)size;
}

void PerformanceGraph::setRange(float min, float max) {
    rangeMin = min;
    rangeMax = max;
}

void PerformanceGraph::setAutoScale(bool enabled) {
    autoScale = enabled;
}

void PerformanceGraph::setColor(const Graphics::Color& color) {
    this->color = color;
}

float PerformanceGraph::getMin() const {
    return samples.empty() ? 0.0f : *std::min_element(samples.begin(), samples.end());
}

float PerformanceGraph::getMax() const {
    return samples.empty() ? 0.0f : *std::max_element(samples.begin(), samples.end());
}

float PerformanceGraph::getAverage() const {
    if (samples.empty()) return 0.0f;
    float sum = 0.0f;
    for (float sample : samples) {
        sum += sample;
    }
    return sum / samples.size();
}

void PerformanceGraph::clear() {
    samples.clear();
}

// DebugConsoleVisual implementation
DebugConsoleVisual& DebugConsoleVisual::getInstance() {
    static DebugConsoleVisual instance;
    return instance;
}

DebugConsoleVisual::DebugConsoleVisual() : maxLines(100), visible(false) {
}

DebugConsoleVisual::~DebugConsoleVisual() {
}

void DebugConsoleVisual::log(const std::string& message) {
    entries.push_back({message, Graphics::Color(255, 255, 255),
                      std::chrono::system_clock::now()});
    if (entries.size() > maxLines) {
        entries.erase(entries.begin());
    }
}

void DebugConsoleVisual::logWarning(const std::string& message) {
    entries.push_back({message, Graphics::Color(255, 255, 0),
                      std::chrono::system_clock::now()});
    if (entries.size() > maxLines) {
        entries.erase(entries.begin());
    }
}

void DebugConsoleVisual::logError(const std::string& message) {
    entries.push_back({message, Graphics::Color(255, 0, 0),
                      std::chrono::system_clock::now()});
    if (entries.size() > maxLines) {
        entries.erase(entries.begin());
    }
}

void DebugConsoleVisual::render() {
    if (!visible) return;
    // Stub implementation - would render console
}

void DebugConsoleVisual::clear() {
    entries.clear();
}

void DebugConsoleVisual::setVisible(bool visible) {
    this->visible = visible;
}

bool DebugConsoleVisual::isVisible() const {
    return visible;
}

void DebugConsoleVisual::setMaxLines(size_t maxLines) {
    this->maxLines = maxLines;
}

size_t DebugConsoleVisual::getMaxLines() const {
    return maxLines;
}

// DebugStatsDisplay implementation
DebugStatsDisplay& DebugStatsDisplay::getInstance() {
    static DebugStatsDisplay instance;
    return instance;
}

DebugStatsDisplay::DebugStatsDisplay() : position(10, 10), visible(true) {
}

DebugStatsDisplay::~DebugStatsDisplay() {
}

void DebugStatsDisplay::update(float deltaTime) {
    (void)deltaTime;
}

void DebugStatsDisplay::render() {
    if (!visible) return;
    // Stub implementation - would render stats
}

void DebugStatsDisplay::setStat(const std::string& name, const std::string& value) {
    stats[name] = value;
}

void DebugStatsDisplay::removeStat(const std::string& name) {
    stats.erase(name);
}

void DebugStatsDisplay::setVisible(bool visible) {
    this->visible = visible;
}

bool DebugStatsDisplay::isVisible() const {
    return visible;
}

void DebugStatsDisplay::setPosition(const Math::Vector2D& position) {
    this->position = position;
}

Math::Vector2D DebugStatsDisplay::getPosition() const {
    return position;
}

// DebugCameraVisual implementation
DebugCameraVisual& DebugCameraVisual::getInstance() {
    static DebugCameraVisual instance;
    return instance;
}

DebugCameraVisual::DebugCameraVisual() : enabled(false) {
}

DebugCameraVisual::~DebugCameraVisual() {
}

void DebugCameraVisual::visualizeCamera(const Math::Vector2D& position, float zoom) {
    if (!enabled) return;
    (void)position;
    (void)zoom;
    // Stub implementation
}

void DebugCameraVisual::visualizeFrustum(const Math::Vector2D& min, const Math::Vector2D& max) {
    if (!enabled) return;
    DebugDraw::getInstance().drawRectangle(min, max, Graphics::Color(255, 255, 0));
}

void DebugCameraVisual::render() {
    if (!enabled) return;
    // Stub implementation
}

void DebugCameraVisual::setEnabled(bool enabled) {
    this->enabled = enabled;
}

bool DebugCameraVisual::isEnabled() const {
    return enabled;
}

// DebugEntityVisual implementation
DebugEntityVisual& DebugEntityVisual::getInstance() {
    static DebugEntityVisual instance;
    return instance;
}

DebugEntityVisual::DebugEntityVisual() : enabled(false) {
}

DebugEntityVisual::~DebugEntityVisual() {
}

void DebugEntityVisual::visualizeEntity(uint32_t entityId, const Math::Vector2D& position) {
    if (!enabled) return;
    entities.push_back({entityId, position, Math::Vector2D(), Math::Vector2D()});
}

void DebugEntityVisual::visualizeEntityBounds(uint32_t entityId,
                                             const Math::Vector2D& min,
                                             const Math::Vector2D& max) {
    if (!enabled) return;
    
    // Find existing entity or create new one
    bool found = false;
    for (auto& entity : entities) {
        if (entity.id == entityId) {
            entity.boundsMin = min;
            entity.boundsMax = max;
            found = true;
            break;
        }
    }
    
    if (!found) {
        entities.push_back({entityId, Math::Vector2D(), min, max});
    }
}

void DebugEntityVisual::render() {
    if (!enabled) return;
    // Stub implementation - would render entity visuals
}

void DebugEntityVisual::clear() {
    entities.clear();
}

void DebugEntityVisual::setEnabled(bool enabled) {
    this->enabled = enabled;
}

bool DebugEntityVisual::isEnabled() const {
    return enabled;
}

// DebugProfilerVisual implementation
DebugProfilerVisual& DebugProfilerVisual::getInstance() {
    static DebugProfilerVisual instance;
    return instance;
}

DebugProfilerVisual::DebugProfilerVisual() : visible(false) {
}

DebugProfilerVisual::~DebugProfilerVisual() {
}

void DebugProfilerVisual::update() {
    if (!visible) return;
    // Stub implementation
}

void DebugProfilerVisual::render(const Math::Vector2D& position, const Math::Vector2D& size) {
    if (!visible) return;
    (void)position;
    (void)size;
    // Stub implementation
}

void DebugProfilerVisual::setVisible(bool visible) {
    this->visible = visible;
}

bool DebugProfilerVisual::isVisible() const {
    return visible;
}

// DebugMemoryVisual implementation
DebugMemoryVisual& DebugMemoryVisual::getInstance() {
    static DebugMemoryVisual instance;
    return instance;
}

DebugMemoryVisual::DebugMemoryVisual()
    : memoryGraph("Memory", 120), visible(false) {
}

DebugMemoryVisual::~DebugMemoryVisual() {
}

void DebugMemoryVisual::update() {
    if (!visible) return;
    // Stub implementation - would add memory sample
}

void DebugMemoryVisual::render(const Math::Vector2D& position, const Math::Vector2D& size) {
    if (!visible) return;
    memoryGraph.render(position, size);
}

void DebugMemoryVisual::setVisible(bool visible) {
    this->visible = visible;
}

bool DebugMemoryVisual::isVisible() const {
    return visible;
}

// DebugVisualizationManager implementation
DebugVisualizationManager& DebugVisualizationManager::getInstance() {
    static DebugVisualizationManager instance;
    return instance;
}

DebugVisualizationManager::DebugVisualizationManager() : globalEnabled(true) {
}

DebugVisualizationManager::~DebugVisualizationManager() {
}

void DebugVisualizationManager::update(float deltaTime) {
    if (!globalEnabled) return;
    
    DebugDraw::getInstance().update(deltaTime);
    DebugStatsDisplay::getInstance().update(deltaTime);
}

void DebugVisualizationManager::render() {
    if (!globalEnabled) return;
    
    DebugDraw::getInstance().render();
    DebugTextOverlay::getInstance().render();
    DebugConsoleVisual::getInstance().render();
    DebugStatsDisplay::getInstance().render();
}

DebugDraw& DebugVisualizationManager::getDebugDraw() {
    return DebugDraw::getInstance();
}

DebugTextOverlay& DebugVisualizationManager::getTextOverlay() {
    return DebugTextOverlay::getInstance();
}

DebugConsoleVisual& DebugVisualizationManager::getConsole() {
    return DebugConsoleVisual::getInstance();
}

DebugStatsDisplay& DebugVisualizationManager::getStatsDisplay() {
    return DebugStatsDisplay::getInstance();
}

void DebugVisualizationManager::setGlobalEnabled(bool enabled) {
    globalEnabled = enabled;
}

bool DebugVisualizationManager::isGlobalEnabled() const {
    return globalEnabled;
}

void DebugVisualizationManager::toggleDebugMode() {
    globalEnabled = !globalEnabled;
}

// DebugGizmo implementation
DebugGizmo::DebugGizmo()
    : type(GizmoType::Translation), scale(1.0f), isDragging(false) {
}

DebugGizmo::~DebugGizmo() {
}

void DebugGizmo::setType(GizmoType type) {
    this->type = type;
}

GizmoType DebugGizmo::getType() const {
    return type;
}

void DebugGizmo::setPosition(const Math::Vector2D& position) {
    this->position = position;
}

Math::Vector2D DebugGizmo::getPosition() const {
    return position;
}

void DebugGizmo::setScale(float scale) {
    this->scale = scale;
}

float DebugGizmo::getScale() const {
    return scale;
}

void DebugGizmo::render() {
    // Stub implementation - would render gizmo
}

bool DebugGizmo::handleInput(const Math::Vector2D& mousePos) {
    (void)mousePos;
    // Stub implementation - would handle input
    return false;
}

// DebugAxisVisual implementation
void DebugAxisVisual::drawAxes(const Math::Vector2D& origin, float size) {
    auto& draw = DebugDraw::getInstance();
    draw.drawLine(origin, origin + Math::Vector2D(size, 0), Graphics::Color(255, 0, 0));
    draw.drawLine(origin, origin + Math::Vector2D(0, size), Graphics::Color(0, 255, 0));
}

void DebugAxisVisual::drawCompass(const Math::Vector2D& position, float size) {
    drawAxes(position, size);
}

// DebugShapeVisual implementation
void DebugShapeVisual::drawPoint(const Math::Vector2D& point, float size,
                                const Graphics::Color& color) {
    DebugDraw::getInstance().drawCross(point, size, color);
}

void DebugShapeVisual::drawLineStrip(const std::vector<Math::Vector2D>& points,
                                    const Graphics::Color& color, float thickness) {
    if (points.size() < 2) return;
    
    auto& draw = DebugDraw::getInstance();
    for (size_t i = 0; i < points.size() - 1; i++) {
        draw.drawLine(points[i], points[i + 1], color, thickness);
    }
}

void DebugShapeVisual::drawBezierCurve(const Math::Vector2D& p0,
                                      const Math::Vector2D& p1,
                                      const Math::Vector2D& p2,
                                      const Math::Vector2D& p3,
                                      const Graphics::Color& color) {
    // Stub implementation - would draw bezier curve
    (void)p0; (void)p1; (void)p2; (void)p3; (void)color;
}

} // namespace Debug
} // namespace JJM
