#include "tilemap/Tilemap.h"

namespace JJM {
namespace Tilemap {

// TileLayer implementation
TileLayer::TileLayer(const std::string& layerName, int w, int h)
    : width(w), height(h), name(layerName), visible(true), opacity(1.0f) {
    tiles.resize(height, std::vector<int>(width, 0));
}

void TileLayer::setTile(int x, int y, int tileId) {
    if (x >= 0 && x < width && y >= 0 && y < height) {
        tiles[y][x] = tileId;
    }
}

int TileLayer::getTile(int x, int y) const {
    if (x >= 0 && x < width && y >= 0 && y < height) {
        return tiles[y][x];
    }
    return 0;
}

// Tilemap implementation
Tilemap::Tilemap(int tileW, int tileH)
    : tilesetTexture(nullptr), tileWidth(tileW), tileHeight(tileH), tilesPerRow(0) {}

Tilemap::~Tilemap() {}

void Tilemap::loadTileset(Graphics::Texture* texture, int tilesPerRow) {
    tilesetTexture = texture;
    this->tilesPerRow = tilesPerRow;
}

void Tilemap::addLayer(const std::string& name, int width, int height) {
    layers.emplace_back(name, width, height);
}

void Tilemap::setTile(int layer, int x, int y, int tileId) {
    if (layer >= 0 && layer < static_cast<int>(layers.size())) {
        layers[layer].setTile(x, y, tileId);
    }
}

int Tilemap::getTile(int layer, int x, int y) const {
    if (layer >= 0 && layer < static_cast<int>(layers.size())) {
        return layers[layer].getTile(x, y);
    }
    return 0;
}

void Tilemap::render(Graphics::Renderer* renderer, const Math::Vector2D& offset) {
    if (!tilesetTexture) return;
    
    for (const auto& layer : layers) {
        if (!layer.isVisible()) continue;
        
        for (int y = 0; y < layer.getHeight(); ++y) {
            for (int x = 0; x < layer.getWidth(); ++x) {
                int tileId = layer.getTile(x, y);
                if (tileId == 0) continue;
                
                // Calculate source position in tileset
                int srcX = ((tileId - 1) % tilesPerRow) * tileWidth;
                int srcY = ((tileId - 1) / tilesPerRow) * tileHeight;
                
                // Calculate destination position
                Math::Vector2D destPos(x * tileWidth + offset.x, y * tileHeight + offset.y);
                Math::Vector2D destSize(tileWidth, tileHeight);
                
                // Render tile (simplified - would use source rect in full implementation)
                renderer->drawRect(destPos, destSize, Graphics::Color(100, 100, 100), true);
            }
        }
    }
}

bool Tilemap::isTileSolid(int tileId) const {
    if (tileId > 0 && tileId <= static_cast<int>(tileSet.size())) {
        return tileSet[tileId - 1].solid;
    }
    return false;
}

Math::Vector2D Tilemap::getTilePosition(int x, int y) const {
    return Math::Vector2D(x * tileWidth, y * tileHeight);
}

} // namespace Tilemap
} // namespace JJM
