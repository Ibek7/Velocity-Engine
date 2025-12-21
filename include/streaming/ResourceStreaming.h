#ifndef JJM_RESOURCE_STREAMING_H
#define JJM_RESOURCE_STREAMING_H

#include <string>
#include <vector>
#include <memory>
#include <queue>
#include <functional>

namespace JJM {
namespace Streaming {

enum class StreamPriority { Low, Normal, High, Critical };
enum class StreamState { Pending, Loading, Loaded, Failed };

class StreamRequest {
public:
    std::string resourcePath;
    StreamPriority priority;
    StreamState state;
    std::function<void(bool)> callback;
    
    StreamRequest(const std::string& path, StreamPriority prio);
};

class ResourceStreamer {
public:
    static ResourceStreamer& getInstance();
    
    void update();
    void streamResource(const std::string& path, StreamPriority priority,
                       std::function<void(bool)> callback);
    void cancelStream(const std::string& path);
    void setMaxConcurrent(int max);
    
private:
    ResourceStreamer();
    ~ResourceStreamer();
    std::priority_queue<std::shared_ptr<StreamRequest>> requests;
    int maxConcurrent;
};

} // namespace Streaming
} // namespace JJM

#endif
