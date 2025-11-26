#include "graphics/TextureAtlas.h"
#include <fstream>
#include <sstream>
#include <iostream>

namespace JJM {
namespace Graphics {

// TextureAtlas implementation
TextureAtlas::TextureAtlas()
    : atlasTexture(nullptr), width(0), height(0) {
}

TextureAtlas::~TextureAtlas() {
    clear();
}

bool TextureAtlas::loadFromFile(const std::string& imagePath, const std::string& dataPath, Renderer* renderer) {
    atlasTexture = new Texture();
    if (!atlasTexture->load(imagePath, renderer)) {
        delete atlasTexture;
        atlasTexture = nullptr;
        return false;
    }
    
    width = atlasTexture->getWidth();
    height = atlasTexture->getHeight();
    
    std::ifstream file(dataPath);
    if (!file.is_open()) {
        std::cerr << "Failed to load atlas data: " << dataPath << std::endl;
        return false;
    }
    
    std::string line;
    while (std::getline(file, line)) {
        std::istringstream iss(line);
        std::string name;
        int x, y, w, h;
        
        if (iss >> name >> x >> y >> w >> h) {
            addRegion(name, x, y, w, h);
        }
    }
    
    file.close();
    return true;
}

bool TextureAtlas::create(int width, int height, Renderer* renderer) {
    this->width = width;
    this->height = height;
    
    atlasTexture = new Texture();
    // Would need to create blank texture here
    return true;
}

void TextureAtlas::addRegion(const std::string& name, int x, int y, int width, int height) {
    regions[name] = AtlasRegion(name, x, y, width, height);
}

void TextureAtlas::addRegion(const std::string& name, const SDL_Rect& rect) {
    addRegion(name, rect.x, rect.y, rect.w, rect.h);
}

AtlasRegion* TextureAtlas::getRegion(const std::string& name) {
    auto it = regions.find(name);
    if (it != regions.end()) {
        return &it->second;
    }
    return nullptr;
}

const AtlasRegion* TextureAtlas::getRegion(const std::string& name) const {
    auto it = regions.find(name);
    if (it != regions.end()) {
        return &it->second;
    }
    return nullptr;
}

void TextureAtlas::render(Renderer* renderer, const std::string& regionName,
                         const Math::Vector2D& position) {
    const AtlasRegion* region = getRegion(regionName);
    if (region) {
        renderRegion(renderer, *region, position);
    }
}

void TextureAtlas::render(Renderer* renderer, const std::string& regionName,
                         const Math::Vector2D& position, const Math::Vector2D& size) {
    const AtlasRegion* region = getRegion(regionName);
    if (region) {
        renderRegion(renderer, *region, position, size);
    }
}

void TextureAtlas::renderRegion(Renderer* renderer, const AtlasRegion& region,
                               const Math::Vector2D& position) {
    renderRegion(renderer, region, position,
                Math::Vector2D(region.width, region.height));
}

void TextureAtlas::renderRegion(Renderer* renderer, const AtlasRegion& region,
                               const Math::Vector2D& position, const Math::Vector2D& size) {
    if (!atlasTexture || !renderer) return;
    
    SDL_Rect srcRect = region.toRect();
    SDL_Rect dstRect = {
        static_cast<int>(position.x),
        static_cast<int>(position.y),
        static_cast<int>(size.x),
        static_cast<int>(size.y)
    };
    
    SDL_RenderCopy(renderer->getSDLRenderer(),
                  atlasTexture->getSDLTexture(),
                  &srcRect, &dstRect);
}

bool TextureAtlas::hasRegion(const std::string& name) const {
    return regions.find(name) != regions.end();
}

std::vector<std::string> TextureAtlas::getRegionNames() const {
    std::vector<std::string> names;
    for (const auto& pair : regions) {
        names.push_back(pair.first);
    }
    return names;
}

void TextureAtlas::clear() {
    regions.clear();
    if (atlasTexture) {
        delete atlasTexture;
        atlasTexture = nullptr;
    }
    width = 0;
    height = 0;
}

// TextureAtlasPacker implementation
TextureAtlasPacker::TextureAtlasPacker(int width, int height)
    : atlasWidth(width), atlasHeight(height) {
    root = new PackNode(0, 0, width, height);
}

TextureAtlasPacker::~TextureAtlasPacker() {
    delete root;
}

bool TextureAtlasPacker::pack(const std::string& name, int width, int height, AtlasRegion& outRegion) {
    PackNode* node = insert(root, width, height);
    
    if (node) {
        outRegion = AtlasRegion(name, node->x, node->y, width, height);
        return true;
    }
    
    return false;
}

void TextureAtlasPacker::clear() {
    delete root;
    root = new PackNode(0, 0, atlasWidth, atlasHeight);
}

TextureAtlasPacker::PackNode* TextureAtlasPacker::insert(PackNode* node, int width, int height) {
    if (!node) return nullptr;
    
    if (node->left || node->right) {
        PackNode* newNode = insert(node->left, width, height);
        if (newNode) return newNode;
        return insert(node->right, width, height);
    }
    
    if (node->filled) return nullptr;
    
    if (width > node->width || height > node->height) {
        return nullptr;
    }
    
    if (width == node->width && height == node->height) {
        node->filled = true;
        return node;
    }
    
    int dw = node->width - width;
    int dh = node->height - height;
    
    if (dw > dh) {
        node->left = new PackNode(node->x, node->y, width, node->height);
        node->right = new PackNode(node->x + width, node->y,
                                  node->width - width, node->height);
    } else {
        node->left = new PackNode(node->x, node->y, node->width, height);
        node->right = new PackNode(node->x, node->y + height,
                                  node->width, node->height - height);
    }
    
    return insert(node->left, width, height);
}

} // namespace Graphics
} // namespace JJM
