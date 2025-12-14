#include "graphics/TextureAtlasPacker.h"
#include <algorithm>
#include <cmath>
#include <fstream>

namespace JJM {
namespace Graphics {

// TextureAtlasPacker implementation
TextureAtlasPacker::TextureAtlasPacker()
    : atlasWidth(1024), atlasHeight(1024), padding(1), 
      allowRotation(false), algorithm(PackingAlgorithm::ShelfBestFit) {}

TextureAtlasPacker::TextureAtlasPacker(int width, int height)
    : atlasWidth(width), atlasHeight(height), padding(1),
      allowRotation(false), algorithm(PackingAlgorithm::ShelfBestFit) {}

TextureAtlasPacker::~TextureAtlasPacker() {}

bool TextureAtlasPacker::addTexture(const std::string& name, int width, int height) {
    TextureInput input;
    input.name = name;
    input.width = width;
    input.height = height;
    input.rotated = false;
    inputTextures.push_back(input);
    return true;
}

bool TextureAtlasPacker::addTexture(const std::string& name, Texture* texture) {
    if (!texture) return false;
    return addTexture(name, texture->getWidth(), texture->getHeight());
}

bool TextureAtlasPacker::pack() {
    if (inputTextures.empty()) return false;
    
    sortTexturesBySize();
    
    switch (algorithm) {
        case PackingAlgorithm::ShelfBestFit:
            return packShelfBestFit();
        case PackingAlgorithm::ShelfFirstFit:
            return packShelfFirstFit();
        case PackingAlgorithm::MaxRects:
            return packMaxRects();
        case PackingAlgorithm::Guillotine:
            return packGuillotine();
        case PackingAlgorithm::Simple:
            return packSimple();
    }
    
    return false;
}

void TextureAtlasPacker::clear() {
    inputTextures.clear();
    packedTextures.clear();
}

AtlasRegion* TextureAtlasPacker::getRegion(const std::string& name) {
    for (auto& packed : packedTextures) {
        if (packed.region.name == name) {
            return &packed.region;
        }
    }
    return nullptr;
}

float TextureAtlasPacker::getEfficiency() const {
    int usedArea = 0;
    for (const auto& packed : packedTextures) {
        usedArea += packed.width * packed.height;
    }
    int totalArea = atlasWidth * atlasHeight;
    return totalArea > 0 ? static_cast<float>(usedArea) / totalArea : 0.0f;
}

void TextureAtlasPacker::sortTexturesBySize() {
    std::sort(inputTextures.begin(), inputTextures.end(),
        [](const TextureInput& a, const TextureInput& b) {
            int areaA = a.width * a.height;
            int areaB = b.width * b.height;
            return areaA > areaB;
        });
}

bool TextureAtlasPacker::packShelfBestFit() {
    ShelfPacker packer(atlasWidth, atlasHeight);
    
    for (const auto& input : inputTextures) {
        int x, y;
        int width = input.width + padding * 2;
        int height = input.height + padding * 2;
        
        if (packer.pack(width, height, x, y)) {
            PackedTexture packed;
            packed.path = input.name;
            packed.width = input.width;
            packed.height = input.height;
            packed.region.name = input.name;
            packed.region.x = x + padding;
            packed.region.y = y + padding;
            packed.region.width = input.width;
            packed.region.height = input.height;
            packed.region.uvMinX = static_cast<float>(packed.region.x) / atlasWidth;
            packed.region.uvMinY = static_cast<float>(packed.region.y) / atlasHeight;
            packed.region.uvMaxX = static_cast<float>(packed.region.x + packed.region.width) / atlasWidth;
            packed.region.uvMaxY = static_cast<float>(packed.region.y + packed.region.height) / atlasHeight;
            packedTextures.push_back(packed);
        } else {
            return false;
        }
    }
    
    return true;
}

bool TextureAtlasPacker::packShelfFirstFit() {
    return packShelfBestFit();
}

bool TextureAtlasPacker::packMaxRects() {
    MaxRectsPacker packer(atlasWidth, atlasHeight);
    
    for (const auto& input : inputTextures) {
        int x, y;
        int width = input.width + padding * 2;
        int height = input.height + padding * 2;
        
        if (packer.pack(width, height, x, y)) {
            PackedTexture packed;
            packed.path = input.name;
            packed.width = input.width;
            packed.height = input.height;
            packed.region.name = input.name;
            packed.region.x = x + padding;
            packed.region.y = y + padding;
            packed.region.width = input.width;
            packed.region.height = input.height;
            packed.region.uvMinX = static_cast<float>(packed.region.x) / atlasWidth;
            packed.region.uvMinY = static_cast<float>(packed.region.y) / atlasHeight;
            packed.region.uvMaxX = static_cast<float>(packed.region.x + packed.region.width) / atlasWidth;
            packed.region.uvMaxY = static_cast<float>(packed.region.y + packed.region.height) / atlasHeight;
            packedTextures.push_back(packed);
        } else {
            return false;
        }
    }
    
    return true;
}

bool TextureAtlasPacker::packGuillotine() {
    GuillotinePacker packer(atlasWidth, atlasHeight);
    
    for (const auto& input : inputTextures) {
        int x, y;
        int width = input.width + padding * 2;
        int height = input.height + padding * 2;
        
        if (packer.pack(width, height, x, y)) {
            PackedTexture packed;
            packed.path = input.name;
            packed.width = input.width;
            packed.height = input.height;
            packed.region.name = input.name;
            packed.region.x = x + padding;
            packed.region.y = y + padding;
            packed.region.width = input.width;
            packed.region.height = input.height;
            packed.region.uvMinX = static_cast<float>(packed.region.x) / atlasWidth;
            packed.region.uvMinY = static_cast<float>(packed.region.y) / atlasHeight;
            packed.region.uvMaxX = static_cast<float>(packed.region.x + packed.region.width) / atlasWidth;
            packed.region.uvMaxY = static_cast<float>(packed.region.y + packed.region.height) / atlasHeight;
            packedTextures.push_back(packed);
        } else {
            return false;
        }
    }
    
    return true;
}

bool TextureAtlasPacker::packSimple() {
    int x = 0, y = 0;
    int rowHeight = 0;
    
    for (const auto& input : inputTextures) {
        int width = input.width + padding * 2;
        int height = input.height + padding * 2;
        
        if (x + width > atlasWidth) {
            x = 0;
            y += rowHeight;
            rowHeight = 0;
        }
        
        if (y + height > atlasHeight) {
            return false;
        }
        
        PackedTexture packed;
        packed.path = input.name;
        packed.width = input.width;
        packed.height = input.height;
        packed.region.name = input.name;
        packed.region.x = x + padding;
        packed.region.y = y + padding;
        packed.region.width = input.width;
        packed.region.height = input.height;
        packed.region.uvMinX = static_cast<float>(packed.region.x) / atlasWidth;
        packed.region.uvMinY = static_cast<float>(packed.region.y) / atlasHeight;
        packed.region.uvMaxX = static_cast<float>(packed.region.x + packed.region.width) / atlasWidth;
        packed.region.uvMaxY = static_cast<float>(packed.region.y + packed.region.height) / atlasHeight;
        packedTextures.push_back(packed);
        
        x += width;
        rowHeight = std::max(rowHeight, height);
    }
    
    return true;
}

// ShelfPacker implementation
ShelfPacker::ShelfPacker(int width, int height)
    : atlasWidth(width), atlasHeight(height) {
    createNewShelf(0);
}

ShelfPacker::~ShelfPacker() {}

bool ShelfPacker::pack(int width, int height, int& x, int& y) {
    for (auto& shelf : shelves) {
        if (addToShelf(shelf, width, height, x, y)) {
            return true;
        }
    }
    
    // Create new shelf
    if (!shelves.empty()) {
        int newY = shelves.back().y + shelves.back().height;
        if (newY + height <= atlasHeight) {
            createNewShelf(newY);
            return addToShelf(shelves.back(), width, height, x, y);
        }
    }
    
    return false;
}

void ShelfPacker::clear() {
    shelves.clear();
    createNewShelf(0);
}

bool ShelfPacker::addToShelf(Shelf& shelf, int width, int height, int& x, int& y) {
    if (shelf.usedWidth + width <= atlasWidth && height <= shelf.height) {
        x = shelf.usedWidth;
        y = shelf.y;
        shelf.usedWidth += width;
        return true;
    }
    return false;
}

void ShelfPacker::createNewShelf(int height) {
    Shelf shelf;
    shelf.y = height;
    shelf.height = 0;
    shelf.usedWidth = 0;
    shelves.push_back(shelf);
}

// MaxRectsPacker implementation
MaxRectsPacker::MaxRectsPacker(int width, int height)
    : atlasWidth(width), atlasHeight(height) {
    Rect rect = {0, 0, width, height};
    freeRects.push_back(rect);
}

MaxRectsPacker::~MaxRectsPacker() {}

bool MaxRectsPacker::pack(int width, int height, int& x, int& y) {
    int bestScore = INT_MAX;
    int bestIndex = -1;
    
    for (size_t i = 0; i < freeRects.size(); ++i) {
        int score = scoreRect(width, height, freeRects[i]);
        if (score < bestScore) {
            bestScore = score;
            bestIndex = static_cast<int>(i);
        }
    }
    
    if (bestIndex >= 0) {
        x = freeRects[bestIndex].x;
        y = freeRects[bestIndex].y;
        splitRect(freeRects[bestIndex], width, height);
        freeRects.erase(freeRects.begin() + bestIndex);
        pruneRects();
        return true;
    }
    
    return false;
}

void MaxRectsPacker::clear() {
    freeRects.clear();
    Rect rect = {0, 0, atlasWidth, atlasHeight};
    freeRects.push_back(rect);
}

int MaxRectsPacker::scoreRect(int width, int height, const Rect& rect) {
    if (width > rect.width || height > rect.height) {
        return INT_MAX;
    }
    
    int leftoverX = rect.width - width;
    int leftoverY = rect.height - height;
    return leftoverX * leftoverY;
}

bool MaxRectsPacker::splitRect(const Rect& rect, int width, int height) {
    if (rect.width > width) {
        Rect newRect = {rect.x + width, rect.y, rect.width - width, height};
        freeRects.push_back(newRect);
    }
    
    if (rect.height > height) {
        Rect newRect = {rect.x, rect.y + height, rect.width, rect.height - height};
        freeRects.push_back(newRect);
    }
    
    return true;
}

void MaxRectsPacker::pruneRects() {
    for (size_t i = 0; i < freeRects.size(); ++i) {
        for (size_t j = i + 1; j < freeRects.size();) {
            if (freeRects[i].x >= freeRects[j].x &&
                freeRects[i].y >= freeRects[j].y &&
                freeRects[i].x + freeRects[i].width <= freeRects[j].x + freeRects[j].width &&
                freeRects[i].y + freeRects[i].height <= freeRects[j].y + freeRects[j].height) {
                freeRects.erase(freeRects.begin() + i);
                break;
            }
            ++j;
        }
    }
}

// GuillotinePacker implementation
GuillotinePacker::GuillotinePacker(int width, int height)
    : atlasWidth(width), atlasHeight(height) {
    Node node = {0, 0, width, height};
    freeNodes.push_back(node);
}

GuillotinePacker::~GuillotinePacker() {}

bool GuillotinePacker::pack(int width, int height, int& x, int& y) {
    int index = findBestNode(width, height);
    if (index >= 0) {
        x = freeNodes[index].x;
        y = freeNodes[index].y;
        splitNode(index, width, height);
        return true;
    }
    return false;
}

void GuillotinePacker::clear() {
    freeNodes.clear();
    Node node = {0, 0, atlasWidth, atlasHeight};
    freeNodes.push_back(node);
}

int GuillotinePacker::findBestNode(int width, int height) {
    int bestIndex = -1;
    int bestArea = INT_MAX;
    
    for (size_t i = 0; i < freeNodes.size(); ++i) {
        if (freeNodes[i].width >= width && freeNodes[i].height >= height) {
            int area = freeNodes[i].width * freeNodes[i].height;
            if (area < bestArea) {
                bestArea = area;
                bestIndex = static_cast<int>(i);
            }
        }
    }
    
    return bestIndex;
}

void GuillotinePacker::splitNode(int index, int width, int height) {
    Node node = freeNodes[index];
    freeNodes.erase(freeNodes.begin() + index);
    
    if (node.width > width) {
        Node right = {node.x + width, node.y, node.width - width, height};
        freeNodes.push_back(right);
    }
    
    if (node.height > height) {
        Node bottom = {node.x, node.y + height, node.width, node.height - height};
        freeNodes.push_back(bottom);
    }
}

// TextureAtlasBuilder implementation
TextureAtlasBuilder::TextureAtlasBuilder()
    : atlasWidth(1024), atlasHeight(1024), padding(1),
      algorithm(PackingAlgorithm::ShelfBestFit) {}

TextureAtlasBuilder::~TextureAtlasBuilder() {}

void TextureAtlasBuilder::addTexture(const std::string& name, const std::string& path) {
    TextureData data;
    data.name = name;
    data.path = path;
    data.texture = nullptr;
    textures.push_back(data);
}

void TextureAtlasBuilder::addTexture(const std::string& name, Texture* texture) {
    if (!texture) return;
    
    TextureData data;
    data.name = name;
    data.texture = texture;
    data.width = texture->getWidth();
    data.height = texture->getHeight();
    textures.push_back(data);
}

bool TextureAtlasBuilder::build() {
    return packTextures();
}

bool TextureAtlasBuilder::packTextures() {
    TextureAtlasPacker packer(atlasWidth, atlasHeight);
    packer.setAlgorithm(algorithm);
    packer.setPadding(padding);
    
    for (const auto& tex : textures) {
        if (tex.texture) {
            packer.addTexture(tex.name, tex.texture);
        }
    }
    
    if (!packer.pack()) {
        return false;
    }
    
    const auto& packed = packer.getPackedTextures();
    for (const auto& p : packed) {
        regions[p.region.name] = p.region;
    }
    
    return true;
}

void TextureAtlasBuilder::blitTexture(Texture* src, int x, int y) {
    if (!src || !atlasTexture) return;
    // Blit source texture to atlas at position
}

bool TextureAtlasBuilder::saveAtlas(const std::string& imagePath, const std::string& dataPath) {
    std::ofstream file(dataPath);
    if (!file.is_open()) return false;
    
    file << atlasWidth << " " << atlasHeight << "\n";
    for (const auto& pair : regions) {
        const auto& region = pair.second;
        file << region.name << " "
             << region.x << " " << region.y << " "
             << region.width << " " << region.height << "\n";
    }
    
    return true;
}

bool TextureAtlasBuilder::loadAtlas(const std::string& imagePath, const std::string& dataPath) {
    std::ifstream file(dataPath);
    if (!file.is_open()) return false;
    
    file >> atlasWidth >> atlasHeight;
    
    std::string name;
    while (file >> name) {
        AtlasRegion region;
        region.name = name;
        file >> region.x >> region.y >> region.width >> region.height;
        region.uvMinX = static_cast<float>(region.x) / atlasWidth;
        region.uvMinY = static_cast<float>(region.y) / atlasHeight;
        region.uvMaxX = static_cast<float>(region.x + region.width) / atlasWidth;
        region.uvMaxY = static_cast<float>(region.y + region.height) / atlasHeight;
        regions[name] = region;
    }
    
    return true;
}

// TextureAtlasManager implementation
TextureAtlasManager& TextureAtlasManager::getInstance() {
    static TextureAtlasManager instance;
    return instance;
}

void TextureAtlasManager::loadAtlas(const std::string& name, const std::string& imagePath,
                                   const std::string& dataPath) {
    TextureAtlasBuilder builder;
    if (builder.loadAtlas(imagePath, dataPath)) {
        AtlasData data;
        data.texture = std::unique_ptr<Texture>(builder.getAtlasTexture());
        data.regions = builder.getRegions();
        atlases[name] = std::move(data);
    }
}

void TextureAtlasManager::unloadAtlas(const std::string& name) {
    atlases.erase(name);
}

Texture* TextureAtlasManager::getAtlasTexture(const std::string& atlasName) {
    auto it = atlases.find(atlasName);
    if (it != atlases.end()) {
        return it->second.texture.get();
    }
    return nullptr;
}

AtlasRegion* TextureAtlasManager::getRegion(const std::string& atlasName, const std::string& regionName) {
    auto it = atlases.find(atlasName);
    if (it != atlases.end()) {
        auto regionIt = it->second.regions.find(regionName);
        if (regionIt != it->second.regions.end()) {
            return &regionIt->second;
        }
    }
    return nullptr;
}

void TextureAtlasManager::clear() {
    atlases.clear();
}

} // namespace Graphics
} // namespace JJM
