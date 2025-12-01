# Contributing to JJM Velocity Engine

Thank you for your interest in contributing! This document provides guidelines and best practices for contributing to the project.

## Table of Contents

- [Code of Conduct](#code-of-conduct)
- [Getting Started](#getting-started)
- [Development Process](#development-process)
- [Coding Standards](#coding-standards)
- [Commit Messages](#commit-messages)
- [Pull Request Process](#pull-request-process)
- [Testing](#testing)
- [Documentation](#documentation)

## Code of Conduct

- Be respectful and inclusive
- Focus on constructive feedback
- Help others learn and grow
- Assume good intentions

## Getting Started

### Prerequisites

1. C++17 compatible compiler
2. SDL2 and related libraries installed
3. Git for version control
4. Make for building

### Setting Up Development Environment

```bash
# Clone the repository
git clone https://github.com/yourusername/JJM.git
cd JJM

# Install dependencies (macOS)
brew install sdl2 sdl2_image sdl2_mixer sdl2_ttf

# Or on Linux
sudo apt-get install libsdl2-dev libsdl2-image-dev libsdl2-mixer-dev libsdl2-ttf-dev

# Build the project
make debug

# Run tests if available
make test
```

## Development Process

### Branching Strategy

- `main`: Stable release branch
- `develop`: Integration branch for features
- `feature/*`: New features
- `bugfix/*`: Bug fixes
- `hotfix/*`: Critical fixes for main

### Workflow

1. Create a branch from `develop`:
   ```bash
   git checkout -b feature/my-awesome-feature develop
   ```

2. Make your changes with incremental commits

3. Keep your branch updated:
   ```bash
   git fetch origin
   git rebase origin/develop
   ```

4. Test thoroughly

5. Submit a pull request to `develop`

## Coding Standards

### C++ Style Guide

#### Naming Conventions

```cpp
// Namespaces: PascalCase
namespace JJM {
namespace Graphics {

// Classes: PascalCase
class Renderer {
public:
    // Public methods: camelCase
    void drawSprite(const Texture& texture);
    
    // Public members: camelCase (avoid public data)
    int getWidth() const { return width; }
    
private:
    // Private members: camelCase with no prefix
    int width;
    int height;
    
    // Private methods: camelCase
    void updateViewport();
};

// Functions: camelCase
void processInput(InputManager& input);

// Constants: UPPER_SNAKE_CASE
const int MAX_PARTICLES = 1000;

// Enums: PascalCase for type, UPPER_SNAKE_CASE for values
enum class BlendMode {
    NONE,
    ALPHA,
    ADDITIVE,
    MULTIPLY
};

// Template parameters: PascalCase with T prefix
template<typename TComponent>
class ComponentManager;
```

#### File Organization

```cpp
// Header file: MyClass.h
#ifndef MYCLASS_H
#define MYCLASS_H

#include <standard_headers>
#include "engine/headers.h"
#include "local/headers.h"

namespace JJM {

class MyClass {
public:
    // Public interface
    
protected:
    // Protected members
    
private:
    // Private implementation
};

} // namespace JJM

#endif // MYCLASS_H
```

```cpp
// Implementation file: MyClass.cpp
#include "../../include/path/MyClass.h"

namespace JJM {

// Implementation

} // namespace JJM
```

#### Code Formatting

- **Indentation:** 4 spaces (no tabs)
- **Braces:** Opening brace on same line
- **Line Length:** Max 100 characters
- **Spacing:** Space after keywords, around operators

```cpp
// Good
if (condition) {
    doSomething();
} else {
    doSomethingElse();
}

for (int i = 0; i < count; i++) {
    process(i);
}

// Function declarations
void myFunction(int param1, float param2);

// Constructor initialization
MyClass::MyClass(int value)
    : memberVar(value),
      anotherMember(0) {
}
```

#### Modern C++ Features

Use C++17 features appropriately:

```cpp
// Use auto for complex types
auto it = container.begin();
auto result = complexFunction();

// Use range-based for loops
for (const auto& item : collection) {
    process(item);
}

// Use smart pointers
std::unique_ptr<Resource> resource = std::make_unique<Resource>();
std::shared_ptr<Data> sharedData = std::make_shared<Data>();

// Use std::optional for nullable values
std::optional<int> findValue(const std::string& key);

// Use structured bindings
auto [x, y] = getPosition();

// Use if with initializer
if (auto result = compute(); result > 0) {
    use(result);
}
```

#### Memory Management

```cpp
// Prefer RAII
class Resource {
public:
    Resource() { allocate(); }
    ~Resource() { deallocate(); }
    
    // Delete copy, implement move
    Resource(const Resource&) = delete;
    Resource& operator=(const Resource&) = delete;
    Resource(Resource&&) = default;
    Resource& operator=(Resource&&) = default;
};

// Use smart pointers
std::unique_ptr<Texture> texture;  // Exclusive ownership
std::shared_ptr<Font> font;        // Shared ownership
std::weak_ptr<Scene> scene;        // Non-owning reference

// Avoid manual new/delete
// Bad
Sprite* sprite = new Sprite();
delete sprite;

// Good
auto sprite = std::make_unique<Sprite>();
```

#### Const Correctness

```cpp
class Example {
public:
    // Const methods don't modify state
    int getValue() const { return value; }
    
    // Const parameters can't be modified
    void setValue(const int newValue) { value = newValue; }
    
    // Const reference for large objects
    void setName(const std::string& name) { this->name = name; }
    
    // Return const reference to avoid copies
    const std::string& getName() const { return name; }
    
private:
    int value;
    std::string name;
};
```

#### Error Handling

```cpp
// Use exceptions for exceptional cases
class ResourceLoadException : public std::runtime_exception {
public:
    ResourceLoadException(const std::string& msg) 
        : std::runtime_exception(msg) {}
};

// Return std::optional for expected failures
std::optional<Texture> loadTexture(const std::string& path) {
    if (!fileExists(path)) {
        return std::nullopt;
    }
    return Texture(path);
}

// Use assertions for programmer errors
void setIndex(int index) {
    assert(index >= 0 && index < size);
    // ...
}
```

### Documentation

#### Header Comments

```cpp
/**
 * @brief Brief description of the class
 * 
 * Detailed description of what the class does,
 * its purpose, and how to use it.
 * 
 * @example
 * ```cpp
 * MyClass obj(param);
 * obj.doSomething();
 * ```
 */
class MyClass {
public:
    /**
     * @brief Brief description of method
     * @param param1 Description of parameter
     * @param param2 Description of parameter
     * @return Description of return value
     * @throws ExceptionType When this exception is thrown
     */
    int myMethod(int param1, float param2);
};
```

#### Implementation Comments

```cpp
// Explain WHY, not WHAT
// Bad: Increment counter
counter++;

// Good: Track number of active connections for connection pooling
activeConnections++;

// Use comments for complex algorithms
// Fisher-Yates shuffle algorithm for uniform randomization
for (int i = array.size() - 1; i > 0; i--) {
    int j = random(0, i);
    std::swap(array[i], array[j]);
}
```

## Commit Messages

Follow the conventional commits specification:

### Format

```
<type>(<scope>): <subject>

<body>

<footer>
```

### Types

- `feat`: New feature
- `fix`: Bug fix
- `docs`: Documentation changes
- `style`: Code style changes (formatting, etc.)
- `refactor`: Code refactoring
- `perf`: Performance improvements
- `test`: Adding or updating tests
- `chore`: Maintenance tasks

### Examples

```
feat(physics): Add spatial hashing for collision detection

Implement spatial hash grid to optimize broad-phase collision
detection for large numbers of entities. This reduces O(n²)
comparisons to O(n) average case.

Closes #123
```

```
fix(renderer): Fix texture bleeding in sprite batch

Adjusted UV coordinates to account for texture filtering when
using texture atlas. Adds 0.5px padding to prevent bleeding
between sprites.

Fixes #456
```

```
docs(api): Add usage examples for animation system

Added comprehensive examples showing:
- Frame-based animation setup
- Skeletal animation with IK
- Animation blending techniques
```

## Pull Request Process

### Before Submitting

1. **Build Successfully:** Ensure code compiles without errors
   ```bash
   make clean
   make release
   ```

2. **Follow Code Style:** Run formatter/linter if available

3. **Add Tests:** Include tests for new functionality

4. **Update Documentation:** Document new APIs and features

5. **Check Performance:** Profile if making performance changes

### PR Template

```markdown
## Description
Brief description of changes

## Type of Change
- [ ] Bug fix
- [ ] New feature
- [ ] Breaking change
- [ ] Documentation update

## Testing
How the changes were tested

## Checklist
- [ ] Code follows project style guidelines
- [ ] Self-review completed
- [ ] Comments added for complex code
- [ ] Documentation updated
- [ ] No new warnings introduced
- [ ] Tests added/updated
- [ ] All tests passing
```

### Review Process

1. Automated CI builds must pass
2. At least one approval from maintainer
3. All review comments addressed
4. No merge conflicts with target branch

## Testing

### Unit Tests

```cpp
// Test structure (when test framework is added)
TEST_CASE("Vector2D arithmetic") {
    SECTION("Addition") {
        Vector2D a(1, 2);
        Vector2D b(3, 4);
        Vector2D c = a + b;
        
        REQUIRE(c.x == 4);
        REQUIRE(c.y == 6);
    }
    
    SECTION("Normalization") {
        Vector2D v(3, 4);
        v.normalize();
        
        REQUIRE(std::abs(v.magnitude() - 1.0f) < 0.001f);
    }
}
```

### Manual Testing

- Build and run the engine
- Test new features interactively
- Check for visual artifacts
- Verify performance is acceptable

## Documentation

### What to Document

- Public APIs and interfaces
- Complex algorithms
- Non-obvious design decisions
- Usage examples
- Performance characteristics
- Thread safety guarantees

### Where to Document

- Header files: Public API documentation
- Implementation files: Algorithm explanations
- README.md: Project overview and setup
- ARCHITECTURE.md: System design
- Wiki/docs folder: Detailed guides and tutorials

## Questions?

If you have questions or need help:

1. Check existing documentation
2. Look at similar code in the project
3. Ask in discussions or issues
4. Contact maintainers

## License

By contributing, you agree that your contributions will be licensed under the same license as the project (MIT License).

---

**Thank you for contributing to JJM Velocity Engine! 🎮**
