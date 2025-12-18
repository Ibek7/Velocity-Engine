#ifndef JJM_FONT_RENDERING_H
#define JJM_FONT_RENDERING_H

#include "graphics/Texture.h"
#include "graphics/Color.h"
#include "math/Vector2D.h"
#include <string>
#include <memory>
#include <unordered_map>
#include <vector>

namespace JJM {
namespace Graphics {

/**
 * @brief Font glyph information
 */
struct Glyph {
    uint32_t codepoint;
    float x, y, width, height;
    float xOffset, yOffset;
    float xAdvance;
    float uvX, uvY, uvWidth, uvHeight;
};

/**
 * @brief Font kerning pair
 */
struct KerningPair {
    uint32_t first;
    uint32_t second;
    float amount;
};

/**
 * @brief Text alignment
 */
enum class TextAlign {
    Left,
    Center,
    Right,
    Justify
};

/**
 * @brief Text vertical alignment
 */
enum class TextVAlign {
    Top,
    Middle,
    Bottom,
    Baseline
};

/**
 * @brief Font rendering style
 */
struct FontStyle {
    Color color;
    bool bold;
    bool italic;
    bool underline;
    bool strikethrough;
    float outlineWidth;
    Color outlineColor;
    float shadowOffsetX;
    float shadowOffsetY;
    Color shadowColor;
    
    FontStyle();
};

/**
 * @brief Font class
 */
class Font {
public:
    Font(const std::string& name);
    ~Font();

    bool loadFromFile(const std::string& filename, int fontSize);
    bool loadFromMemory(const uint8_t* data, size_t size, int fontSize);
    
    void setSize(int size);
    int getSize() const;
    
    std::string getName() const;
    
    const Glyph* getGlyph(uint32_t codepoint) const;
    float getKerning(uint32_t first, uint32_t second) const;
    
    std::shared_ptr<Texture> getTexture() const;
    
    float getLineHeight() const;
    float getAscender() const;
    float getDescender() const;
    
    Math::Vector2D measureText(const std::string& text) const;
    float getTextWidth(const std::string& text) const;
    float getTextHeight(const std::string& text) const;

private:
    std::string name;
    int fontSize;
    float lineHeight;
    float ascender;
    float descender;
    
    std::unordered_map<uint32_t, Glyph> glyphs;
    std::vector<KerningPair> kerningPairs;
    std::shared_ptr<Texture> atlasTexture;
    
    void addGlyph(const Glyph& glyph);
    void addKerningPair(uint32_t first, uint32_t second, float amount);
    void buildAtlas();
};

/**
 * @brief Text layout for advanced text rendering
 */
struct TextLayout {
    struct Line {
        std::string text;
        float width;
        float y;
        std::vector<Math::Vector2D> glyphPositions;
    };
    
    std::vector<Line> lines;
    float totalWidth;
    float totalHeight;
};

/**
 * @brief Font renderer
 */
class FontRenderer {
public:
    FontRenderer();
    ~FontRenderer();

    void drawText(Font* font, const std::string& text,
                 float x, float y, const FontStyle& style = FontStyle());
    
    void drawTextAligned(Font* font, const std::string& text,
                        float x, float y, TextAlign align,
                        const FontStyle& style = FontStyle());
    
    void drawTextBox(Font* font, const std::string& text,
                    float x, float y, float maxWidth,
                    TextAlign align = TextAlign::Left,
                    const FontStyle& style = FontStyle());
    
    void drawTextWrapped(Font* font, const std::string& text,
                        float x, float y, float maxWidth,
                        const FontStyle& style = FontStyle());
    
    TextLayout layoutText(Font* font, const std::string& text,
                         float maxWidth, TextAlign align);
    
    void setLineSpacing(float spacing);
    float getLineSpacing() const;
    
    void setCharacterSpacing(float spacing);
    float getCharacterSpacing() const;

private:
    float lineSpacing;
    float characterSpacing;
    
    void renderGlyph(const Glyph& glyph, float x, float y,
                    const FontStyle& style);
    std::vector<std::string> wrapText(Font* font, const std::string& text,
                                     float maxWidth);
};

/**
 * @brief Bitmap font loader
 */
class BitmapFont : public Font {
public:
    BitmapFont(const std::string& name);
    ~BitmapFont();

    bool loadFromBMFont(const std::string& fntFile);
    bool loadFromAngelCode(const std::string& fntFile);

private:
    struct BMFontInfo {
        std::string face;
        int size;
        bool bold;
        bool italic;
        std::string charset;
        bool unicode;
    };
    
    bool parseBMFontText(const std::string& content);
    bool parseBMFontBinary(const uint8_t* data, size_t size);
};

/**
 * @brief TrueType font loader
 */
class TrueTypeFont : public Font {
public:
    TrueTypeFont(const std::string& name);
    ~TrueTypeFont();

