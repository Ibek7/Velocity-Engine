#include "utils/DebugUtils.h"
#include <iostream>
#include <iomanip>
#include <algorithm>

namespace JJM {
namespace Debug {

// DebugDraw implementation
void DebugDraw::drawPoint(Graphics::Renderer* renderer, const Math::Vector2D& pos, const Graphics::Color& color) {
    renderer->drawPoint(pos, color);
}

void DebugDraw::drawLine(Graphics::Renderer* renderer, const Math::Vector2D& start, const Math::Vector2D& end, const Graphics::Color& color) {
    renderer->drawLine(start, end, color);
}

void DebugDraw::drawCircle(Graphics::Renderer* renderer, const Math::Vector2D& center, float radius, const Graphics::Color& color) {
    renderer->drawCircle(center, radius, color, false);
}

void DebugDraw::drawRect(Graphics::Renderer* renderer, const Math::Vector2D& pos, const Math::Vector2D& size, const Graphics::Color& color) {
    renderer->drawRect(pos, size, color, false);
}

void DebugDraw::drawCross(Graphics::Renderer* renderer, const Math::Vector2D& pos, float size, const Graphics::Color& color) {
    renderer->drawLine(pos - Math::Vector2D(size, 0), pos + Math::Vector2D(size, 0), color);
    renderer->drawLine(pos - Math::Vector2D(0, size), pos + Math::Vector2D(0, size), color);
}

void DebugDraw::drawGrid(Graphics::Renderer* renderer, int cellSize, const Graphics::Color& color) {
    int width = renderer->getWindowWidth();
    int height = renderer->getWindowHeight();
    
    for (int x = 0; x < width; x += cellSize) {
        renderer->drawLine(Math::Vector2D(x, 0), Math::Vector2D(x, height), color);
    }
    
    for (int y = 0; y < height; y += cellSize) {
        renderer->drawLine(Math::Vector2D(0, y), Math::Vector2D(width, y), color);
    }
}

// Profiler implementation
Profiler* Profiler::instance = nullptr;

Profiler::Profiler() {}

Profiler::~Profiler() {}

Profiler* Profiler::getInstance() {
    if (!instance) {
        instance = new Profiler();
    }
    return instance;
}

void Profiler::destroy() {
    if (instance) {
        delete instance;
        instance = nullptr;
    }
}

void Profiler::begin(const std::string& name) {
    auto it = std::find_if(entries.begin(), entries.end(),
        [&name](const ProfileEntry& entry) { return entry.name == name; });
    
    if (it != entries.end()) {
        it->startTime = std::chrono::high_resolution_clock::now();
        it->active = true;
    } else {
        ProfileEntry entry;
        entry.name = name;
        entry.startTime = std::chrono::high_resolution_clock::now();
        entry.duration = 0;
        entry.active = true;
        entries.push_back(entry);
    }
}

void Profiler::end(const std::string& name) {
    auto now = std::chrono::high_resolution_clock::now();
    
    auto it = std::find_if(entries.begin(), entries.end(),
        [&name](const ProfileEntry& entry) { return entry.name == name && entry.active; });
    
    if (it != entries.end()) {
        std::chrono::duration<double, std::milli> duration = now - it->startTime;
        it->duration = duration.count();
        it->active = false;
    }
}

double Profiler::getDuration(const std::string& name) const {
    auto it = std::find_if(entries.begin(), entries.end(),
        [&name](const ProfileEntry& entry) { return entry.name == name; });
    
    if (it != entries.end()) {
        return it->duration;
    }
    return 0.0;
}

void Profiler::reset() {
    entries.clear();
}

void Profiler::printResults() const {
    std::cout << "\n=== Profiler Results ===" << std::endl;
    for (const auto& entry : entries) {
        std::cout << std::setw(30) << std::left << entry.name << ": " 
                  << std::fixed << std::setprecision(3) << entry.duration << " ms" << std::endl;
    }
    std::cout << "========================\n" << std::endl;
}

// ScopedProfile implementation
ScopedProfile::ScopedProfile(const std::string& profileName) : name(profileName) {
    Profiler::getInstance()->begin(name);
}

ScopedProfile::~ScopedProfile() {
    Profiler::getInstance()->end(name);
}

// Logger implementation
Logger::Level Logger::currentLevel = Logger::Level::Info;

void Logger::setLevel(Level level) {
    currentLevel = level;
}

void Logger::debug(const std::string& message) {
    log(Level::Debug, message);
}

void Logger::info(const std::string& message) {
    log(Level::Info, message);
}

void Logger::warning(const std::string& message) {
    log(Level::Warning, message);
}

void Logger::error(const std::string& message) {
    log(Level::Error, message);
}

void Logger::log(Level level, const std::string& message) {
    if (level < currentLevel) return;
    
    std::string prefix;
    switch (level) {
        case Level::Debug:   prefix = "[DEBUG] "; break;
        case Level::Info:    prefix = "[INFO] "; break;
        case Level::Warning: prefix = "[WARN] "; break;
        case Level::Error:   prefix = "[ERROR] "; break;
    }
    
    std::cout << prefix << message << std::endl;
}

// FPSCounter implementation
FPSCounter::FPSCounter(double interval)
    : frameCount(0), elapsedTime(0), fps(0), updateInterval(interval) {}

void FPSCounter::update(double deltaTime) {
    frameCount++;
    elapsedTime += deltaTime;
    
    if (elapsedTime >= updateInterval) {
        fps = frameCount / elapsedTime;
        frameCount = 0;
        elapsedTime = 0;
    }
}

void FPSCounter::reset() {
    frameCount = 0;
    elapsedTime = 0;
    fps = 0;
}

} // namespace Debug
} // namespace JJM
