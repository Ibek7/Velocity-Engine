#pragma once

#include <string>
#include <vector>
#include <memory>
#include <functional>

// Minimap rendering system for 2D navigation
namespace Engine {

enum class MinimapIconType {
    Player,
    Enemy,
    Ally,
    Objective,
    Waypoint,
    Item,
    Vehicle,
    Building,
    Custom
};

struct MinimapIcon {
    MinimapIconType type;
    float worldX;
    float worldZ;
    float rotation;
    float scale;
    float color[4];
    std::string customTexture;
    bool visible;
};

enum class MinimapMode {
    Fixed,        // Fixed rotation (north always up)
    Rotating      // Rotates with player
};

class MinimapRenderer {
public:
    static MinimapRenderer& getInstance();

    // Configuration
    void setPosition(float screenX, float screenY);
    void setSize(float width, float height);
    void setZoom(float zoom);
    void setMode(MinimapMode mode) { m_mode = mode; }
    void setPlayerPosition(float worldX, float worldZ);
    void setPlayerRotation(float degrees);
    
    // Icons
    void addIcon(const MinimapIcon& icon);
    void removeIcon(MinimapIconType type, float worldX, float worldZ);
    void clearIcons();
    void updateIconPosition(MinimapIconType type, float oldX, float oldZ, float newX, float newZ);
    
    // Rendering
    void render();
    void setBackgroundTexture(const std::string& path);
    void setBorderColor(float r, float g, float b, float a);
    void setMaskTexture(const std::string& path); // For circular/custom shapes
    
    // World bounds
    void setWorldBounds(float minX, float minZ, float maxX, float maxZ);
    
    // Visibility
    void setVisible(bool visible) { m_visible = visible; }
    bool isVisible() const { return m_visible; }

private:
    MinimapRenderer();
    MinimapRenderer(const MinimapRenderer&) = delete;
    MinimapRenderer& operator=(const MinimapRenderer&) = delete;

    void renderBackground();
    void renderIcons();
    void renderBorder();
    void worldToMinimap(float worldX, float worldZ, float& minimapX, float& minimapY);

    float m_screenX;
    float m_screenY;
    float m_width;
    float m_height;
    float m_zoom;
    
    MinimapMode m_mode;
    float m_playerX;
    float m_playerZ;
    float m_playerRotation;
    
    float m_worldMinX;
    float m_worldMinZ;
    float m_worldMaxX;
    float m_worldMaxZ;
    
    std::string m_backgroundTexture;
    std::string m_maskTexture;
    float m_borderColor[4];
    
    std::vector<MinimapIcon> m_icons;
    bool m_visible;
};

} // namespace Engine
