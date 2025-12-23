#include "ui/MinimapRenderer.h"
#include <cmath>
#include <algorithm>

namespace Engine {

MinimapRenderer::MinimapRenderer()
    : m_screenX(10.0f)
    , m_screenY(10.0f)
    , m_width(200.0f)
    , m_height(200.0f)
    , m_zoom(1.0f)
    , m_mode(MinimapMode::Rotating)
    , m_playerX(0.0f)
    , m_playerZ(0.0f)
    , m_playerRotation(0.0f)
    , m_worldMinX(-1000.0f)
    , m_worldMinZ(-1000.0f)
    , m_worldMaxX(1000.0f)
    , m_worldMaxZ(1000.0f)
    , m_visible(true)
{
    m_borderColor[0] = 1.0f;
    m_borderColor[1] = 1.0f;
    m_borderColor[2] = 1.0f;
    m_borderColor[3] = 1.0f;
}

MinimapRenderer& MinimapRenderer::getInstance() {
    static MinimapRenderer instance;
    return instance;
}

void MinimapRenderer::setPosition(float screenX, float screenY) {
    m_screenX = screenX;
    m_screenY = screenY;
}

void MinimapRenderer::setSize(float width, float height) {
    m_width = width;
    m_height = height;
}

void MinimapRenderer::setZoom(float zoom) {
    m_zoom = std::max(0.1f, std::min(10.0f, zoom));
}

void MinimapRenderer::setPlayerPosition(float worldX, float worldZ) {
    m_playerX = worldX;
    m_playerZ = worldZ;
}

void MinimapRenderer::setPlayerRotation(float degrees) {
    m_playerRotation = degrees;
}

void MinimapRenderer::addIcon(const MinimapIcon& icon) {
    m_icons.push_back(icon);
}

void MinimapRenderer::removeIcon(MinimapIconType type, float worldX, float worldZ) {
    m_icons.erase(
        std::remove_if(m_icons.begin(), m_icons.end(),
            [type, worldX, worldZ](const MinimapIcon& icon) {
                return icon.type == type && 
                       std::abs(icon.worldX - worldX) < 0.1f &&
                       std::abs(icon.worldZ - worldZ) < 0.1f;
            }),
        m_icons.end()
    );
}

void MinimapRenderer::clearIcons() {
    m_icons.clear();
}

void MinimapRenderer::updateIconPosition(MinimapIconType type, float oldX, float oldZ, float newX, float newZ) {
    for (auto& icon : m_icons) {
        if (icon.type == type && 
            std::abs(icon.worldX - oldX) < 0.1f &&
            std::abs(icon.worldZ - oldZ) < 0.1f) {
            icon.worldX = newX;
            icon.worldZ = newZ;
            break;
        }
    }
}

void MinimapRenderer::render() {
    if (!m_visible) {
        return;
    }
    
    renderBackground();
    renderIcons();
    renderBorder();
}

void MinimapRenderer::renderBackground() {
    // TODO: Render background texture or solid color
    // In a real implementation, this would draw the map background
}

void MinimapRenderer::renderIcons() {
    for (const auto& icon : m_icons) {
        if (!icon.visible) {
            continue;
        }
        
        float minimapX, minimapY;
        worldToMinimap(icon.worldX, icon.worldZ, minimapX, minimapY);
        
        // Check if icon is within minimap bounds
        if (minimapX < m_screenX || minimapX > m_screenX + m_width ||
            minimapY < m_screenY || minimapY > m_screenY + m_height) {
            continue;
        }
        
        // TODO: Render icon at (minimapX, minimapY)
        // Apply icon rotation, scale, and color
    }
}

void MinimapRenderer::renderBorder() {
    // TODO: Render border around minimap
    // Use m_borderColor for border color
}

void MinimapRenderer::worldToMinimap(float worldX, float worldZ, float& minimapX, float& minimapY) {
    // Calculate relative position to player
    float relX = worldX - m_playerX;
    float relZ = worldZ - m_playerZ;
    
    // Apply rotation if in rotating mode
    if (m_mode == MinimapMode::Rotating) {
        float angleRad = -m_playerRotation * 3.14159f / 180.0f;
        float cosAngle = std::cos(angleRad);
        float sinAngle = std::sin(angleRad);
        
        float rotX = relX * cosAngle - relZ * sinAngle;
        float rotZ = relX * sinAngle + relZ * cosAngle;
        
        relX = rotX;
        relZ = rotZ;
    }
    
    // Apply zoom
    relX *= m_zoom;
    relZ *= m_zoom;
    
    // Calculate world size
    float worldWidth = m_worldMaxX - m_worldMinX;
    float worldHeight = m_worldMaxZ - m_worldMinZ;
    
    // Scale to minimap size
    float scaleX = m_width / (worldWidth * m_zoom);
    float scaleZ = m_height / (worldHeight * m_zoom);
    
    // Convert to minimap coordinates (centered on player)
    minimapX = m_screenX + m_width * 0.5f + relX * scaleX;
    minimapY = m_screenY + m_height * 0.5f + relZ * scaleZ;
}

void MinimapRenderer::setBackgroundTexture(const std::string& path) {
    m_backgroundTexture = path;
}

void MinimapRenderer::setBorderColor(float r, float g, float b, float a) {
    m_borderColor[0] = r;
    m_borderColor[1] = g;
    m_borderColor[2] = b;
    m_borderColor[3] = a;
}

void MinimapRenderer::setMaskTexture(const std::string& path) {
    m_maskTexture = path;
}

void MinimapRenderer::setWorldBounds(float minX, float minZ, float maxX, float maxZ) {
    m_worldMinX = minX;
    m_worldMinZ = minZ;
    m_worldMaxX = maxX;
    m_worldMaxZ = maxZ;
}

} // namespace Engine
