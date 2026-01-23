#include "graphics/ViewportManager.h"
#include <stdexcept>
#include <algorithm>

namespace Engine {

ViewportManager::ViewportManager() 
    : m_nextViewportId(1)
    , m_activeViewport(-1) {
}

ViewportManager::~ViewportManager() {
    clear();
}

int ViewportManager::addViewport(const std::string& name, const Viewport& viewport) {
    ViewportEntry entry;
    entry.name = name;
    entry.viewport = viewport;
    entry.id = m_nextViewportId++;
    entry.active = false;
    
    m_viewports.push_back(entry);
    
    if (m_activeViewport == -1) {
        m_activeViewport = entry.id;
    }
    
    return entry.id;
}

void ViewportManager::removeViewport(int viewportId) {
    auto it = std::remove_if(m_viewports.begin(), m_viewports.end(),
        [viewportId](const ViewportEntry& entry) {
            return entry.id == viewportId;
        });
    
    if (it != m_viewports.end()) {
        m_viewports.erase(it, m_viewports.end());
        
        if (m_activeViewport == viewportId) {
            m_activeViewport = m_viewports.empty() ? -1 : m_viewports[0].id;
        }
    }
}

void ViewportManager::updateViewport(int viewportId, const Viewport& viewport) {
    int index = findViewportIndex(viewportId);
    if (index >= 0) {
        m_viewports[index].viewport = viewport;
    }
}

const Viewport& ViewportManager::getViewport(int viewportId) const {
    int index = findViewportIndex(viewportId);
    if (index >= 0) {
        return m_viewports[index].viewport;
    }
    throw std::runtime_error("Viewport not found");
}

const Viewport* ViewportManager::getViewportByName(const std::string& name) const {
    for (const auto& entry : m_viewports) {
        if (entry.name == name) {
            return &entry.viewport;
        }
    }
    return nullptr;
}

void ViewportManager::setActiveViewport(int viewportId) {
    int index = findViewportIndex(viewportId);
    if (index >= 0) {
        m_activeViewport = viewportId;
    }
}

void ViewportManager::applyViewport(int viewportId) {
    const Viewport& vp = getViewport(viewportId);
    // This would interface with the actual graphics API
    // For now, we just mark it as applied
    setActiveViewport(viewportId);
}

void ViewportManager::enableSplitScreen(int numViewports, bool horizontal) {
    clear();
    
    if (numViewports == 2) {
        if (horizontal) {
            addViewport("split_top", {0.0f, 0.0f, 1.0f, 0.5f, 0.0f, 1.0f, -1});
            addViewport("split_bottom", {0.0f, 0.5f, 1.0f, 0.5f, 0.0f, 1.0f, -1});
        } else {
            addViewport("split_left", {0.0f, 0.0f, 0.5f, 1.0f, 0.0f, 1.0f, -1});
            addViewport("split_right", {0.5f, 0.0f, 0.5f, 1.0f, 0.0f, 1.0f, -1});
        }
    } else if (numViewports == 4) {
        addViewport("split_tl", {0.0f, 0.0f, 0.5f, 0.5f, 0.0f, 1.0f, -1});
        addViewport("split_tr", {0.5f, 0.0f, 0.5f, 0.5f, 0.0f, 1.0f, -1});
        addViewport("split_bl", {0.0f, 0.5f, 0.5f, 0.5f, 0.0f, 1.0f, -1});
        addViewport("split_br", {0.5f, 0.5f, 0.5f, 0.5f, 0.0f, 1.0f, -1});
    }
}

void ViewportManager::enablePictureInPicture(int mainViewportId, int pipViewportId, 
                                            int position, float scale) {
    float pipWidth = scale;
    float pipHeight = scale;
    float margin = 0.02f;
    float x, y;
    
    switch (position) {
        case 0: // Top-left
            x = margin;
            y = margin;
            break;
        case 1: // Top-right
            x = 1.0f - pipWidth - margin;
            y = margin;
            break;
        case 2: // Bottom-left
            x = margin;
            y = 1.0f - pipHeight - margin;
            break;
        case 3: // Bottom-right
        default:
            x = 1.0f - pipWidth - margin;
            y = 1.0f - pipHeight - margin;
            break;
    }
    
    updateViewport(mainViewportId, {0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, -1});
    updateViewport(pipViewportId, {x, y, pipWidth, pipHeight, 0.0f, 1.0f, -1});
}

std::vector<int> ViewportManager::getAllViewports() const {
    std::vector<int> ids;
    ids.reserve(m_viewports.size());
    for (const auto& entry : m_viewports) {
        ids.push_back(entry.id);
    }
    return ids;
}

void ViewportManager::clear() {
    m_viewports.clear();
    m_activeViewport = -1;
}

bool ViewportManager::isPointInViewport(int viewportId, float x, float y) const {
    const Viewport& vp = getViewport(viewportId);
    return x >= vp.x && x < (vp.x + vp.width) &&
           y >= vp.y && y < (vp.y + vp.height);
}

bool ViewportManager::screenToViewportLocal(int viewportId, float screenX, float screenY,
                                           float& outLocalX, float& outLocalY) const {
    if (!isPointInViewport(viewportId, screenX, screenY)) {
        return false;
    }
    
    const Viewport& vp = getViewport(viewportId);
    outLocalX = (screenX - vp.x) / vp.width;
    outLocalY = (screenY - vp.y) / vp.height;
    return true;
}

int ViewportManager::findViewportIndex(int viewportId) const {
    for (size_t i = 0; i < m_viewports.size(); ++i) {
        if (m_viewports[i].id == viewportId) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

} // namespace Engine
