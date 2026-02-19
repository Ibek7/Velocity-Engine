#ifndef JJM_DEBUG_VISUALIZATION_H
#define JJM_DEBUG_VISUALIZATION_H

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "graphics/Color.h"
#include "math/Vector2D.h"

namespace JJM {
namespace Debug {

/**
 * @brief Debug draw primitive types
 */
enum class DebugPrimitiveType { Line, Circle, Rectangle, Polygon, Text, Arrow, Cross, Grid };

/**
 * @brief Debug draw primitive
 */
struct DebugPrimitive {
    DebugPrimitiveType type;
    std::vector<Math::Vector2D> points;
    Graphics::Color color;
    float thickness;
    bool filled;
    std::string text;
    float duration;
    float timeRemaining;
};

/**
 * @brief Debug draw system
 */
class DebugDraw {
   public:
    static DebugDraw& getInstance();

    DebugDraw(const DebugDraw&) = delete;
    DebugDraw& operator=(const DebugDraw&) = delete;

    // 2D Drawing primitives
    void drawLine(const Math::Vector2D& start, const Math::Vector2D& end,
                  const Graphics::Color& color = Graphics::Color(255, 255, 255),
                  float thickness = 1.0f, float duration = 0.0f);

    void drawCircle(const Math::Vector2D& center, float radius,
                    const Graphics::Color& color = Graphics::Color(255, 255, 255),
                    float thickness = 1.0f, bool filled = false, float duration = 0.0f);

    void drawRectangle(const Math::Vector2D& min, const Math::Vector2D& max,
                       const Graphics::Color& color = Graphics::Color(255, 255, 255),
                       float thickness = 1.0f, bool filled = false, float duration = 0.0f);

    void drawPolygon(const std::vector<Math::Vector2D>& points,
                     const Graphics::Color& color = Graphics::Color(255, 255, 255),
                     float thickness = 1.0f, bool filled = false, float duration = 0.0f);

    void drawText(const Math::Vector2D& position, const std::string& text,
                  const Graphics::Color& color = Graphics::Color(255, 255, 255),
                  float duration = 0.0f);

    void drawArrow(const Math::Vector2D& start, const Math::Vector2D& end,
                   const Graphics::Color& color = Graphics::Color(255, 255, 255),
                   float thickness = 1.0f, float duration = 0.0f);

    void drawCross(const Math::Vector2D& center, float size,
                   const Graphics::Color& color = Graphics::Color(255, 255, 255),
                   float thickness = 1.0f, float duration = 0.0f);

    void drawGrid(const Math::Vector2D& origin, float cellSize, int width, int height,
                  const Graphics::Color& color = Graphics::Color(128, 128, 128),
                  float thickness = 1.0f);

    // Physics debug drawing
    void drawAABB(const Math::Vector2D& min, const Math::Vector2D& max,
                  const Graphics::Color& color = Graphics::Color(0, 255, 0));

    void drawRay(const Math::Vector2D& origin, const Math::Vector2D& direction, float length,
                 const Graphics::Color& color = Graphics::Color(255, 0, 0));

    void drawVelocity(const Math::Vector2D& position, const Math::Vector2D& velocity,
                      const Graphics::Color& color = Graphics::Color(0, 0, 255));

    // Collision debug helpers
    void drawCollisionNormal(const Math::Vector2D& point, const Math::Vector2D& normal,
                             const Graphics::Color& color = Graphics::Color(255, 255, 0),
                             float length = 20.0f, float duration = 0.0f);

    void drawCollisionPoint(const Math::Vector2D& point,
                            const Graphics::Color& color = Graphics::Color(255, 0, 255),
                            float radius = 5.0f, float duration = 0.0f);

    void drawContactManifold(const Math::Vector2D& point1, const Math::Vector2D& point2,
                             const Math::Vector2D& normal, float penetration,
                             const Graphics::Color& color = Graphics::Color(255, 128, 0),
                             float duration = 0.0f);

