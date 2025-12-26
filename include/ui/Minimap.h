#pragma once

#include <string>
#include <vector>
#include <functional>

// Minimap rendering system
namespace Engine {

struct MinimapIcon {
    int entityId;
    float x;
    float y;
    unsigned int color;
    float size;
    int iconType;
    bool isVisible;
};

class MinimapRenderer {
public:
    static MinimapRenderer& getInstance();

    // Minimap setup
    void initialize(int width, int height);
    void shutdown();
    
    // Rendering
    void render(float cameraX, float cameraY, float zoom);
    void setViewport(int x, int y, int width, int height);
    
    // Icons
    void addIcon(int entityId, float x, float y, unsigned int color, 
                float size = 4.0f, int iconType = 0);
    void updateIcon(int entityId, float x, float y);
    void removeIcon(int entityId);
    void clearIcons();
    
    // Display options
    void setZoom(float zoom) { m_zoom = zoom; }
    float getZoom() const { return m_zoom; }
    void setRotation(float angle) { m_rotation = angle; }
    float getRotation() const { return m_rotation; }
    void setOpacity(float opacity);
    void setBackgroundColor(unsigned int color) { m_backgroundColor = color; }
    
    // Visibility
    void show() { m_isVisible = true; }
    void hide() { m_isVisible = false; }
    void toggle() { m_isVisible = !m_isVisible; }
    bool isVisible() const { return m_isVisible; }
    
    // Map bounds
    void setWorldBounds(float minX, float minY, float maxX, float maxY);
    void enableFogOfWar(bool enable) { m_fogOfWarEnabled = enable; }
    void revealArea(float x, float y, float radius);
    
    // Border and frame
    void setBorderColor(unsigned int color) { m_borderColor = color; }
    void setBorderWidth(float width) { m_borderWidth = width; }
    void setFrameTexture(const std::string& texturePath);

private:
    MinimapRenderer();
    MinimapRenderer(const MinimapRenderer&) = delete;
    MinimapRenderer& operator=(const MinimapRenderer&) = delete;

    void renderBackground();
    void renderIcons(float cameraX, float cameraY);
    void renderBorder();
    void renderFogOfWar();
    
    MinimapIcon* getIcon(int entityId);
    void worldToMinimap(float worldX, float worldY, float& minimapX, float& minimapY);

    std::vector<MinimapIcon> m_icons;
    
    int m_width;
    int m_height;
    int m_viewportX;
    int m_viewportY;
    int m_viewportWidth;
    int m_viewportHeight;
    
    float m_zoom;
    float m_rotation;
    float m_opacity;
    unsigned int m_backgroundColor;
    unsigned int m_borderColor;
    float m_borderWidth;
    
    bool m_isVisible;
    bool m_fogOfWarEnabled;
    
    float m_worldMinX;
    float m_worldMinY;
    float m_worldMaxX;
    float m_worldMaxY;
    
    std::vector<bool> m_revealedAreas;
};

} // namespace Engine
