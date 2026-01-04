#ifndef SPRITE_BATCH_H
#define SPRITE_BATCH_H

#include "math/Vector2D.h"
#include "graphics/Color.h"
#include "graphics/Texture.h"
#include "graphics/Renderer.h"
#include <vector>
#include <SDL.h>
#include <unordered_map>
#include <memory>
#include <array>
#include <algorithm>
#include <cmath>

namespace JJM {
namespace Graphics {

// =============================================================================
// Advanced Sprite Batching System
// =============================================================================

// Sprite blend modes
enum class SpriteBlendMode {
    Alpha,
    Additive,
    Multiply,
    Screen,
    None
};

// Sprite flip flags
enum class SpriteFlip {
    None = 0,
    Horizontal = 1,
    Vertical = 2,
    Both = 3
};

// Sprite sort mode
enum class SpriteSortMode {
    None,           // No sorting (draw order)
    FrontToBack,    // By depth (front first)
    BackToFront,    // By depth (back first)
    Texture,        // Group by texture
    Immediate       // Draw immediately, no batching
};

struct SpriteData {
    Texture* texture;
    Math::Vector2D position;
    Math::Vector2D size;
    float rotation;
    Math::Vector2D origin;
    Color tint;
    SDL_Rect* sourceRect;
    int layer;
    
    SpriteData()
        : texture(nullptr), position(0, 0), size(0, 0),
          rotation(0), origin(0, 0), tint(Color::White()),
          sourceRect(nullptr), layer(0) {}
};

// =============================================================================
// Texture Atlas System
// =============================================================================

struct AtlasRegion {
    std::string name;
    int x, y;
    int width, height;
    
    // Rotation and trimming
    bool rotated;
    int originalWidth, originalHeight;
    int offsetX, offsetY;
    
    // UV coordinates (normalized)
    float u0, v0, u1, v1;
    
    // Nine-slice data (for scalable UI)
    int left, right, top, bottom;  // Border sizes
    bool hasNineSlice;
    
    AtlasRegion()
        : x(0), y(0), width(0), height(0)
        , rotated(false)
        , originalWidth(0), originalHeight(0)
        , offsetX(0), offsetY(0)
        , u0(0), v0(0), u1(1), v1(1)
        , left(0), right(0), top(0), bottom(0)
        , hasNineSlice(false)
    {}
};

class TextureAtlas {
private:
    Texture* texture;
    std::unordered_map<std::string, AtlasRegion> regions;
    int atlasWidth, atlasHeight;
    std::string name;
    
public:
    TextureAtlas() : texture(nullptr), atlasWidth(0), atlasHeight(0) {}
    
    bool loadFromFile(const std::string& atlasPath, Texture* tex);
    bool loadFromJSON(const std::string& jsonPath, Texture* tex);
    
    void addRegion(const AtlasRegion& region);
    const AtlasRegion* getRegion(const std::string& name) const;
    bool hasRegion(const std::string& name) const;
    
    Texture* getTexture() const { return texture; }
    int getWidth() const { return atlasWidth; }
    int getHeight() const { return atlasHeight; }
    
    std::vector<std::string> getRegionNames() const;
    
    // Generate UV coordinates for a region
    void calculateUVs(AtlasRegion& region) {
        region.u0 = static_cast<float>(region.x) / atlasWidth;
        region.v0 = static_cast<float>(region.y) / atlasHeight;
        region.u1 = static_cast<float>(region.x + region.width) / atlasWidth;
        region.v1 = static_cast<float>(region.y + region.height) / atlasHeight;
    }
};

// =============================================================================
// Sprite Vertex for GPU Batching
// =============================================================================

struct SpriteVertex {
    float x, y;           // Position
    float u, v;           // Texture coordinates
    float r, g, b, a;     // Color
    
    SpriteVertex()
        : x(0), y(0), u(0), v(0)
        , r(1), g(1), b(1), a(1)
    {}
    
    SpriteVertex(float px, float py, float tu, float tv, const Color& c)
        : x(px), y(py), u(tu), v(tv)
        , r(c.r / 255.0f), g(c.g / 255.0f), b(c.b / 255.0f), a(c.a / 255.0f)
    {}
};

// =============================================================================
// Sprite Instance Data for Instanced Rendering
// =============================================================================

struct SpriteInstance {
    // Transform (4x4 matrix compressed to needed values)
    float posX, posY;
    float scaleX, scaleY;
    float rotation;
    float originX, originY;
    