    // Physics body visualization
    void drawRigidbody(const Math::Vector2D& position, const Math::Vector2D& velocity,
                       const Math::Vector2D& force, float mass,
                       const Graphics::Color& color = Graphics::Color(0, 255, 255),
                       float duration = 0.0f);

    void drawBounds(const Math::Vector2D& center, const Math::Vector2D& extents,
                    float rotation = 0.0f,
                    const Graphics::Color& color = Graphics::Color(255, 255, 0),
                    float duration = 0.0f);

    // Transform visualization
    void drawTransform(const Math::Vector2D& position, float rotation, float scale = 1.0f,
                       float axisLength = 50.0f, float duration = 0.0f);

    // Common debug shapes (convenience methods)
    void drawSphere2D(const Math::Vector2D& center, float radius,
                      const Graphics::Color& color = Graphics::Color(255, 255, 255),
                      float duration = 0.0f);

    void drawCapsule2D(const Math::Vector2D& start, const Math::Vector2D& end, float radius,
                       const Graphics::Color& color = Graphics::Color(255, 255, 255),
                       float duration = 0.0f);

    void drawPath(const std::vector<Math::Vector2D>& waypoints,
                  const Graphics::Color& color = Graphics::Color(255, 255, 0),
                  bool showWaypoints = true, float duration = 0.0f);

    void drawBezierCurve(const Math::Vector2D& p0, const Math::Vector2D& p1,
                         const Math::Vector2D& p2, const Math::Vector2D& p3,
                         const Graphics::Color& color = Graphics::Color(255, 255, 255),
                         int segments = 20, float duration = 0.0f);

    void drawFrustum(const Math::Vector2D& position, float rotation, float fov, float nearDist,
                     float farDist, const Graphics::Color& color = Graphics::Color(0, 255, 255),
                     float duration = 0.0f);

    void drawBasis(const Math::Vector2D& origin, const Math::Vector2D& xAxis,
                   const Math::Vector2D& yAxis, float length = 50.0f, float duration = 0.0f);

    // Update and render
    void update(float deltaTime);
    void render();
    void clear();

    // Settings
    void setEnabled(bool enabled);
    bool isEnabled() const;

    void setDepthTest(bool enabled);
    bool getDepthTest() const;

   private:
    DebugDraw();
    ~DebugDraw();

    std::vector<DebugPrimitive> primitives;
    bool enabled;
    bool depthTest;
};

/**
 * @brief Debug text overlay system
 */
class DebugTextOverlay {
   public:
    static DebugTextOverlay& getInstance();

    DebugTextOverlay(const DebugTextOverlay&) = delete;
    DebugTextOverlay& operator=(const DebugTextOverlay&) = delete;

    void addText(const std::string& text, const Math::Vector2D& position = Math::Vector2D(10, 10),
                 const Graphics::Color& color = Graphics::Color(255, 255, 255));

    void addTextFormat(const Math::Vector2D& position, const Graphics::Color& color,
                       const char* format, ...);

    void addPersistentText(const std::string& key, const std::string& text,
                           const Math::Vector2D& position = Math::Vector2D(10, 10),
                           const Graphics::Color& color = Graphics::Color(255, 255, 255));

    void removePersistentText(const std::string& key);

    void render();
    void clear();
    void clearPersistent();

    void setEnabled(bool enabled);
    bool isEnabled() const;

   private:
    DebugTextOverlay();
    ~DebugTextOverlay();

    struct TextEntry {
        std::string text;
        Math::Vector2D position;
        Graphics::Color color;
    };

    std::vector<TextEntry> frameText;
    std::unordered_map<std::string, TextEntry> persistentText;
    bool enabled;
};

/**
 * @brief Performance graph overlay
 */
class PerformanceGraph {
   public:
    PerformanceGraph(const std::string& name, size_t maxSamples = 120);
    ~PerformanceGraph();

    void addSample(float value);
    void render(const Math::Vector2D& position, const Math::Vector2D& size);

    void setRange(float min, float max);
    void setAutoScale(bool enabled);

