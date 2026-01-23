#pragma once

#include <vector>
#include <string>
#include <functional>

/**
 * @file ViewportManager.h
 * @brief Advanced viewport management system for multi-view rendering
 * 
 * Manages multiple viewports for split-screen, picture-in-picture,
 * and multi-camera rendering scenarios.
 */

namespace Engine {

/**
 * @struct Viewport
 * @brief Defines a rendering viewport region
 */
struct Viewport {
    float x, y;          ///< Viewport position (normalized 0-1)
    float width, height; ///< Viewport dimensions (normalized 0-1)
    float minDepth;      ///< Minimum depth value
    float maxDepth;      ///< Maximum depth value
    int layerMask;       ///< Render layer mask for selective rendering
};

/**
 * @class ViewportManager
 * @brief Manages multiple viewports for complex rendering scenarios
 */
class ViewportManager {
public:
    ViewportManager();
    ~ViewportManager();
    
    /**
     * @brief Add a new viewport
     * @param name Unique identifier for the viewport
     * @param viewport Viewport configuration
     * @return Viewport ID
     */
    int addViewport(const std::string& name, const Viewport& viewport);
    
    /**
     * @brief Remove a viewport by ID
     * @param viewportId Viewport identifier
     */
    void removeViewport(int viewportId);
    
    /**
     * @brief Update viewport configuration
     * @param viewportId Viewport identifier
     * @param viewport New viewport configuration
     */
    void updateViewport(int viewportId, const Viewport& viewport);
    
    /**
     * @brief Get viewport by ID
     * @param viewportId Viewport identifier
     * @return Viewport configuration
     */
    const Viewport& getViewport(int viewportId) const;
    
    /**
     * @brief Get viewport by name
     * @param name Viewport name
     * @return Viewport configuration
     */
    const Viewport* getViewportByName(const std::string& name) const;
    
    /**
     * @brief Set active viewport for rendering
     * @param viewportId Viewport identifier
     */
    void setActiveViewport(int viewportId);
    
    /**
     * @brief Get currently active viewport
     * @return Active viewport ID
     */
    int getActiveViewport() const { return m_activeViewport; }
    
    /**
     * @brief Apply viewport to rendering context
     * @param viewportId Viewport identifier
     */
    void applyViewport(int viewportId);
    
    /**
     * @brief Enable split-screen mode
     * @param numViewports Number of viewports (2 or 4)
     * @param horizontal Split horizontally (true) or vertically (false)
     */
    void enableSplitScreen(int numViewports, bool horizontal = true);
    
    /**
     * @brief Enable picture-in-picture mode
     * @param mainViewportId Main viewport ID
     * @param pipViewportId PIP viewport ID
     * @param position PIP position (0=top-left, 1=top-right, 2=bottom-left, 3=bottom-right)
     * @param scale PIP scale relative to screen (0-1)
     */
    void enablePictureInPicture(int mainViewportId, int pipViewportId, int position, float scale = 0.25f);
    
    /**
     * @brief Get all viewport IDs
     * @return Vector of viewport IDs
     */
    std::vector<int> getAllViewports() const;
    
    /**
     * @brief Clear all viewports
     */
    void clear();
    
    /**
     * @brief Check if point is within viewport
     * @param viewportId Viewport identifier
     * @param x Screen X coordinate (0-1)
     * @param y Screen Y coordinate (0-1)
     * @return True if point is within viewport
     */
    bool isPointInViewport(int viewportId, float x, float y) const;
    
    /**
     * @brief Convert screen coordinates to viewport-local coordinates
     * @param viewportId Viewport identifier
     * @param screenX Screen X coordinate
     * @param screenY Screen Y coordinate
     * @param outLocalX Output local X coordinate
     * @param outLocalY Output local Y coordinate
     * @return True if conversion successful
     */
    bool screenToViewportLocal(int viewportId, float screenX, float screenY, 
                              float& outLocalX, float& outLocalY) const;

private:
    struct ViewportEntry {
        std::string name;
        Viewport viewport;
        int id;
        bool active;
    };
    
    std::vector<ViewportEntry> m_viewports;
    int m_nextViewportId;
    int m_activeViewport;
    
    int findViewportIndex(int viewportId) const;
};

} // namespace Engine
