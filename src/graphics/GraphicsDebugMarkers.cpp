#include "graphics/GraphicsDebugMarkers.h"
#ifdef __APPLE__
#include <OpenGL/gl3.h>
#define GLEW_KHR_debug 0
#else
#include <GL/glew.h>
#endif
#include <cstring>

namespace JJM {
namespace Graphics {

// Global instance
GraphicsDebugMarkers* g_debugMarkers = nullptr;

void InitializeDebugMarkers() {
    if (!g_debugMarkers) {
        g_debugMarkers = new GraphicsDebugMarkers();
        g_debugMarkers->initialize();
    }
}

void ShutdownDebugMarkers() {
    if (g_debugMarkers) {
        delete g_debugMarkers;
        g_debugMarkers = nullptr;
    }
}

// =============================================================================
// GraphicsDebugMarkers Implementation
// =============================================================================

GraphicsDebugMarkers::GraphicsDebugMarkers()
    : m_enabled(true),
      m_hasDebugExtension(false),
      m_nestingLevel(0),
      m_glPushDebugGroupFunc(nullptr),
      m_glPopDebugGroupFunc(nullptr),
      m_glObjectLabelFunc(nullptr),
      m_glDebugMessageInsertFunc(nullptr) {}

GraphicsDebugMarkers::~GraphicsDebugMarkers() = default;

bool GraphicsDebugMarkers::initialize() {
    loadDebugExtensions();
    return m_hasDebugExtension;
}

void GraphicsDebugMarkers::beginRegion(const std::string& name, const DebugColor& color) {
    if (!m_enabled || !m_hasDebugExtension) return;

    // Use GL_KHR_debug extension
    if (m_glPushDebugGroupFunc) {
        // Convert color to unsigned int (RGBA)
        unsigned int colorInt = (static_cast<unsigned int>(color.r * 255) << 24) |
                                (static_cast<unsigned int>(color.g * 255) << 16) |
                                (static_cast<unsigned int>(color.b * 255) << 8) |
                                static_cast<unsigned int>(color.a * 255);

        typedef void (*PushDebugGroupFunc)(GLenum source, GLuint id, GLsizei length,
                                           const GLchar* message);
        auto pushFunc = reinterpret_cast<PushDebugGroupFunc>(m_glPushDebugGroupFunc);
        pushFunc(GL_DEBUG_SOURCE_APPLICATION, colorInt, static_cast<GLsizei>(name.length()),
                 name.c_str());
    } else {
        // Fallback: use glPushDebugGroup if available
        if (GLEW_KHR_debug) {
            glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 0, static_cast<GLsizei>(name.length()),
                             name.c_str());
        }
    }

    m_markerStack.push(name);
    m_nestingLevel++;
}

void GraphicsDebugMarkers::endRegion() {
    if (!m_enabled || !m_hasDebugExtension) return;
    if (m_markerStack.empty()) return;

    if (m_glPopDebugGroupFunc) {
        typedef void (*PopDebugGroupFunc)(void);
        auto popFunc = reinterpret_cast<PopDebugGroupFunc>(m_glPopDebugGroupFunc);
        popFunc();
    } else {
        // Fallback
        if (GLEW_KHR_debug) {
            glPopDebugGroup();
        }
    }

    m_markerStack.pop();
    m_nestingLevel--;
}

void GraphicsDebugMarkers::insertMarker(const std::string& message) {
    if (!m_enabled || !m_hasDebugExtension) return;

    if (m_glDebugMessageInsertFunc) {
        typedef void (*DebugMessageInsertFunc)(GLenum source, GLenum type, GLuint id,
                                               GLenum severity, GLsizei length, const GLchar* buf);
        auto insertFunc = reinterpret_cast<DebugMessageInsertFunc>(m_glDebugMessageInsertFunc);
        insertFunc(GL_DEBUG_SOURCE_APPLICATION, GL_DEBUG_TYPE_MARKER, 0,
                   GL_DEBUG_SEVERITY_NOTIFICATION, static_cast<GLsizei>(message.length()),
                   message.c_str());
    } else {
        // Fallback
        if (GLEW_KHR_debug) {
            glDebugMessageInsert(GL_DEBUG_SOURCE_APPLICATION, GL_DEBUG_TYPE_MARKER, 0,
                                 GL_DEBUG_SEVERITY_NOTIFICATION,
                                 static_cast<GLsizei>(message.length()), message.c_str());
        }
    }
}

void GraphicsDebugMarkers::labelObject(unsigned int objectType, unsigned int objectID,
                                       const std::string& label) {
    if (!m_enabled || !m_hasDebugExtension) return;

    if (m_glObjectLabelFunc) {
        typedef void (*ObjectLabelFunc)(GLenum identifier, GLuint name, GLsizei length,
                                        const GLchar* label);
        auto labelFunc = reinterpret_cast<ObjectLabelFunc>(m_glObjectLabelFunc);
        labelFunc(objectType, objectID, static_cast<GLsizei>(label.length()), label.c_str());
    } else {
        // Fallback
        if (GLEW_KHR_debug) {
            glObjectLabel(objectType, objectID, static_cast<GLsizei>(label.length()),
                          label.c_str());
        }
    }
}

void GraphicsDebugMarkers::insertDebugMessage(unsigned int severity, const std::string& message) {
    if (!m_enabled || !m_hasDebugExtension) return;

    if (GLEW_KHR_debug) {
        glDebugMessageInsert(GL_DEBUG_SOURCE_APPLICATION, GL_DEBUG_TYPE_OTHER, 0, severity,
                             static_cast<GLsizei>(message.length()), message.c_str());
    }
}

void GraphicsDebugMarkers::loadDebugExtensions() {
    // Check for GL_KHR_debug extension
    if (GLEW_KHR_debug) {
        m_hasDebugExtension = true;

        // Get function pointers
        m_glPushDebugGroupFunc = reinterpret_cast<void*>(glPushDebugGroup);
        m_glPopDebugGroupFunc = reinterpret_cast<void*>(glPopDebugGroup);
        m_glObjectLabelFunc = reinterpret_cast<void*>(glObjectLabel);
        m_glDebugMessageInsertFunc = reinterpret_cast<void*>(glDebugMessageInsert);

        // Enable debug output
        glEnable(GL_DEBUG_OUTPUT);
        glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);

        // Set up debug callback
        glDebugMessageCallback(
            [](GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length,
               const GLchar* message, const void* userParam) {
                // Log debug messages
                // TODO: Implement proper logging
            },
            nullptr);

        // Filter messages
        glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);
    } else {
        // Check for vendor-specific extensions as fallback
        // AMD: GL_AMD_debug_output
        // NVIDIA: GL_NV_gpu_program4
        m_hasDebugExtension = false;
    }
}

}  // namespace Graphics
}  // namespace JJM
