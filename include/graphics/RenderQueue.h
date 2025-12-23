#ifndef JJM_RENDER_QUEUE_H
#define JJM_RENDER_QUEUE_H

#include <vector>
#include <functional>
#include <map>

namespace JJM {
namespace Graphics {

enum class RenderLayer { Background, World, Transparent, UI, Overlay };

struct RenderCommand {
    RenderLayer layer;
    int priority;
    float distance;
    std::function<void()> execute;
    
    bool operator<(const RenderCommand& other) const {
        if (layer != other.layer) return layer < other.layer;
        if (priority != other.priority) return priority > other.priority;
        return distance > other.distance;
    }
};

class RenderQueue {
public:
    static RenderQueue& getInstance();
    
    void submit(RenderLayer layer, std::function<void()> command, int priority = 0, float distance = 0.0f);
    void sort();
    void execute();
    void clear();
    
    int getCommandCount() const;
    int getCommandCount(RenderLayer layer) const;

private:
    RenderQueue();
    ~RenderQueue();
    
    std::vector<RenderCommand> commands;
    bool sorted;
};

} // namespace Graphics
} // namespace JJM

#endif
