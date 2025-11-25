#ifndef DEBUG_UTILS_H
#define DEBUG_UTILS_H

#include "math/Vector2D.h"
#include "graphics/Renderer.h"
#include "graphics/Color.h"
#include <string>
#include <vector>
#include <chrono>

namespace JJM {
namespace Debug {

class DebugDraw {
public:
    static void drawPoint(Graphics::Renderer* renderer, const Math::Vector2D& pos, const Graphics::Color& color);
    static void drawLine(Graphics::Renderer* renderer, const Math::Vector2D& start, const Math::Vector2D& end, const Graphics::Color& color);
    static void drawCircle(Graphics::Renderer* renderer, const Math::Vector2D& center, float radius, const Graphics::Color& color);
    static void drawRect(Graphics::Renderer* renderer, const Math::Vector2D& pos, const Math::Vector2D& size, const Graphics::Color& color);
    static void drawCross(Graphics::Renderer* renderer, const Math::Vector2D& pos, float size, const Graphics::Color& color);
    static void drawGrid(Graphics::Renderer* renderer, int cellSize, const Graphics::Color& color);
};

class Profiler {
private:
    struct ProfileEntry {
        std::string name;
        std::chrono::high_resolution_clock::time_point startTime;
        double duration;
        bool active;
    };
    
    std::vector<ProfileEntry> entries;
    static Profiler* instance;
    
    Profiler();
    
public:
    ~Profiler();
    
    static Profiler* getInstance();
    static void destroy();
    
    void begin(const std::string& name);
    void end(const std::string& name);
    double getDuration(const std::string& name) const;
    void reset();
    void printResults() const;
    
    Profiler(const Profiler&) = delete;
    Profiler& operator=(const Profiler&) = delete;
};

class ScopedProfile {
private:
    std::string name;
    
public:
    ScopedProfile(const std::string& profileName);
    ~ScopedProfile();
};

class Logger {
public:
    enum class Level {
        Debug,
        Info,
        Warning,
        Error
    };
    
private:
    static Level currentLevel;
    
public:
    static void setLevel(Level level);
    static void debug(const std::string& message);
    static void info(const std::string& message);
    static void warning(const std::string& message);
    static void error(const std::string& message);
    static void log(Level level, const std::string& message);
};

class FPSCounter {
private:
    int frameCount;
    double elapsedTime;
    double fps;
    double updateInterval;
    
public:
    FPSCounter(double interval = 1.0);
    
    void update(double deltaTime);
    double getFPS() const { return fps; }
    void reset();
};

} // namespace Debug
} // namespace JJM

#endif // DEBUG_UTILS_H
