#include "ui/Minimap.h"
#include <algorithm>
#include <cmath>

namespace Engine {

MinimapRenderer::MinimapRenderer()
    : m_width(200)
    , m_height(200)
    , m_viewportX(10)
    , m_viewportY(10)
    , m_viewportWidth(200)
    , m_viewportHeight(200)
    , m_zoom(1.0f)
    , m_rotation(0.0f)
    , m_opacity(0.8f)
    , m_backgroundColor(0x000000FF)
    , m_borderColor(0xFFFFFFFF)
    , m_borderWidth(2.0f)
    , m_isVisible(true)
    , m_fogOfWarEnabled(false)
    , m_worldMinX(0.0f)
    , m_worldMinY(0.0f)
    , m_worldMaxX(1000.0f)
    , m_worldMaxY(1000.0f)
{
}

MinimapRenderer& MinimapRenderer::getInstance() {
    static MinimapRenderer instance;
    return instance;
}

void MinimapRenderer::initialize(int width, int height) {
    m_width = width;
    m_height = height;
    m_viewportWidth = width;
    m_viewportHeight = height;
    
    // Initialize fog of war grid if enabled
    if (m_fogOfWarEnabled) {
        int gridSize = 100;
        m_revealedAreas.resize(gridSize * gridSize, false);
    }
}

void MinimapRenderer::shutdown() {
    clearIcons();
    m_revealedAreas.clear();
}

void MinimapRenderer::render(float cameraX, float cameraY, float zoom) {
    if (!m_isVisible) {
        return;
    }
    
    renderBackground();
    
    if (m_fogOfWarEnabled) {
        renderFogOfWar();
    }
    
    renderIcons(cameraX, cameraY);
    renderBorder();
}

void MinimapRenderer::setViewport(int x, int y, int width, int height) {
    m_viewportX = x;
    m_viewportY = y;
    m_viewportWidth = width;
    m_viewportHeight = height;
}

void MinimapRenderer::addIcon(int entityId, float x, float y, unsigned int color,
                              float size, int iconType) {
    MinimapIcon icon;
    icon.entityId = entityId;
    icon.x = x;
    icon.y = y;
    icon.color = color;
    icon.size = size;
    icon.iconType = iconType;
    icon.isVisible = true;
    
    m_icons.push_back(icon);
}

void MinimapRenderer::updateIcon(int entityId, float x, float y) {
    MinimapIcon* icon = getIcon(entityId);
    if (icon) {
        icon->x = x;
        icon->y = y;
    }
}

void MinimapRenderer::removeIcon(int entityId) {
    m_icons.erase(
        std::remove_if(m_icons.begin(), m_icons.end(),
            [entityId](const MinimapIcon& icon) {
                return icon.entityId == entityId;
            }),
        m_icons.end()
    );
}

void MinimapRenderer::clearIcons() {
    m_icons.clear();
}

void MinimapRenderer::setOpacity(float opacity) {
    m_opacity = std::max(0.0f, std::min(1.0f, opacity));
}

void MinimapRenderer::setWorldBounds(float minX, float minY, float maxX, float maxY) {
    m_worldMinX = minX;
    m_worldMinY = minY;
    m_worldMaxX = maxX;
    m_worldMaxY = maxY;
}

void MinimapRenderer::revealArea(float x, float y, float radius) {
    if (!m_fogOfWarEnabled) {
        return;
    }
    
    // TODO: Update revealed areas grid
}

void MinimapRenderer::setFrameTexture(const std::string& texturePath) {
    // TODO: Load frame texture
}

void MinimapRenderer::renderBackground() {
    // TODO: Render minimap background
}

void MinimapRenderer::renderIcons(float cameraX, float cameraY) {
    for (const auto& icon : m_icons) {
        if (!icon.isVisible) {
            continue;
        }
        
        float minimapX, minimapY;
        worldToMinimap(icon.x, icon.y, minimapX, minimapY);
        
        // Apply rotation
        if (m_rotation != 0.0f) {
            float centerX = m_viewportWidth / 2.0f;
            float centerY = m_viewportHeight / 2.0f;
            float dx = minimapX - centerX;
            float dy = minimapY - centerY;
            float cos_a = cos(m_rotation);
            float sin_a = sin(m_rotation);
            minimapX = centerX + dx * cos_a - dy * sin_a;
            minimapY = centerY + dx * sin_a + dy * cos_a;
        }
        
        // TODO: Render icon at (minimapX, minimapY)
    }
}

void MinimapRenderer::renderBorder() {
    if (m_borderWidth <= 0.0f) {
        return;
    }
    
    // TODO: Render border rectangle
}

void MinimapRenderer::renderFogOfWar() {
    // TODO: Render fog of war overlay
}

MinimapIcon* MinimapRenderer::getIcon(int entityId) {
    for (auto& icon : m_icons) {
        if (icon.entityId == entityId) {
            return &icon;
        }
    }
    return nullptr;
}

void MinimapRenderer::worldToMinimap(float worldX, float worldY, 
                                    float& minimapX, float& minimapY) {
    // Normalize world coordinates to 0-1
    float normalizedX = (worldX - m_worldMinX) / (m_worldMaxX - m_worldMinX);
    float normalizedY = (worldY - m_worldMinY) / (m_worldMaxY - m_worldMinY);
    
    // Apply zoom
    normalizedX = (normalizedX - 0.5f) * m_zoom + 0.5f;
    normalizedY = (normalizedY - 0.5f) * m_zoom + 0.5f;
    
    // Convert to minimap coordinates
    minimapX = m_viewportX + normalizedX * m_viewportWidth;
    minimapY = m_viewportY + normalizedY * m_viewportHeight;
}

} // namespace Engine
