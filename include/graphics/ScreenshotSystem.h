#ifndef JJM_SCREENSHOT_SYSTEM_H
#define JJM_SCREENSHOT_SYSTEM_H

#include <string>
#include <functional>

namespace JJM {
namespace Graphics {

class ScreenshotSystem {
public:
    static ScreenshotSystem& getInstance();
    
    bool captureScreenshot(const std::string& filename);
    bool captureScreenshot(const std::string& filename, int x, int y, int width, int height);
    
    void setFormat(const std::string& format);
    void setQuality(int quality);
    void setDirectory(const std::string& dir);
    
    std::string generateFilename() const;
    
    using CaptureCallback = std::function<void(bool, const std::string&)>;
    void setCaptureCallback(CaptureCallback callback);

private:
    ScreenshotSystem();
    ~ScreenshotSystem();
    
    bool saveAsPNG(const std::string& filename, unsigned char* pixels,
                   int width, int height);
    bool saveAsJPEG(const std::string& filename, unsigned char* pixels,
                    int width, int height, int quality);
    
    std::string format;
    int quality;
    std::string directory;
    CaptureCallback captureCallback;
};

} // namespace Graphics
} // namespace JJM

#endif