    // Texture region
    float u0, v0, u1, v1;
    
    // Color
    float r, g, b, a;
    
    // Depth
    float depth;
    
    SpriteInstance()
        : posX(0), posY(0)
        , scaleX(1), scaleY(1)
        , rotation(0)
        , originX(0), originY(0)
        , u0(0), v0(0), u1(1), v1(1)
        , r(1), g(1), b(1), a(1)
        , depth(0)
    {}
};

// =============================================================================
// Batch Statistics
// =============================================================================

struct BatchStatistics {
    size_t drawCalls;
    size_t spriteCount;
    size_t vertexCount;
    size_t batchCount;
    size_t textureSwaps;
    size_t blendModeSwaps;
    float batchEfficiency;      // Sprites per draw call
    
    BatchStatistics()
        : drawCalls(0), spriteCount(0), vertexCount(0)
        , batchCount(0), textureSwaps(0), blendModeSwaps(0)
        , batchEfficiency(0)
    {}
    
    void reset() {
        drawCalls = 0;
        spriteCount = 0;
        vertexCount = 0;
        batchCount = 0;
        textureSwaps = 0;
        blendModeSwaps = 0;
        batchEfficiency = 0;
    }
    
    void calculate() {
        if (drawCalls > 0) {
            batchEfficiency = static_cast<float>(spriteCount) / drawCalls;
        }
    }
};

// =============================================================================
// Advanced Sprite Batch with Instancing Support
// =============================================================================

class AdvancedSpriteBatch {
public:
    struct BatchConfig {
        size_t maxSpritesPerBatch;
        size_t initialBatchCapacity;
        bool useInstancing;
        bool autoSort;
        SpriteSortMode defaultSortMode;
        
        BatchConfig()
            : maxSpritesPerBatch(8192)
            , initialBatchCapacity(256)
            , useInstancing(false)
            , autoSort(true)
            , defaultSortMode(SpriteSortMode::Texture)
        {}
    };
    
private:
    // Sprite entry for batching
    struct SpriteEntry {
        Texture* texture;
        SpriteBlendMode blendMode;
        float depth;
        SpriteVertex vertices[4];
        
        bool operator<(const SpriteEntry& other) const {
            return depth < other.depth;
        }
    };
    
    BatchConfig config;
    Renderer* renderer;
    
    // Sprite storage
    std::vector<SpriteEntry> spriteBuffer;
    
    // Instance data for instanced rendering
    std::vector<SpriteInstance> instanceBuffer;
    
    // Vertex buffer for non-instanced rendering
    std::vector<SpriteVertex> vertexBuffer;
    std::vector<uint16_t> indexBuffer;
    
    // Current state
    bool begun;
    SpriteSortMode currentSortMode;
    SpriteBlendMode currentBlendMode;
    Texture* currentTexture;
    
    // Custom shader support
    uint32_t customShader;
    bool usingCustomShader;
    
    // Statistics
    BatchStatistics stats;
    BatchStatistics lastFrameStats;
    
    // Transform matrix stack
    std::vector<std::array<float, 6>> transformStack;  // 2D affine transform
    std::array<float, 6> currentTransform;
    
public:
    AdvancedSpriteBatch(Renderer* renderer);
    ~AdvancedSpriteBatch();
    
    void setConfig(const BatchConfig& cfg) { config = cfg; }
    const BatchConfig& getConfig() const { return config; }
    
    // Batch control
    void begin(SpriteSortMode sortMode = SpriteSortMode::Texture);
    void end();
    void flush();
    
    // Basic draw methods
    void draw(Texture* texture, const Math::Vector2D& position);
    void draw(Texture* texture, const Math::Vector2D& position, const Color& tint);
    void draw(Texture* texture, const Math::Vector2D& position, const Math::Vector2D& size);
    void draw(Texture* texture, const Math::Vector2D& position, const Math::Vector2D& size,
              const Color& tint);
    
    // Full draw with all parameters
    void draw(Texture* texture, 
              const Math::Vector2D& position, 
              const Math::Vector2D& size,
              const SDL_Rect* sourceRect,
              float rotation, 
              const Math::Vector2D& origin,
              const Color& tint,
              SpriteFlip flip = SpriteFlip::None,
              float depth = 0,
              SpriteBlendMode blendMode = SpriteBlendMode::Alpha);
    
