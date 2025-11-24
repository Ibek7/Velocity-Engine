#ifndef RESOURCE_MANAGER_H
#define RESOURCE_MANAGER_H

#include "graphics/Texture.h"
#include <unordered_map>
#include <memory>
#include <string>

namespace JJM {
namespace Core {

class ResourceManager {
private:
    std::unordered_map<std::string, std::shared_ptr<Graphics::Texture>> textures;
    Graphics::Renderer* renderer;
    
    static ResourceManager* instance;
    
    ResourceManager();

public:
    ~ResourceManager();
    
    static ResourceManager* getInstance();
    static void destroyInstance();
    
    void setRenderer(Graphics::Renderer* r) { renderer = r; }
    
    // Texture management
    std::shared_ptr<Graphics::Texture> loadTexture(const std::string& id, const std::string& filePath);
    std::shared_ptr<Graphics::Texture> getTexture(const std::string& id);
    void unloadTexture(const std::string& id);
    void unloadAllTextures();
    
    // Resource queries
    bool hasTexture(const std::string& id) const;
    size_t getTextureCount() const { return textures.size(); }
    
    // Clear all resources
    void clear();
};

} // namespace Core
} // namespace JJM

#endif // RESOURCE_MANAGER_H
