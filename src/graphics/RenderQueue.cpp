#include "graphics/RenderQueue.h"
#include <algorithm>
#include <iostream>

namespace JJM {
namespace Graphics {

RenderQueue& RenderQueue::getInstance() {
    static RenderQueue instance;
    return instance;
}

RenderQueue::RenderQueue() : sorted(false) {
}

RenderQueue::~RenderQueue() {
}

void RenderQueue::submit(RenderLayer layer, std::function<void()> command, int priority, float distance) {
    RenderCommand cmd;
    cmd.layer = layer;
    cmd.priority = priority;
    cmd.distance = distance;
    cmd.execute = command;
    
    commands.push_back(cmd);
    sorted = false;
}

void RenderQueue::sort() {
    if (!sorted) {
        std::sort(commands.begin(), commands.end());
        sorted = true;
    }
}

void RenderQueue::execute() {
    sort();
    
    for (auto& cmd : commands) {
        if (cmd.execute) {
            cmd.execute();
        }
    }
}

void RenderQueue::clear() {
    commands.clear();
    sorted = false;
}

int RenderQueue::getCommandCount() const {
    return static_cast<int>(commands.size());
}

int RenderQueue::getCommandCount(RenderLayer layer) const {
    int count = 0;
    for (const auto& cmd : commands) {
        if (cmd.layer == layer) {
            count++;
        }
    }
    return count;
}

} // namespace Graphics
} // namespace JJM