    // Atlas region drawing
    void drawRegion(TextureAtlas* atlas, const std::string& regionName,
                   const Math::Vector2D& position,
                   const Color& tint = Color::White(),
                   float rotation = 0,
                   const Math::Vector2D& scale = Math::Vector2D(1, 1),
                   float depth = 0);
    
    // Nine-slice drawing for UI
    void drawNineSlice(TextureAtlas* atlas, const std::string& regionName,
                      const Math::Vector2D& position,
                      const Math::Vector2D& size,
                      const Color& tint = Color::White(),
                      float depth = 0);
    
    // Transform matrix
    void pushTransform();
    void popTransform();
    void setTransform(const std::array<float, 6>& transform);
    void translate(float x, float y);
    void rotate(float radians);
    void scale(float sx, float sy);
    void resetTransform();
    
    // Blend mode
    void setBlendMode(SpriteBlendMode mode);
    SpriteBlendMode getBlendMode() const { return currentBlendMode; }
    
    // Custom shader
    void setCustomShader(uint32_t shaderProgram);
    void clearCustomShader();
    
    // Statistics
    const BatchStatistics& getStatistics() const { return lastFrameStats; }
    void resetStatistics() { stats.reset(); lastFrameStats.reset(); }
    
private:
    void addSprite(Texture* texture, const SpriteVertex* vertices, 
                   SpriteBlendMode blendMode, float depth);
    
    void sortSprites();
    void renderBatch();
    void renderInstanced();
    
    void applyTransform(float& x, float& y) const;
    void calculateVertices(SpriteVertex* vertices,
                          const Math::Vector2D& position,
                          const Math::Vector2D& size,
                          const SDL_Rect* sourceRect,
                          float rotation,
                          const Math::Vector2D& origin,
                          const Color& tint,
                          SpriteFlip flip,
                          int texWidth, int texHeight);
    
    void setupBlendMode(SpriteBlendMode mode);
};

// =============================================================================
// Sprite Animation Support
// =============================================================================

struct AnimationFrame {
    std::string regionName;     // Atlas region name
    float duration;             // Frame duration in seconds
    Math::Vector2D offset;      // Render offset
    
    AnimationFrame()
        : duration(0.1f)
        , offset(0, 0)
    {}
    
    AnimationFrame(const std::string& region, float dur)
        : regionName(region), duration(dur), offset(0, 0)
    {}
};

class SpriteAnimation {
private:
    std::string name;
    std::vector<AnimationFrame> frames;
    bool looping;
    float totalDuration;
    
    // Playback state
    float currentTime;
    size_t currentFrameIndex;
    bool playing;
    float playbackSpeed;
    
public:
    SpriteAnimation(const std::string& animName = "")
        : name(animName)
        , looping(true)
        , totalDuration(0)
        , currentTime(0)
        , currentFrameIndex(0)
        , playing(false)
        , playbackSpeed(1.0f)
    {}
    
    void addFrame(const AnimationFrame& frame) {
        frames.push_back(frame);
        totalDuration += frame.duration;
    }
    
    void addFrames(const std::string& prefix, int startIndex, int endIndex, 
                  float frameDuration, const std::string& suffix = "") {
        for (int i = startIndex; i <= endIndex; ++i) {
            AnimationFrame frame;
            frame.regionName = prefix + std::to_string(i) + suffix;
            frame.duration = frameDuration;
            addFrame(frame);
        }
    }
    
    void play() { playing = true; }
    void pause() { playing = false; }
    void stop() { playing = false; currentTime = 0; currentFrameIndex = 0; }
    void reset() { currentTime = 0; currentFrameIndex = 0; }
    
    void setLooping(bool loop) { looping = loop; }
    void setPlaybackSpeed(float speed) { playbackSpeed = speed; }
    
    void update(float deltaTime) {
        if (!playing || frames.empty()) return;
        
        currentTime += deltaTime * playbackSpeed;
        
        // Advance frames
        while (currentTime >= frames[currentFrameIndex].duration) {
            currentTime -= frames[currentFrameIndex].duration;
            ++currentFrameIndex;
            
            if (currentFrameIndex >= frames.size()) {
                if (looping) {
                    currentFrameIndex = 0;
                } else {
                    currentFrameIndex = frames.size() - 1;
                    playing = false;
                    break;
                }
            }
        }
    }
    
