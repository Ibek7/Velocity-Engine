#ifndef TILEMAP_H
#define TILEMAP_H

#include "math/Vector2D.h"
#include "graphics/Renderer.h"
#include "graphics/Texture.h"
#include <vector>
#include <string>

namespace JJM {
namespace Tilemap {

struct Tile {
    int id;
    bool solid;
    int textureIndex;
    
    Tile() : id(0), solid(false), textureIndex(0) {}
    Tile(int i, bool s, int tex) : id(i), solid(s), textureIndex(tex) {}
};

class TileLayer {
private:
    std::vector<std::vector<int>> tiles;
    int width, height;
    std::string name;
    bool visible;
    float opacity;
    
public:
    TileLayer(const std::string& layerName, int w, int h);
    
    void setTile(int x, int y, int tileId);
    int getTile(int x, int y) const;
    
    int getWidth() const { return width; }
    int getHeight() const { return height; }
    void setVisible(bool vis) { visible = vis; }
    bool isVisible() const { return visible; }
};

class Tilemap {
private:
    std::vector<TileLayer> layers;
    std::vector<Tile> tileSet;
    Graphics::Texture* tilesetTexture;
    
    int tileWidth, tileHeight;
    int tilesPerRow;
    
public:
    Tilemap(int tileW, int tileH);
    ~Tilemap();
    
    void loadTileset(Graphics::Texture* texture, int tilesPerRow);
    void addLayer(const std::string& name, int width, int height);
    
    void setTile(int layer, int x, int y, int tileId);
    int getTile(int layer, int x, int y) const;
    
    void render(Graphics::Renderer* renderer, const Math::Vector2D& offset);
    
    bool isTileSolid(int tileId) const;
    Math::Vector2D getTilePosition(int x, int y) const;
    
    int getLayerCount() const { return static_cast<int>(layers.size()); }
    int getTileWidth() const { return tileWidth; }
    int getTileHeight() const { return tileHeight; }
};

} // namespace Tilemap
} // namespace JJM

#endif // TILEMAP_H