    bool loadFromTTF(const std::string& filename, int fontSize);
    bool loadFromTTFMemory(const uint8_t* data, size_t size, int fontSize);
    
    void setHinting(bool enabled);
    bool getHinting() const;
    
    void setAntialiasing(bool enabled);
    bool getAntialiasing() const;

private:
    bool hinting;
    bool antialiasing;
    void* ttfData; // Platform-specific font data
    
    bool rasterizeGlyph(uint32_t codepoint, Glyph& glyph);
};

/**
 * @brief Distance field font for scalable rendering
 */
class DistanceFieldFont : public Font {
public:
    DistanceFieldFont(const std::string& name);
    ~DistanceFieldFont();

    bool generateFromTTF(const std::string& filename, int fontSize, int spread);
    
    void setSpread(int spread);
    int getSpread() const;
    
    void setSmoothness(float smoothness);
    float getSmoothness() const;

private:
    int spread;
    float smoothness;
    
    void generateDistanceField(const uint8_t* bitmap, int width, int height,
                              uint8_t* output);
};

/**
 * @brief Font cache for managing loaded fonts
 */
class FontCache {
public:
    static FontCache& getInstance();
    
    FontCache(const FontCache&) = delete;
    FontCache& operator=(const FontCache&) = delete;

    std::shared_ptr<Font> loadFont(const std::string& name,
                                   const std::string& filename,
                                   int fontSize);
    
    std::shared_ptr<Font> getFont(const std::string& name);
    bool hasFont(const std::string& name) const;
    
    void removeFont(const std::string& name);
    void clear();
    
    std::shared_ptr<Font> getDefaultFont();
    void setDefaultFont(std::shared_ptr<Font> font);

private:
    FontCache();
    ~FontCache();
    
    std::unordered_map<std::string, std::shared_ptr<Font>> fonts;
    std::shared_ptr<Font> defaultFont;
};

/**
 * @brief Rich text parser for markup
 */
class RichTextParser {
public:
    struct TextSegment {
        std::string text;
        FontStyle style;
    };
    
    RichTextParser();
    ~RichTextParser();

    std::vector<TextSegment> parse(const std::string& markup);
    
    void setDefaultStyle(const FontStyle& style);
    FontStyle getDefaultStyle() const;
    
    void defineTag(const std::string& tag, const FontStyle& style);
    void removeTag(const std::string& tag);

private:
    FontStyle defaultStyle;
    std::unordered_map<std::string, FontStyle> tagStyles;
    
    bool parseTag(const std::string& tag, size_t& pos, FontStyle& currentStyle);
};

/**
 * @brief Text effects
 */
class TextEffects {
public:
    static void drawOutlinedText(FontRenderer& renderer, Font* font,
                                 const std::string& text, float x, float y,
                                 const Color& textColor, const Color& outlineColor,
                                 float outlineWidth);
    
    static void drawShadowedText(FontRenderer& renderer, Font* font,
                                const std::string& text, float x, float y,
                                const Color& textColor, const Color& shadowColor,
                                float shadowOffsetX, float shadowOffsetY);
    
    static void drawGradientText(FontRenderer& renderer, Font* font,
                                const std::string& text, float x, float y,
                                const Color& topColor, const Color& bottomColor);
    
    static void drawWavyText(FontRenderer& renderer, Font* font,
                            const std::string& text, float x, float y,
                            float amplitude, float frequency, float time,
                            const FontStyle& style);
    
    static void drawTypewriterText(FontRenderer& renderer, Font* font,
                                  const std::string& text, float x, float y,
                                  int visibleChars, const FontStyle& style);
};

/**
 * @brief Text input helper
 */
class TextInput {
public:
    TextInput();
    ~TextInput();

    void setText(const std::string& text);
    std::string getText() const;
    
    void setCursorPosition(size_t position);
    size_t getCursorPosition() const;
    
    void setSelection(size_t start, size_t end);
    void getSelection(size_t& start, size_t& end) const;
    
    void insertText(const std::string& text);
    void deleteSelection();
    void backspace();
    void deleteChar();
    
    void moveCursorLeft();
    void moveCursorRight();
    void moveCursorToStart();
    void moveCursorToEnd();
    
    std::string getSelectedText() const;
    void replaceSelection(const std::string& text);
    
    void setMaxLength(size_t length);
    size_t getMaxLength() const;

private:
    std::string text;
    size_t cursorPosition;
    size_t selectionStart;
    size_t selectionEnd;
    size_t maxLength;
};

} // namespace Graphics
} // namespace JJM

#endif // JJM_FONT_RENDERING_H