    const AnimationFrame& getCurrentFrame() const {
        static AnimationFrame empty;
        return frames.empty() ? empty : frames[currentFrameIndex];
    }
    
    size_t getCurrentFrameIndex() const { return currentFrameIndex; }
    float getProgress() const { return totalDuration > 0 ? currentTime / totalDuration : 0; }
    bool isPlaying() const { return playing; }
    bool isLooping() const { return looping; }
    const std::string& getName() const { return name; }
    size_t getFrameCount() const { return frames.size(); }
};

// =============================================================================
// Animated Sprite Component
// =============================================================================

class AnimatedSprite {
private:
    TextureAtlas* atlas;
    std::unordered_map<std::string, SpriteAnimation> animations;
    std::string currentAnimationName;
    SpriteAnimation* currentAnimation;
    
    // Render properties
    Math::Vector2D position;
    Math::Vector2D scale;
    float rotation;
    Math::Vector2D origin;
    Color tint;
    SpriteFlip flip;
    float depth;
    
public:
    AnimatedSprite(TextureAtlas* textureAtlas = nullptr)
        : atlas(textureAtlas)
        , currentAnimation(nullptr)
        , position(0, 0)
        , scale(1, 1)
        , rotation(0)
        , origin(0, 0)
        , tint(Color::White())
        , flip(SpriteFlip::None)
        , depth(0)
    {}
    
    void setAtlas(TextureAtlas* textureAtlas) { atlas = textureAtlas; }
    
    void addAnimation(const std::string& name, const SpriteAnimation& animation) {
        animations[name] = animation;
    }
    
    void play(const std::string& animationName) {
        if (currentAnimationName == animationName && currentAnimation && currentAnimation->isPlaying()) {
            return;
        }
        
        auto it = animations.find(animationName);
        if (it != animations.end()) {
            currentAnimationName = animationName;
            currentAnimation = &it->second;
            currentAnimation->reset();
            currentAnimation->play();
        }
    }
    
    void pause() { if (currentAnimation) currentAnimation->pause(); }
    void stop() { if (currentAnimation) currentAnimation->stop(); }
    
    void update(float deltaTime) {
        if (currentAnimation) {
            currentAnimation->update(deltaTime);
        }
    }
    
    void draw(AdvancedSpriteBatch& batch) {
        if (!atlas || !currentAnimation) return;
        
        const auto& frame = currentAnimation->getCurrentFrame();
        Math::Vector2D drawPos = position + frame.offset;
        
        batch.drawRegion(atlas, frame.regionName, drawPos, tint, rotation, scale, depth);
    }
    
    // Accessors
    void setPosition(const Math::Vector2D& pos) { position = pos; }
    void setScale(const Math::Vector2D& s) { scale = s; }
    void setRotation(float rot) { rotation = rot; }
    void setOrigin(const Math::Vector2D& orig) { origin = orig; }
    void setTint(const Color& color) { tint = color; }
    void setFlip(SpriteFlip f) { flip = f; }
    void setDepth(float d) { depth = d; }
    
    const Math::Vector2D& getPosition() const { return position; }
    const std::string& getCurrentAnimationName() const { return currentAnimationName; }
};

class SpriteBatch {
private:
    std::vector<SpriteData> sprites;
    Renderer* renderer;
    bool begun;
    bool needsSort;
    
public:
    SpriteBatch(Renderer* renderer);
    ~SpriteBatch();
    
    void begin();
    void end();
    
    void draw(Texture* texture, const Math::Vector2D& position);
    void draw(Texture* texture, const Math::Vector2D& position, const Math::Vector2D& size);
    void draw(Texture* texture, const Math::Vector2D& position, const Math::Vector2D& size,
             float rotation, const Math::Vector2D& origin, const Color& tint);
    void draw(Texture* texture, const Math::Vector2D& position, const Math::Vector2D& size,
             SDL_Rect* sourceRect, float rotation, const Math::Vector2D& origin,
             const Color& tint, int layer = 0);
    
    void flush();
    void clear();
    
    int getSpriteCount() const { return static_cast<int>(sprites.size()); }
    
private:
    void sortSprites();
};

} // namespace Graphics
} // namespace JJM

#endif // SPRITE_BATCH_H