    void setColor(const Graphics::Color& color);

    float getMin() const;
    float getMax() const;
    float getAverage() const;

    void clear();

   private:
    std::string name;
    std::vector<float> samples;
    size_t maxSamples;
    float rangeMin;
    float rangeMax;
    bool autoScale;
    Graphics::Color color;
};

/**
 * @brief Debug console
 */
class DebugConsoleVisual {
   public:
    static DebugConsoleVisual& getInstance();

    DebugConsoleVisual(const DebugConsoleVisual&) = delete;
    DebugConsoleVisual& operator=(const DebugConsoleVisual&) = delete;

    void log(const std::string& message);
    void logWarning(const std::string& message);
    void logError(const std::string& message);

    void render();
    void clear();

    void setVisible(bool visible);
    bool isVisible() const;

    void setMaxLines(size_t maxLines);
    size_t getMaxLines() const;

   private:
    DebugConsoleVisual();
    ~DebugConsoleVisual();

    struct LogEntry {
        std::string message;
        Graphics::Color color;
        std::chrono::system_clock::time_point timestamp;
    };

    std::vector<LogEntry> entries;
    size_t maxLines;
    bool visible;
};

/**
 * @brief Debug stats display
 */
class DebugStatsDisplay {
   public:
    static DebugStatsDisplay& getInstance();

    DebugStatsDisplay(const DebugStatsDisplay&) = delete;
    DebugStatsDisplay& operator=(const DebugStatsDisplay&) = delete;

    void update(float deltaTime);
    void render();

    void setStat(const std::string& name, const std::string& value);
    void removeStat(const std::string& name);

    void setVisible(bool visible);
    bool isVisible() const;

    void setPosition(const Math::Vector2D& position);
    Math::Vector2D getPosition() const;

   private:
    DebugStatsDisplay();
    ~DebugStatsDisplay();

    std::unordered_map<std::string, std::string> stats;
    Math::Vector2D position;
    bool visible;
};

/**
 * @brief Debug camera visualization
 */
class DebugCameraVisual {
   public:
    static DebugCameraVisual& getInstance();

    DebugCameraVisual(const DebugCameraVisual&) = delete;
    DebugCameraVisual& operator=(const DebugCameraVisual&) = delete;

    void visualizeCamera(const Math::Vector2D& position, float zoom);
    void visualizeFrustum(const Math::Vector2D& min, const Math::Vector2D& max);

    void render();

    void setEnabled(bool enabled);
    bool isEnabled() const;

   private:
    DebugCameraVisual();
    ~DebugCameraVisual();

    bool enabled;
};

/**
 * @brief Debug entity visualization
 */
class DebugEntityVisual {
   public:
    static DebugEntityVisual& getInstance();

    DebugEntityVisual(const DebugEntityVisual&) = delete;
    DebugEntityVisual& operator=(const DebugEntityVisual&) = delete;

    void visualizeEntity(uint32_t entityId, const Math::Vector2D& position);
    void visualizeEntityBounds(uint32_t entityId, const Math::Vector2D& min,
                               const Math::Vector2D& max);

    void render();
    void clear();

    void setEnabled(bool enabled);
    bool isEnabled() const;

   private:
    DebugEntityVisual();
    ~DebugEntityVisual();

    struct EntityVisual {
        uint32_t id;
        Math::Vector2D position;
        Math::Vector2D boundsMin;
        Math::Vector2D boundsMax;
    };

    std::vector<EntityVisual> entities;
    bool enabled;
};

/**
 * @brief Debug profiler visualization
 */
class DebugProfilerVisual {
   public:
    static DebugProfilerVisual& getInstance();

    DebugProfilerVisual(const DebugProfilerVisual&) = delete;
    DebugProfilerVisual& operator=(const DebugProfilerVisual&) = delete;

    void update();
    void render(const Math::Vector2D& position, const Math::Vector2D& size);

    void setVisible(bool visible);
    bool isVisible() const;

   private:
    DebugProfilerVisual();
    ~DebugProfilerVisual();

