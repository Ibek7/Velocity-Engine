#include "streaming/ResourceStreaming.h"

namespace JJM {
namespace Streaming {

StreamRequest::StreamRequest(const std::string& path, StreamPriority prio)
    : resourcePath(path), priority(prio), state(StreamState::Pending) {
}

ResourceStreamer& ResourceStreamer::getInstance() {
    static ResourceStreamer instance;
    return instance;
}

ResourceStreamer::ResourceStreamer() : maxConcurrent(4) {
}

ResourceStreamer::~ResourceStreamer() {
}

void ResourceStreamer::update() {
    // Process streaming requests
}

void ResourceStreamer::streamResource(const std::string& path, StreamPriority priority,
                                     std::function<void(bool)> callback) {
    auto request = std::make_shared<StreamRequest>(path, priority);
    request->callback = callback;
    requests.push(request);
}

void ResourceStreamer::cancelStream(const std::string& path) {
    (void)path;
}

void ResourceStreamer::setMaxConcurrent(int max) {
    maxConcurrent = max;
}

} // namespace Streaming
} // namespace JJM
