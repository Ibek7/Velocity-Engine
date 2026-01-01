#pragma once

#include <vector>
#include <string>
#include <functional>

namespace Engine {

/**
 * @brief Compute shader dispatch dimensions
 */
struct DispatchSize {
    unsigned int x, y, z;
    
    DispatchSize() : x(1), y(1), z(1) {}
    DispatchSize(unsigned int x_, unsigned int y_, unsigned int z_) : x(x_), y(y_), z(z_) {}
};

/**
 * @brief Compute buffer types
 */
enum class BufferType {
    Uniform,        // Uniform buffer (read-only)
    Storage,        // Storage buffer (read-write)
    Atomic,         // Atomic counter buffer
    Texture,        // Texture buffer
    Image          // Image buffer (read-write texture)
};

/**
 * @brief Buffer binding information
 */
struct BufferBinding {
    unsigned int buffer;
    BufferType type;
    unsigned int binding;
    size_t size;
    void* data;
    
    BufferBinding()
        : buffer(0), type(BufferType::Storage), binding(0), size(0), data(nullptr)
    {}
};

/**
 * @brief Compute shader barrier types
 */
enum MemoryBarrier {
    MemoryBarrier_None            = 0,
    MemoryBarrier_ShaderStorage   = 1 << 0,
    MemoryBarrier_ShaderImage     = 1 << 1,
    MemoryBarrier_Uniform         = 1 << 2,
    MemoryBarrier_TextureFetch    = 1 << 3,
    MemoryBarrier_AtomicCounter   = 1 << 4,
    MemoryBarrier_All             = 0x7FFFFFFF
};
typedef int MemoryBarrierFlags;

/**
 * @brief Compute shader program
 */
class ComputeShader {
public:
    ComputeShader();
    ~ComputeShader();
    
    // Initialization
    bool compileFromSource(const std::string& source);
    bool compileFromFile(const std::string& path);
    void destroy();
    
    // Shader management
    unsigned int getProgram() const { return m_program; }
    bool isValid() const { return m_program != 0; }
    
    // Work group info
    void getWorkGroupSize(int& x, int& y, int& z) const;
    void getWorkGroupCount(int& x, int& y, int& z) const;
    int getLocalSize() const;
    
    // Uniform setters
    void setUniform1i(const std::string& name, int value);
    void setUniform1f(const std::string& name, float value);
    void setUniform2f(const std::string& name, float x, float y);
    void setUniform3f(const std::string& name, float x, float y, float z);
    void setUniform4f(const std::string& name, float x, float y, float z, float w);
    void setUniformMatrix4fv(const std::string& name, const float* matrix);
    
    // Buffer binding
    void bindBuffer(unsigned int binding, unsigned int buffer, BufferType type);
    void bindTexture(unsigned int binding, unsigned int texture);
    void bindImage(unsigned int binding, unsigned int texture, int level, bool layered, int layer);
    
    // Dispatch
    void dispatch(unsigned int groupsX, unsigned int groupsY = 1, unsigned int groupsZ = 1);
    void dispatchIndirect(unsigned int buffer, size_t offset = 0);
    
    // Memory barriers
    void memoryBarrier(MemoryBarrierFlags barriers);
    
private:
    int getUniformLocation(const std::string& name) const;
    
    unsigned int m_program;
    int m_workGroupSizeX;
    int m_workGroupSizeY;
    int m_workGroupSizeZ;
};

/**
 * @brief Compute shader manager
 */
class ComputeShaderSystem {
public:
    ComputeShaderSystem();
    ~ComputeShaderSystem();
    
    // Initialization
    void initialize();
    void shutdown();
    
    // Shader management
    int createShader(const std::string& name, const std::string& source);
    int loadShader(const std::string& name, const std::string& path);
    void destroyShader(const std::string& name);
    ComputeShader* getShader(const std::string& name);
    
    // Buffer creation
    unsigned int createStorageBuffer(size_t size, const void* data = nullptr);
    unsigned int createUniformBuffer(size_t size, const void* data = nullptr);
    unsigned int createAtomicBuffer(size_t size);
    void destroyBuffer(unsigned int buffer);
    
    // Buffer operations
    void updateBuffer(unsigned int buffer, size_t offset, size_t size, const void* data);
    void* mapBuffer(unsigned int buffer);
    void unmapBuffer(unsigned int buffer);
    void copyBuffer(unsigned int src, unsigned int dst, size_t srcOffset, size_t dstOffset, size_t size);
    
    // Common compute operations
    void computePrefix(unsigned int inputBuffer, unsigned int outputBuffer, int count);
    void computeSort(unsigned int buffer, int count);
    void computeReduce(unsigned int inputBuffer, unsigned int outputBuffer, int count);
    void computeHistogram(unsigned int inputBuffer, unsigned int histogramBuffer, int count, int bins);
    
    // Query support
    bool isComputeSupported() const;
    int getMaxWorkGroupCount(int dimension) const;
    int getMaxWorkGroupSize(int dimension) const;
    int getMaxLocalWorkGroupSize() const;
    
private:
    std::unordered_map<std::string, ComputeShader*> m_shaders;
    bool m_supported;
};

/**
 * @brief Parallel computation helper
 */
class ParallelCompute {
public:
    // 1D operations
    static void forEach(unsigned int buffer, int count, ComputeShader& shader);
    static void reduce(unsigned int inputBuffer, unsigned int outputBuffer, int count, ComputeShader& shader);
    static void scan(unsigned int buffer, int count, ComputeShader& shader);
    
    // 2D operations
    static void forEach2D(unsigned int buffer, int width, int height, ComputeShader& shader);
    static void convolution2D(unsigned int inputBuffer, unsigned int outputBuffer, 
                              int width, int height, ComputeShader& shader);
    
    // 3D operations
    static void forEach3D(unsigned int buffer, int width, int height, int depth, ComputeShader& shader);
    
    // Utility
    static DispatchSize calculateDispatchSize(int count, int localSize);
    static DispatchSize calculateDispatchSize2D(int width, int height, int localSizeX, int localSizeY);
    static DispatchSize calculateDispatchSize3D(int width, int height, int depth, 
                                                int localSizeX, int localSizeY, int localSizeZ);
};

/**
 * @brief Global compute shader system
 */
class ComputeSystem {
public:
    static ComputeSystem& getInstance();
    
    void initialize();
    void shutdown();
    
    ComputeShaderSystem& getShaderSystem() { return m_shaderSystem; }
    
    // Convenience
    int createShader(const std::string& name, const std::string& source) {
        return m_shaderSystem.createShader(name, source);
    }
    
    ComputeShader* getShader(const std::string& name) {
        return m_shaderSystem.getShader(name);
    }
    
private:
    ComputeSystem() {}
    ComputeSystem(const ComputeSystem&) = delete;
    ComputeSystem& operator=(const ComputeSystem&) = delete;
    
    ComputeShaderSystem m_shaderSystem;
};

} // namespace Engine
