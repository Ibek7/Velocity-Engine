#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include <memory>
#include "math/Vector2D.h"
#include "graphics/Color.h"

namespace JJM {
namespace Graphics {

struct Glyph {
    char32_t character;
    Math::Vector2D position;
    Math::Vector2D size;
    Math::Vector2D bearing;
    float advance;
    
    // UV coordinates in texture atlas
    Math::Vector2D uvMin;
    Math::Vector2D uvMax;
    
    Glyph() : character(0), advance(0.0f) {}
};

class Font {
public:
    Font();
    explicit Font(const std::string& path, int size = 16);
    ~Font();
    
    bool loadFromFile(const std::string& path, int size);
    bool isLoaded() const { return loaded; }
    
    const Glyph* getGlyph(char32_t character) const;
    
    float getLineHeight() const { return lineHeight; }
    int getSize() const { return fontSize; }
    
    void* getTextureAtlas() const { return textureAtlas; }
    
    Math::Vector2D measureText(const std::string& text) const;
    Math::Vector2D measureText(const std::u32string& text) const;

private:
    std::string path;
    int fontSize;
    float lineHeight;
    bool loaded;
    
    std::unordered_map<char32_t, Glyph> glyphs;
    void* textureAtlas; // Texture containing all glyphs
    
    bool generateAtlas();
};

class TextRenderer {
public:
    TextRenderer();
    ~TextRenderer();
    
    void setFont(std::shared_ptr<Font> font);
    std::shared_ptr<Font> getFont() const { return font; }
    
    void drawText(const std::string& text, const Math::Vector2D& position, 
                  const Color& color = Color(255, 255, 255, 255));
    
    void drawText(const std::u32string& text, const Math::Vector2D& position,
                  const Color& color = Color(255, 255, 255, 255));
    
    void setScale(float scale) { this->scale = scale; }
    float getScale() const { return scale; }
    
    void setLineSpacing(float spacing) { this->lineSpacing = spacing; }
    float getLineSpacing() const { return lineSpacing; }
    
    Math::Vector2D measureText(const std::string& text) const;

private:
    std::shared_ptr<Font> font;
    float scale;
    float lineSpacing;
    
    void renderGlyph(const Glyph& glyph, const Math::Vector2D& position, const Color& color);
};

enum class TextAlignment {
    Left,
    Center,
    Right,
    Justify
};

class FormattedTextRenderer {
public:
    FormattedTextRenderer();
    ~FormattedTextRenderer();
    
    void setFont(std::shared_ptr<Font> font);
    void setAlignment(TextAlignment alignment) { this->alignment = alignment; }
    void setMaxWidth(float width) { this->maxWidth = width; }
    void setColor(const Color& color) { this->color = color; }
    
    void drawText(const std::string& text, const Math::Vector2D& position);
    
    void setWordWrap(bool wrap) { this->wordWrap = wrap; }
    bool getWordWrap() const { return wordWrap; }
    
    float getLineHeight() const;
    Math::Vector2D measureText(const std::string& text) const;

private:
    std::shared_ptr<Font> font;
    TextAlignment alignment;
    float maxWidth;
    Color color;
    bool wordWrap;
    
    std::vector<std::string> wrapText(const std::string& text) const;
    float getLineWidth(const std::string& line) const;
};

class TextBatch {
public:
    TextBatch();
    ~TextBatch();
    
    void begin();
    void end();
    
    void drawText(std::shared_ptr<Font> font, const std::string& text,
                  const Math::Vector2D& position, const Color& color = Color(255, 255, 255, 255));
    
    void flush();
    void clear();

private:
    struct TextVertex {
        Math::Vector2D position;
        Math::Vector2D uv;
        Color color;
    };
    
    std::vector<TextVertex> vertices;
    std::vector<uint32_t> indices;
    bool batching;
    
    void addQuad(const Math::Vector2D& pos, const Math::Vector2D& size,
                 const Math::Vector2D& uvMin, const Math::Vector2D& uvMax,
                 const Color& color);
};

class RichTextRenderer {
public:
    RichTextRenderer();
    ~RichTextRenderer();
    
    void setDefaultFont(std::shared_ptr<Font> font);
    void setDefaultColor(const Color& color) { defaultColor = color; }
    
    void drawRichText(const std::string& richText, const Math::Vector2D& position);
    
    // Rich text markup: <color=#FF0000>Red Text</color> <b>Bold</b> <i>Italic</i>
    void registerTag(const std::string& tag, 
                    std::function<void(const std::string&)> handler);

private:
    std::shared_ptr<Font> defaultFont;
    Color defaultColor;
    std::unordered_map<std::string, std::function<void(const std::string&)>> tagHandlers;
    
    struct TextSegment {
        std::string text;
        Color color;
        std::shared_ptr<Font> font;
        bool bold;
        bool italic;
        
        TextSegment() : bold(false), italic(false) {}
    };
    
    std::vector<TextSegment> parseRichText(const std::string& richText);
};

class BMFontLoader {
public:
    static std::shared_ptr<Font> loadBMFont(const std::string& fntPath);

private:
    struct BMFontInfo {
        std::string face;
        int size;
        int lineHeight;
        int base;
        std::string texturePath;
    };
    
    static bool parseFNT(const std::string& path, BMFontInfo& info, 
                        std::unordered_map<char32_t, Glyph>& glyphs);
};

} // namespace Graphics
} // namespace JJM
