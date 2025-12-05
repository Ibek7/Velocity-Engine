#ifndef SHADER_SYSTEM_H
#define SHADER_SYSTEM_H

#include <string>
#include <unordered_map>
#include <memory>
#include <vector>

namespace JJM {
namespace Graphics {

class Shader {
public:
    Shader();
    ~Shader();
    
    bool loadFromFiles(const std::string& vertexPath, const std::string& fragmentPath);
    bool loadFromStrings(const std::string& vertexSrc, const std::string& fragmentSrc);
    bool compile();
    
    void use() const;
    void unuse() const;
    
    // Uniform setters
    void setUniform(const std::string& name, int value);
    void setUniform(const std::string& name, float value);
    void setUniform(const std::string& name, float x, float y);
    void setUniform(const std::string& name, float x, float y, float z);
    void setUniform(const std::string& name, float x, float y, float z, float w);
    void setUniformMatrix(const std::string& name, const float* matrix);
    
    unsigned int getProgram() const { return program; }
    bool isValid() const { return program != 0; }
    
private:
    unsigned int program;
    unsigned int vertexShader;
    unsigned int fragmentShader;
    std::string vertexSource;
    std::string fragmentSource;
    std::unordered_map<std::string, int> uniformCache;
    
    int getUniformLocation(const std::string& name);
    bool compileShader(unsigned int shader, const std::string& source);
    bool linkProgram();
};

class ShaderLibrary {
public:
    static ShaderLibrary& getInstance();
    
    bool loadShader(const std::string& name, const std::string& vertPath, const std::string& fragPath);
    std::shared_ptr<Shader> getShader(const std::string& name);
    void reloadAll();
    
private:
    ShaderLibrary() = default;
    std::unordered_map<std::string, std::shared_ptr<Shader>> shaders;
    std::unordered_map<std::string, std::pair<std::string, std::string>> shaderPaths;
};

} // namespace Graphics
} // namespace JJM

#endif
