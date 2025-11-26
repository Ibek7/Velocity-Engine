#ifndef TEXTURE_ATLAS_H
#define TEXTURE_ATLAS_H

#include "graphics/Texture.h"
#include "math/Vector2D.h"
#include <string>
#include <map>
#include <vector>
#include <SDL2/SDL.h>

namespace JJM {
namespace Graphics {

struct AtlasRegion {
    std::string name;
    int x, y;
    int width, height;
    
    AtlasRegion() : x(0), y(0), width(0), height(0) {}
    AtlasRegion(const std::string& name, int x, int y, int w, int h)
        : name(name), x(x), y(y), width(w), height(h) {}
    
    SDL_Rect toRect() const {
        return {x, y, width, height};
    }
};

class TextureAtlas {
private:
    Texture* atlasTexture;
    std::map<std::string, AtlasRegion> regions;
    int width;
    int height;
    
public:
    TextureAtlas();
    ~TextureAtlas();
    
    bool loadFromFile(const std::string& imagePath, const std::string& dataPath, Renderer* renderer);
    bool create(int width, int height, Renderer* renderer);
    
    void addRegion(const std::string& name, int x, int y, int width, int height);
    void addRegion(const std::string& name, const SDL_Rect& rect);
    
    AtlasRegion* getRegion(const std::string& name);
    const AtlasRegion* getRegion(const std::string& name) const;
    
    void render(Renderer* renderer, const std::string& regionName,
               const Math::Vector2D& position);
    void render(Renderer* renderer, const std::string& regionName,
               const Math::Vector2D& position, const Math::Vector2D& size);
    
    void renderRegion(Renderer* renderer, const AtlasRegion& region,
                     const Math::Vector2D& position);
    void renderRegion(Renderer* renderer, const AtlasRegion& region,
                     const Math::Vector2D& position, const Math::Vector2D& size);
    
    Texture* getTexture() { return atlasTexture; }
    const Texture* getTexture() const { return atlasTexture; }
    
    bool hasRegion(const std::string& name) const;
    std::vector<std::string> getRegionNames() const;
    
    int getWidth() const { return width; }
    int getHeight() const { return height; }
    int getRegionCount() const { return static_cast<int>(regions.size()); }
    
    void clear();
};

class TextureAtlasPacker {
private:
    struct PackNode {
        int x, y, width, height;
        bool filled;
        PackNode* left;
        PackNode* right;
        
        PackNode(int x, int y, int w, int h)
            : x(x), y(y), width(w), height(h),
              filled(false), left(nullptr), right(nullptr) {}
        
        ~PackNode() {
            delete left;
            delete right;
        }
    };
    
    PackNode* root;
    int atlasWidth;
    int atlasHeight;
    
public:
    TextureAtlasPacker(int width, int height);
    ~TextureAtlasPacker();
    
    bool pack(const std::string& name, int width, int height, AtlasRegion& outRegion);
    void clear();
    
private:
    PackNode* insert(PackNode* node, int width, int height);
};

} // namespace Graphics
} // namespace JJM

#endif // TEXTURE_ATLAS_H
