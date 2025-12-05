#include "graphics/ShaderSystem.h"
#include <iostream>
#include <fstream>
#include <sstream>

namespace JJM {
namespace Graphics {

// Stubbed OpenGL functions
unsigned int glCreateShader(unsigned int) { static unsigned int id = 0; return ++id; }
void glShaderSource(unsigned int, int, const char**, const int*) {}
void glCompileShader(unsigned int) {}
void glGetShaderiv(unsigned int, unsigned int, int* params) { *params = 1; }
void glGetShaderInfoLog(unsigned int, int, int*, char*) {}
void glDeleteShader(unsigned int) {}
unsigned int glCreateProgram() { static unsigned int id = 0; return ++id; }
void glAttachShader(unsigned int, unsigned int) {}
void glLinkProgram(unsigned int) {}
void glGetProgramiv(unsigned int, unsigned int, int* params) { *params = 1; }
void glGetProgramInfoLog(unsigned int, int, int*, char*) {}
void glDetachShader(unsigned int, unsigned int) {}
void glDeleteProgram(unsigned int) {}
void glUseProgram(unsigned int) {}
int glGetUniformLocation(unsigned int, const char*) { static int loc = 0; return loc++; }
void glUniform1i(int, int) {}
void glUniform1f(int, float) {}
void glUniform2f(int, float, float) {}
void glUniform3f(int, float, float, float) {}
void glUniform4f(int, float, float, float, float) {}
void glUniformMatrix4fv(int, int, unsigned char, const float*) {}

#define GL_VERTEX_SHADER 0x8B31
#define GL_FRAGMENT_SHADER 0x8B30
#define GL_COMPILE_STATUS 0x8B81
#define GL_LINK_STATUS 0x8B82

Shader::Shader() : program(0), vertexShader(0), fragmentShader(0) {}

Shader::~Shader() {
    if (program) glDeleteProgram(program);
    if (vertexShader) glDeleteShader(vertexShader);
    if (fragmentShader) glDeleteShader(fragmentShader);
}

bool Shader::loadFromFiles(const std::string& vertexPath, const std::string& fragmentPath) {
    std::ifstream vFile(vertexPath);
    std::ifstream fFile(fragmentPath);
    
    if (!vFile.is_open() || !fFile.is_open()) return false;
    
    std::stringstream vStream, fStream;
    vStream << vFile.rdbuf();
    fStream << fFile.rdbuf();
    
    vertexSource = vStream.str();
    fragmentSource = fStream.str();
    
    return compile();
}

bool Shader::loadFromStrings(const std::string& vertexSrc, const std::string& fragmentSrc) {
    vertexSource = vertexSrc;
    fragmentSource = fragmentSrc;
    return compile();
}

bool Shader::compile() {
    vertexShader = glCreateShader(GL_VERTEX_SHADER);
    fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    
    if (!compileShader(vertexShader, vertexSource)) return false;
    if (!compileShader(fragmentShader, fragmentSource)) return false;
    if (!linkProgram()) return false;
    
    return true;
}

bool Shader::compileShader(unsigned int shader, const std::string& source) {
    const char* src = source.c_str();
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);
    
    int success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(shader, 512, nullptr, infoLog);
        std::cerr << "Shader compilation failed: " << infoLog << std::endl;
        return false;
    }
    return true;
}

bool Shader::linkProgram() {
    program = glCreateProgram();
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);
    
    int success;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetProgramInfoLog(program, 512, nullptr, infoLog);
        std::cerr << "Shader linking failed: " << infoLog << std::endl;
        return false;
    }
    
    glDetachShader(program, vertexShader);
    glDetachShader(program, fragmentShader);
    
    return true;
}

void Shader::use() const {
    glUseProgram(program);
}

void Shader::unuse() const {
    glUseProgram(0);
}

int Shader::getUniformLocation(const std::string& name) {
    if (uniformCache.find(name) != uniformCache.end()) {
        return uniformCache[name];
    }
    int location = glGetUniformLocation(program, name.c_str());
    uniformCache[name] = location;
    return location;
}

void Shader::setUniform(const std::string& name, int value) {
    glUniform1i(getUniformLocation(name), value);
}

void Shader::setUniform(const std::string& name, float value) {
    glUniform1f(getUniformLocation(name), value);
}

void Shader::setUniform(const std::string& name, float x, float y) {
    glUniform2f(getUniformLocation(name), x, y);
}

void Shader::setUniform(const std::string& name, float x, float y, float z) {
    glUniform3f(getUniformLocation(name), x, y, z);
}

void Shader::setUniform(const std::string& name, float x, float y, float z, float w) {
    glUniform4f(getUniformLocation(name), x, y, z, w);
}

void Shader::setUniformMatrix(const std::string& name, const float* matrix) {
    glUniformMatrix4fv(getUniformLocation(name), 1, 0, matrix);
}

ShaderLibrary& ShaderLibrary::getInstance() {
    static ShaderLibrary instance;
    return instance;
}

bool ShaderLibrary::loadShader(const std::string& name, const std::string& vertPath, const std::string& fragPath) {
    auto shader = std::make_shared<Shader>();
    if (shader->loadFromFiles(vertPath, fragPath)) {
        shaders[name] = shader;
        shaderPaths[name] = {vertPath, fragPath};
        return true;
    }
    return false;
}

std::shared_ptr<Shader> ShaderLibrary::getShader(const std::string& name) {
    auto it = shaders.find(name);
    if (it != shaders.end()) {
        return it->second;
    }
    return nullptr;
}

void ShaderLibrary::reloadAll() {
    for (const auto& [name, paths] : shaderPaths) {
        loadShader(name, paths.first, paths.second);
    }
}

} // namespace Graphics
} // namespace JJM