    bool visible;
};

/**
 * @brief Debug memory visualization
 */
class DebugMemoryVisual {
   public:
    static DebugMemoryVisual& getInstance();

    DebugMemoryVisual(const DebugMemoryVisual&) = delete;
    DebugMemoryVisual& operator=(const DebugMemoryVisual&) = delete;

    void update();
    void render(const Math::Vector2D& position, const Math::Vector2D& size);

    void setVisible(bool visible);
    bool isVisible() const;

   private:
    DebugMemoryVisual();
    ~DebugMemoryVisual();

    PerformanceGraph memoryGraph;
    bool visible;
};

/**
 * @brief Debug visualization manager
 */
class DebugVisualizationManager {
   public:
    static DebugVisualizationManager& getInstance();

    DebugVisualizationManager(const DebugVisualizationManager&) = delete;
    DebugVisualizationManager& operator=(const DebugVisualizationManager&) = delete;

    void update(float deltaTime);
    void render();

    DebugDraw& getDebugDraw();
    DebugTextOverlay& getTextOverlay();
    DebugConsoleVisual& getConsole();
    DebugStatsDisplay& getStatsDisplay();

    void setGlobalEnabled(bool enabled);
    bool isGlobalEnabled() const;

    void toggleDebugMode();

   private:
    DebugVisualizationManager();
    ~DebugVisualizationManager();

    bool globalEnabled;
};

/**
 * @brief Debug gizmo types
 */
enum class GizmoType { Translation, Rotation, Scale };

/**
 * @brief Debug gizmo
 */
class DebugGizmo {
   public:
    DebugGizmo();
    ~DebugGizmo();

    void setType(GizmoType type);
    GizmoType getType() const;

    void setPosition(const Math::Vector2D& position);
    Math::Vector2D getPosition() const;

    void setScale(float scale);
    float getScale() const;

    void render();

    bool handleInput(const Math::Vector2D& mousePos);

   private:
    GizmoType type;
    Math::Vector2D position;
    float scale;
    bool isDragging;
};

/**
 * @brief Debug axis visualization
 */
class DebugAxisVisual {
   public:
    static void drawAxes(const Math::Vector2D& origin, float size);
    static void drawCompass(const Math::Vector2D& position, float size);

   private:
    DebugAxisVisual() = delete;
};

/**
 * @brief Debug shape visualization helpers
 */
class DebugShapeVisual {
   public:
    static void drawPoint(const Math::Vector2D& point, float size = 5.0f,
                          const Graphics::Color& color = Graphics::Color(255, 255, 255));

    static void drawLineStrip(const std::vector<Math::Vector2D>& points,
                              const Graphics::Color& color = Graphics::Color(255, 255, 255),
                              float thickness = 1.0f);

    static void drawBezierCurve(const Math::Vector2D& p0, const Math::Vector2D& p1,
                                const Math::Vector2D& p2, const Math::Vector2D& p3,
                                const Graphics::Color& color = Graphics::Color(255, 255, 255));

   private:
    DebugShapeVisual() = delete;
};

/**
 * @brief Macros for debug drawing
 */
#ifdef JJM_DEBUG
#define JJM_DEBUG_DRAW_LINE(start, end, color) \
    JJM::Debug::DebugDraw::getInstance().drawLine(start, end, color)
#define JJM_DEBUG_DRAW_CIRCLE(center, radius, color) \
    JJM::Debug::DebugDraw::getInstance().drawCircle(center, radius, color)
#define JJM_DEBUG_DRAW_TEXT(pos, text, color) \
    JJM::Debug::DebugDraw::getInstance().drawText(pos, text, color)
#else
#define JJM_DEBUG_DRAW_LINE(start, end, color)
#define JJM_DEBUG_DRAW_CIRCLE(center, radius, color)
#define JJM_DEBUG_DRAW_TEXT(pos, text, color)
#endif

}  // namespace Debug
}  // namespace JJM

#endif  // JJM_DEBUG_VISUALIZATION_H
