#include "graphics/ScreenshotSystem.h"
#include <ctime>
#include <sstream>
#include <iomanip>
#include <fstream>
#include <iostream>

namespace JJM {
namespace Graphics {

ScreenshotSystem& ScreenshotSystem::getInstance() {
    static ScreenshotSystem instance;
    return instance;
}

ScreenshotSystem::ScreenshotSystem() 
    : format("png"), quality(90), directory("screenshots/") {
}

ScreenshotSystem::~ScreenshotSystem() {
}

bool ScreenshotSystem::captureScreenshot(const std::string& filename) {
    // Stub implementation - would integrate with rendering system
    std::cout << "Capturing screenshot to: " << filename << std::endl;
    
    bool success = true; // Placeholder
    
    if (captureCallback) {
        captureCallback(success, filename);
    }
    
    return success;
}

bool ScreenshotSystem::captureScreenshot(const std::string& filename, int x, int y, 
                                        int width, int height) {
    std::cout << "Capturing screenshot region (" << x << "," << y << "," 
              << width << "x" << height << ") to: " << filename << std::endl;
    
    bool success = true; // Placeholder
    
    if (captureCallback) {
        captureCallback(success, filename);
    }
    
    return success;
}

void ScreenshotSystem::setFormat(const std::string& fmt) {
    if (fmt == "png" || fmt == "jpg" || fmt == "jpeg") {
        format = fmt;
    }
}

void ScreenshotSystem::setQuality(int q) {
    quality = q < 0 ? 0 : (q > 100 ? 100 : q);
}

void ScreenshotSystem::setDirectory(const std::string& dir) {
    directory = dir;
}

std::string ScreenshotSystem::generateFilename() const {
    std::time_t now = std::time(nullptr);
    std::tm* timeinfo = std::localtime(&now);
    
    std::ostringstream oss;
    oss << directory << "screenshot_" 
        << std::put_time(timeinfo, "%Y%m%d_%H%M%S")
        << "." << format;
    
    return oss.str();
}

void ScreenshotSystem::setCaptureCallback(CaptureCallback callback) {
    captureCallback = callback;
}

bool ScreenshotSystem::saveAsPNG(const std::string& filename, unsigned char* pixels,
                                 int width, int height) {
    std::cout << "Saving PNG: " << filename << " (" << width << "x" << height << ")" << std::endl;
    (void)pixels;
    return true;
}

bool ScreenshotSystem::saveAsJPEG(const std::string& filename, unsigned char* pixels,
                                  int width, int height, int qual) {
    std::cout << "Saving JPEG: " << filename << " (" << width << "x" << height 
              << ") quality=" << qual << std::endl;
    (void)pixels;
    return true;
}

} // namespace Graphics
} // namespace JJM
