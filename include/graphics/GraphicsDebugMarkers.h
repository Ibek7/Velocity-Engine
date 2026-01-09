#ifndef GRAPHICS_DEBUG_MARKERS_H
#define GRAPHICS_DEBUG_MARKERS_H

#include <string>
#include <vector>
#include <stack>

namespace JJM {
namespace Graphics {

/**
 * @brief Debug marker color
 */
struct DebugColor {
    float r, g, b, a;
    
    DebugColor(float red = 1.0f, float green = 1.0f, float blue = 1.0f, float alpha = 1.0f)
        : r(red), g(green), b(blue), a(alpha) {}
    
    static DebugColor Red() { return DebugColor(1.0f, 0.0f, 0.0f, 1.0f); }
    static DebugColor Green() { return DebugColor(0.0f, 1.0f, 0.0f, 1.0f); }
    static DebugColor Blue() { return DebugColor(0.0f, 0.0f, 1.0f, 1.0f); }
    static DebugColor Yellow() { return DebugColor(1.0f, 1.0f, 0.0f, 1.0f); }
    static DebugColor Cyan() { return DebugColor(0.0f, 1.0f, 1.0f, 1.0f); }
    static DebugColor Magenta() { return DebugColor(1.0f, 0.0f, 1.0f, 1.0f); }
    static DebugColor White() { return DebugColor(1.0f, 1.0f, 1.0f, 1.0f); }
    static DebugColor Black() { return DebugColor(0.0f, 0.0f, 0.0f, 1.0f); }
};

/**
 * @brief Graphics debug marker system
 * 
 * Provides debug labels and markers for GPU debugging tools
 * (RenderDoc, NVIDIA Nsight, AMD Radeon GPU Profiler).
 * 
 * Supports:
 * - GL_KHR_debug (OpenGL)
 * - VK_EXT_debug_utils (Vulkan)
 * - D3D debug layers (DirectX)
 */
class GraphicsDebugMarkers {
private:
    bool m_enabled;
    bool m_hasDebugExtension;
    std::stack<std::string> m_markerStack;
    int m_nestingLevel;
    
    // Function pointers for debug extensions
    void* m_glPushDebugGroupFunc;
    void* m_glPopDebugGroupFunc;
    void* m_glObjectLabelFunc;
    void* m_glDebugMessageInsertFunc;
    
public:
    GraphicsDebugMarkers();
    ~GraphicsDebugMarkers();
    
    /**
     * @brief Initialize debug markers
     * @return True if debug extension is available
     */
    bool initialize();
    
    /**
     * @brief Begin a named debug group/region
     * @param name Region name to display in debugger
     * @param color Optional color for visualization
     */
    void beginRegion(const std::string& name, const DebugColor& color = DebugColor::White());
    
    /**
     * @brief End the current debug region
     */
    void endRegion();
    
    /**
     * @brief Insert a debug marker at current position
     * @param message Message to display in debugger
     */
    void insertMarker(const std::string& message);
    
    /**
     * @brief Label a GPU object for debugging
     * @param objectType Type of object (GL_TEXTURE, GL_BUFFER, etc.)
     * @param objectID GPU handle/ID
     * @param label Human-readable label
     */
    void labelObject(unsigned int objectType, unsigned int objectID, const std::string& label);
    
    /**
     * @brief Label a shader program
     */
    void labelProgram(unsigned int programID, const std::string& label) {
        labelObject(0x82E4 /* GL_PROGRAM */, programID, label);
    }
    
    /**
     * @brief Label a texture
     */
    void labelTexture(unsigned int textureID, const std::string& label) {
        labelObject(0x1702 /* GL_TEXTURE */, textureID, label);
    }
    
    /**
     * @brief Label a buffer
     */
    void labelBuffer(unsigned int bufferID, const std::string& label) {
        labelObject(0x82E0 /* GL_BUFFER */, bufferID, label);
    }
    
