#pragma once

#include "graphics/Texture.h"
#include "math/Vector2D.h"
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>

namespace JJM {
namespace Graphics {

struct AtlasRegion {
    std::string name;
    int x;
    int y;
    int width;
    int height;
    float uvMinX;
    float uvMinY;
    float uvMaxX;
    float uvMaxY;
};

struct PackedTexture {
    std::string path;
    int width;
    int height;
    AtlasRegion region;
};

enum class PackingAlgorithm {
    ShelfBestFit,
    ShelfFirstFit,
    MaxRects,
    Guillotine,
    Simple
};

class TextureAtlasPacker {
public:
    TextureAtlasPacker();
    explicit TextureAtlasPacker(int width, int height);
    ~TextureAtlasPacker();
    
    void setAtlasSize(int width, int height) {
        atlasWidth = width;
        atlasHeight = height;
    }
    
    void getAtlasSize(int& width, int& height) const {
        width = atlasWidth;
        height = atlasHeight;
    }
    
    void setAlgorithm(PackingAlgorithm algo) { algorithm = algo; }
    PackingAlgorithm getAlgorithm() const { return algorithm; }
    
    void setPadding(int padding) { this->padding = padding; }
    int getPadding() const { return padding; }
    
    void setAllowRotation(bool allow) { allowRotation = allow; }
    bool isRotationAllowed() const { return allowRotation; }
    
    bool addTexture(const std::string& name, int width, int height);
    bool addTexture(const std::string& name, Texture* texture);
    
    bool pack();
    void clear();
    
    const std::vector<PackedTexture>& getPackedTextures() const {
        return packedTextures;
    }
    
    AtlasRegion* getRegion(const std::string& name);
    
    float getEfficiency() const;

private:
    int atlasWidth;
    int atlasHeight;
    int padding;
    bool allowRotation;
    PackingAlgorithm algorithm;
    
    struct TextureInput {
        std::string name;
        int width;
        int height;
        bool rotated;
    };
    
    std::vector<TextureInput> inputTextures;
    std::vector<PackedTexture> packedTextures;
    
    bool packShelfBestFit();
    bool packShelfFirstFit();
    bool packMaxRects();
    bool packGuillotine();
    bool packSimple();
    
    void sortTexturesBySize();
};

class ShelfPacker {
public:
    ShelfPacker(int width, int height);
    ~ShelfPacker();
    
    struct Shelf {
        int y;
        int height;
        int usedWidth;
    };
    
    bool pack(int width, int height, int& x, int& y);
    void clear();

private:
    int atlasWidth;
    int atlasHeight;
    std::vector<Shelf> shelves;
    
    bool addToShelf(Shelf& shelf, int width, int height, int& x, int& y);
    void createNewShelf(int height);
};

class MaxRectsPacker {
public:
    MaxRectsPacker(int width, int height);
    ~MaxRectsPacker();
    
    struct Rect {
        int x;
        int y;
        int width;
        int height;
    };
    
    bool pack(int width, int height, int& x, int& y);
    void clear();

private:
    int atlasWidth;
    int atlasHeight;
    std::vector<Rect> freeRects;
    
    int scoreRect(int width, int height, const Rect& rect);
    bool splitRect(const Rect& rect, int width, int height);
    void pruneRects();
};

class GuillotinePacker {
public:
    GuillotinePacker(int width, int height);
    ~GuillotinePacker();
    
    struct Node {
        int x;
        int y;
        int width;
        int height;
    };
    
    bool pack(int width, int height, int& x, int& y);
    void clear();

private:
    int atlasWidth;
    int atlasHeight;
    std::vector<Node> freeNodes;
    
    int findBestNode(int width, int height);
    void splitNode(int index, int width, int height);
};

class TextureAtlasBuilder {
public:
    TextureAtlasBuilder();
    ~TextureAtlasBuilder();
    
    void setAtlasSize(int width, int height) { atlasWidth = width; atlasHeight = height; }
    void setAlgorithm(PackingAlgorithm algo) { algorithm = algo; }
    void setPadding(int padding) { this->padding = padding; }
    
    void addTexture(const std::string& name, const std::string& path);
    void addTexture(const std::string& name, Texture* texture);
    
    bool build();
    
    Texture* getAtlasTexture() const { return atlasTexture.get(); }
    const std::unordered_map<std::string, AtlasRegion>& getRegions() const {
        return regions;
    }
    
    bool saveAtlas(const std::string& imagePath, const std::string& dataPath);
    bool loadAtlas(const std::string& imagePath, const std::string& dataPath);

private:
    int atlasWidth;
    int atlasHeight;
    int padding;
    PackingAlgorithm algorithm;
    
    struct TextureData {
        std::string name;
        std::string path;
        Texture* texture;
        int width;
        int height;
    };
    
    std::vector<TextureData> textures;
    std::unordered_map<std::string, AtlasRegion> regions;
    std::unique_ptr<Texture> atlasTexture;
    
    bool packTextures();
    void blitTexture(Texture* src, int x, int y);
};

class TextureAtlasManager {
public:
    static TextureAtlasManager& getInstance();
    
    void loadAtlas(const std::string& name, const std::string& imagePath, 
                   const std::string& dataPath);
    
    void unloadAtlas(const std::string& name);
    
    Texture* getAtlasTexture(const std::string& atlasName);
    AtlasRegion* getRegion(const std::string& atlasName, const std::string& regionName);
    
    void clear();

private:
    TextureAtlasManager() {}
    ~TextureAtlasManager() {}
    TextureAtlasManager(const TextureAtlasManager&) = delete;
    TextureAtlasManager& operator=(const TextureAtlasManager&) = delete;
    
    struct AtlasData {
        std::unique_ptr<Texture> texture;
        std::unordered_map<std::string, AtlasRegion> regions;
    };
    
    std::unordered_map<std::string, AtlasData> atlases;
};

} // namespace Graphics
} // namespace JJM