    /**
     * @brief Label a framebuffer
     */
    void labelFramebuffer(unsigned int fboID, const std::string& label) {
        labelObject(0x8D40 /* GL_FRAMEBUFFER */, fboID, label);
    }
    
    /**
     * @brief Insert a custom debug message
     * @param severity Message severity (GL_DEBUG_SEVERITY_*)
     * @param message Message text
     */
    void insertDebugMessage(unsigned int severity, const std::string& message);
    
    /**
     * @brief Enable or disable debug markers
     */
    void setEnabled(bool enabled) { m_enabled = enabled; }
    bool isEnabled() const { return m_enabled; }
    
    /**
     * @brief Check if debug extension is available
     */
    bool hasDebugExtension() const { return m_hasDebugExtension; }
    
    /**
     * @brief Get current nesting level
     */
    int getNestingLevel() const { return m_nestingLevel; }
    
    /**
     * @brief RAII helper for automatic begin/end
     */
    class ScopedRegion {
    private:
        GraphicsDebugMarkers* m_markers;
        bool m_active;
        
    public:
        ScopedRegion(GraphicsDebugMarkers* markers, const std::string& name,
                    const DebugColor& color = DebugColor::White())
            : m_markers(markers)
            , m_active(markers && markers->isEnabled())
        {
            if (m_active) {
                m_markers->beginRegion(name, color);
            }
        }
        
        ~ScopedRegion() {
            if (m_active) {
                m_markers->endRegion();
            }
        }
        
        // Non-copyable
        ScopedRegion(const ScopedRegion&) = delete;
        ScopedRegion& operator=(const ScopedRegion&) = delete;
    };
    
private:
    void loadDebugExtensions();
};

/**
 * @brief Global debug markers instance
 */
extern GraphicsDebugMarkers* g_debugMarkers;

/**
 * @brief Initialize global debug markers
 */
void InitializeDebugMarkers();

/**
 * @brief Shutdown global debug markers
 */
void ShutdownDebugMarkers();

// Convenience macros
#ifdef _DEBUG
    #define GPU_DEBUG_REGION(name) \
        JJM::Graphics::GraphicsDebugMarkers::ScopedRegion CONCAT(_debug_region_, __LINE__) \
        (JJM::Graphics::g_debugMarkers, name)
    
    #define GPU_DEBUG_REGION_COLOR(name, color) \
        JJM::Graphics::GraphicsDebugMarkers::ScopedRegion CONCAT(_debug_region_, __LINE__) \
        (JJM::Graphics::g_debugMarkers, name, color)
    
    #define GPU_DEBUG_MARKER(message) \
        if (JJM::Graphics::g_debugMarkers) { \
            JJM::Graphics::g_debugMarkers->insertMarker(message); \
        }
    
    #define GPU_LABEL_TEXTURE(id, label) \
        if (JJM::Graphics::g_debugMarkers) { \
            JJM::Graphics::g_debugMarkers->labelTexture(id, label); \
        }
    
    #define GPU_LABEL_BUFFER(id, label) \
        if (JJM::Graphics::g_debugMarkers) { \
            JJM::Graphics::g_debugMarkers->labelBuffer(id, label); \
        }
    
    #define GPU_LABEL_PROGRAM(id, label) \
        if (JJM::Graphics::g_debugMarkers) { \
            JJM::Graphics::g_debugMarkers->labelProgram(id, label); \
        }
#else
    #define GPU_DEBUG_REGION(name) ((void)0)
    #define GPU_DEBUG_REGION_COLOR(name, color) ((void)0)
    #define GPU_DEBUG_MARKER(message) ((void)0)
    #define GPU_LABEL_TEXTURE(id, label) ((void)0)
    #define GPU_LABEL_BUFFER(id, label) ((void)0)
    #define GPU_LABEL_PROGRAM(id, label) ((void)0)
#endif

} // namespace Graphics
} // namespace JJM

#endif // GRAPHICS_DEBUG_MARKERS_H
