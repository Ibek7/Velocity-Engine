#include "graphics/FontRendering.h"
#include <algorithm>
#include <sstream>
#include <cmath>

namespace JJM {
namespace Graphics {

// FontStyle implementation
FontStyle::FontStyle()
    : color(255, 255, 255, 255), bold(false), italic(false),
      underline(false), strikethrough(false), outlineWidth(0.0f),
      outlineColor(0, 0, 0, 255), shadowOffsetX(0.0f), shadowOffsetY(0.0f),
      shadowColor(0, 0, 0, 128) {}

// Font implementation
Font::Font(const std::string& name)
    : name(name), fontSize(16), lineHeight(20.0f), ascender(16.0f), descender(4.0f) {}

Font::~Font() {}

bool Font::loadFromFile(const std::string& filename, int size) {
    (void)filename; (void)size;
    return false; // Stub
}

bool Font::loadFromMemory(const uint8_t* data, size_t size, int fSize) {
    (void)data; (void)size; (void)fSize;
    return false; // Stub
}

void Font::setSize(int size) {
    fontSize = size;
    lineHeight = size * 1.25f;
}

int Font::getSize() const { return fontSize; }

std::string Font::getName() const { return name; }

const Glyph* Font::getGlyph(uint32_t codepoint) const {
    auto it = glyphs.find(codepoint);
    return it != glyphs.end() ? &it->second : nullptr;
}

float Font::getKerning(uint32_t first, uint32_t second) const {
    for (const auto& pair : kerningPairs) {
        if (pair.first == first && pair.second == second) {
            return pair.amount;
        }
    }
    return 0.0f;
}

std::shared_ptr<Texture> Font::getTexture() const { return atlasTexture; }

float Font::getLineHeight() const { return lineHeight; }
float Font::getAscender() const { return ascender; }
float Font::getDescender() const { return descender; }

Math::Vector2D Font::measureText(const std::string& text) const {
    float width = getTextWidth(text);
    float height = getTextHeight(text);
    return Math::Vector2D(width, height);
}

float Font::getTextWidth(const std::string& text) const {
    float width = 0.0f;
    uint32_t previous = 0;
    
    for (char c : text) {
        uint32_t codepoint = static_cast<uint32_t>(c);
        const Glyph* glyph = getGlyph(codepoint);
        
        if (glyph) {
            width += glyph->xAdvance;
            width += getKerning(previous, codepoint);
        }
        previous = codepoint;
    }
    
    return width;
}

float Font::getTextHeight(const std::string& text) const {
    int lineCount = 1;
    for (char c : text) {
        if (c == '\n') lineCount++;
    }
    return lineHeight * lineCount;
}

void Font::addGlyph(const Glyph& glyph) {
    glyphs[glyph.codepoint] = glyph;
}

void Font::addKerningPair(uint32_t first, uint32_t second, float amount) {
    KerningPair pair;
    pair.first = first;
    pair.second = second;
    pair.amount = amount;
    kerningPairs.push_back(pair);
}

void Font::buildAtlas() {
    // Stub: Build texture atlas from glyphs
}

// FontRenderer implementation
FontRenderer::FontRenderer() : lineSpacing(1.0f), characterSpacing(0.0f) {}
FontRenderer::~FontRenderer() {}

void FontRenderer::drawText(Font* font, const std::string& text,
                           float x, float y, const FontStyle& style) {
    if (!font) return;
    
    float currentX = x;
    float currentY = y;
    uint32_t previous = 0;
    
    for (char c : text) {
        if (c == '\n') {
            currentX = x;
            currentY += font->getLineHeight() * lineSpacing;
            previous = 0;
            continue;
        }
        
        uint32_t codepoint = static_cast<uint32_t>(c);
        const Glyph* glyph = font->getGlyph(codepoint);
        
        if (glyph) {
            currentX += font->getKerning(previous, codepoint);
            renderGlyph(*glyph, currentX, currentY, style);
            currentX += glyph->xAdvance + characterSpacing;
        }
        previous = codepoint;
    }
}

void FontRenderer::drawTextAligned(Font* font, const std::string& text,
                                  float x, float y, TextAlign align,
                                  const FontStyle& style) {
    if (!font) return;
    
    float offsetX = 0.0f;
    if (align == TextAlign::Center) {
        offsetX = -font->getTextWidth(text) * 0.5f;
    } else if (align == TextAlign::Right) {
        offsetX = -font->getTextWidth(text);
    }
    
    drawText(font, text, x + offsetX, y, style);
}

void FontRenderer::drawTextBox(Font* font, const std::string& text,
                              float x, float y, float maxWidth,
                              TextAlign align, const FontStyle& style) {
    auto lines = wrapText(font, text, maxWidth);
    float currentY = y;
    
    for (const auto& line : lines) {
        drawTextAligned(font, line, x, currentY, align, style);
        currentY += font->getLineHeight() * lineSpacing;
    }
}

void FontRenderer::drawTextWrapped(Font* font, const std::string& text,
                                  float x, float y, float maxWidth,
                                  const FontStyle& style) {
    drawTextBox(font, text, x, y, maxWidth, TextAlign::Left, style);
}

TextLayout FontRenderer::layoutText(Font* font, const std::string& text,
                                   float maxWidth, TextAlign align) {
    TextLayout layout;
    layout.totalWidth = 0.0f;
    layout.totalHeight = 0.0f;
    
    if (!font) return layout;
    
    auto lines = wrapText(font, text, maxWidth);
    float currentY = 0.0f;
    
    for (const auto& lineText : lines) {
        TextLayout::Line line;
        line.text = lineText;
        line.width = font->getTextWidth(lineText);
        line.y = currentY;
        
        layout.totalWidth = std::max(layout.totalWidth, line.width);
        layout.lines.push_back(line);
        currentY += font->getLineHeight() * lineSpacing;
    }
    
    layout.totalHeight = currentY;
    return layout;
}

void FontRenderer::setLineSpacing(float spacing) { lineSpacing = spacing; }
float FontRenderer::getLineSpacing() const { return lineSpacing; }

void FontRenderer::setCharacterSpacing(float spacing) { characterSpacing = spacing; }
float FontRenderer::getCharacterSpacing() const { return characterSpacing; }

void FontRenderer::renderGlyph(const Glyph& glyph, float x, float y,
                              const FontStyle& style) {
    (void)glyph; (void)x; (void)y; (void)style;
    // Stub: Render glyph using graphics backend
}

std::vector<std::string> FontRenderer::wrapText(Font* font, const std::string& text,
                                               float maxWidth) {
    std::vector<std::string> lines;
    std::istringstream stream(text);
    std::string word;
    std::string currentLine;
    
    while (stream >> word) {
        std::string testLine = currentLine.empty() ? word : currentLine + " " + word;
        float width = font->getTextWidth(testLine);
        
        if (width > maxWidth && !currentLine.empty()) {
            lines.push_back(currentLine);
            currentLine = word;
        } else {
            currentLine = testLine;
        }
    }
    
    if (!currentLine.empty()) {
        lines.push_back(currentLine);
    }
    
    return lines;
}

// BitmapFont implementation
BitmapFont::BitmapFont(const std::string& name) : Font(name) {}
BitmapFont::~BitmapFont() {}

bool BitmapFont::loadFromBMFont(const std::string& fntFile) {
    (void)fntFile;
    return false; // Stub
}

bool BitmapFont::loadFromAngelCode(const std::string& fntFile) {
    return loadFromBMFont(fntFile);
}

bool BitmapFont::parseBMFontText(const std::string& content) {
    (void)content;
    return false; // Stub
}

bool BitmapFont::parseBMFontBinary(const uint8_t* data, size_t size) {
    (void)data; (void)size;
    return false; // Stub
}

// TrueTypeFont implementation
TrueTypeFont::TrueTypeFont(const std::string& name)
    : Font(name), hinting(true), antialiasing(true), ttfData(nullptr) {}

TrueTypeFont::~TrueTypeFont() {}

bool TrueTypeFont::loadFromTTF(const std::string& filename, int fontSize) {
    (void)filename; (void)fontSize;
    return false; // Stub
}

bool TrueTypeFont::loadFromTTFMemory(const uint8_t* data, size_t size, int fontSize) {
    (void)data; (void)size; (void)fontSize;
    return false; // Stub
}

void TrueTypeFont::setHinting(bool enabled) { hinting = enabled; }
bool TrueTypeFont::getHinting() const { return hinting; }

void TrueTypeFont::setAntialiasing(bool enabled) { antialiasing = enabled; }
bool TrueTypeFont::getAntialiasing() const { return antialiasing; }

bool TrueTypeFont::rasterizeGlyph(uint32_t codepoint, Glyph& glyph) {
    (void)codepoint; (void)glyph;
    return false; // Stub
}

// DistanceFieldFont implementation
DistanceFieldFont::DistanceFieldFont(const std::string& name)
    : Font(name), spread(4), smoothness(0.25f) {}

DistanceFieldFont::~DistanceFieldFont() {}

bool DistanceFieldFont::generateFromTTF(const std::string& filename, int fontSize, int spr) {
    (void)filename; (void)fontSize; (void)spr;
    return false; // Stub
}

void DistanceFieldFont::setSpread(int spr) { spread = spr; }
int DistanceFieldFont::getSpread() const { return spread; }

void DistanceFieldFont::setSmoothness(float s) { smoothness = s; }
float DistanceFieldFont::getSmoothness() const { return smoothness; }

void DistanceFieldFont::generateDistanceField(const uint8_t* bitmap, int width, int height,
                                             uint8_t* output) {
    (void)bitmap; (void)width; (void)height; (void)output;
    // Stub: Generate signed distance field
}

// FontCache implementation
FontCache::FontCache() {}
FontCache::~FontCache() {}

FontCache& FontCache::getInstance() {
    static FontCache instance;
    return instance;
}

std::shared_ptr<Font> FontCache::loadFont(const std::string& name,
                                         const std::string& filename,
                                         int fontSize) {
    auto font = std::make_shared<Font>(name);
    if (font->loadFromFile(filename, fontSize)) {
        fonts[name] = font;
        return font;
    }
    return nullptr;
}

std::shared_ptr<Font> FontCache::getFont(const std::string& name) {
    auto it = fonts.find(name);
    return it != fonts.end() ? it->second : defaultFont;
}

bool FontCache::hasFont(const std::string& name) const {
    return fonts.find(name) != fonts.end();
}

void FontCache::removeFont(const std::string& name) {
    fonts.erase(name);
}

void FontCache::clear() {
    fonts.clear();
}

std::shared_ptr<Font> FontCache::getDefaultFont() {
    return defaultFont;
}

void FontCache::setDefaultFont(std::shared_ptr<Font> font) {
    defaultFont = font;
}

// RichTextParser implementation
RichTextParser::RichTextParser() {}
RichTextParser::~RichTextParser() {}

std::vector<RichTextParser::TextSegment> RichTextParser::parse(const std::string& markup) {
    std::vector<TextSegment> segments;
    TextSegment segment;
    segment.style = defaultStyle;
    
    std::string currentText;
    FontStyle currentStyle = defaultStyle;
    
    for (size_t i = 0; i < markup.length(); ++i) {
        if (markup[i] == '<') {
            if (!currentText.empty()) {
                segment.text = currentText;
                segment.style = currentStyle;
                segments.push_back(segment);
                currentText.clear();
            }
            
            if (parseTag(markup, i, currentStyle)) {
                continue;
            }
        }
        currentText += markup[i];
    }
    
    if (!currentText.empty()) {
        segment.text = currentText;
        segment.style = currentStyle;
        segments.push_back(segment);
    }
    
    return segments;
}

void RichTextParser::setDefaultStyle(const FontStyle& style) {
    defaultStyle = style;
}

FontStyle RichTextParser::getDefaultStyle() const {
    return defaultStyle;
}

void RichTextParser::defineTag(const std::string& tag, const FontStyle& style) {
    tagStyles[tag] = style;
}

void RichTextParser::removeTag(const std::string& tag) {
    tagStyles.erase(tag);
}

bool RichTextParser::parseTag(const std::string& tag, size_t& pos, FontStyle& currentStyle) {
    (void)tag; (void)pos; (void)currentStyle;
    return false; // Stub
}

// TextEffects implementation
void TextEffects::drawOutlinedText(FontRenderer& renderer, Font* font,
                                   const std::string& text, float x, float y,
                                   const Color& textColor, const Color& outlineColor,
                                   float outlineWidth) {
    FontStyle outlineStyle;
    outlineStyle.color = outlineColor;
    
    // Draw outline
    for (int dx = -1; dx <= 1; ++dx) {
        for (int dy = -1; dy <= 1; ++dy) {
            if (dx != 0 || dy != 0) {
                renderer.drawText(font, text, x + dx * outlineWidth, y + dy * outlineWidth, outlineStyle);
            }
        }
    }
    
    // Draw main text
    FontStyle textStyle;
    textStyle.color = textColor;
    renderer.drawText(font, text, x, y, textStyle);
}

void TextEffects::drawShadowedText(FontRenderer& renderer, Font* font,
                                   const std::string& text, float x, float y,
                                   const Color& textColor, const Color& shadowColor,
                                   float shadowOffsetX, float shadowOffsetY) {
    FontStyle shadowStyle;
    shadowStyle.color = shadowColor;
    renderer.drawText(font, text, x + shadowOffsetX, y + shadowOffsetY, shadowStyle);
    
    FontStyle textStyle;
    textStyle.color = textColor;
    renderer.drawText(font, text, x, y, textStyle);
}

void TextEffects::drawGradientText(FontRenderer& renderer, Font* font,
                                   const std::string& text, float x, float y,
                                   const Color& topColor, const Color& bottomColor) {
    (void)renderer; (void)font; (void)text; (void)x; (void)y;
    (void)topColor; (void)bottomColor;
    // Stub: Implement gradient text
}

void TextEffects::drawWavyText(FontRenderer& renderer, Font* font,
                               const std::string& text, float x, float y,
                               float amplitude, float frequency, float time,
                               const FontStyle& style) {
    (void)renderer; (void)font; (void)text; (void)amplitude;
    (void)frequency; (void)time; (void)style;
    
    float currentX = x;
    for (size_t i = 0; i < text.length(); ++i) {
        float wave = amplitude * std::sin(frequency * i + time);
        std::string charStr(1, text[i]);
        renderer.drawText(font, charStr, currentX, y + wave, style);
        currentX += font->getTextWidth(charStr);
    }
}

void TextEffects::drawTypewriterText(FontRenderer& renderer, Font* font,
                                    const std::string& text, float x, float y,
                                    int visibleChars, const FontStyle& style) {
    std::string visible = text.substr(0, std::min<size_t>(visibleChars, text.length()));
    renderer.drawText(font, visible, x, y, style);
}

// TextInput implementation
TextInput::TextInput()
    : cursorPosition(0), selectionStart(0), selectionEnd(0), maxLength(0) {}

TextInput::~TextInput() {}

void TextInput::setText(const std::string& t) {
    text = t;
    if (maxLength > 0 && text.length() > maxLength) {
        text = text.substr(0, maxLength);
    }
    cursorPosition = std::min(cursorPosition, text.length());
}

std::string TextInput::getText() const { return text; }

void TextInput::setCursorPosition(size_t position) {
    cursorPosition = std::min(position, text.length());
}

size_t TextInput::getCursorPosition() const { return cursorPosition; }

void TextInput::setSelection(size_t start, size_t end) {
    selectionStart = std::min(start, text.length());
    selectionEnd = std::min(end, text.length());
}

void TextInput::getSelection(size_t& start, size_t& end) const {
    start = selectionStart;
    end = selectionEnd;
}

void TextInput::insertText(const std::string& insertText) {
    deleteSelection();
    text.insert(cursorPosition, insertText);
    cursorPosition += insertText.length();
    
    if (maxLength > 0 && text.length() > maxLength) {
        text = text.substr(0, maxLength);
        cursorPosition = std::min(cursorPosition, maxLength);
    }
}

void TextInput::deleteSelection() {
    if (selectionStart != selectionEnd) {
        size_t start = std::min(selectionStart, selectionEnd);
        size_t end = std::max(selectionStart, selectionEnd);
        text.erase(start, end - start);
        cursorPosition = start;
        selectionStart = selectionEnd = start;
    }
}

void TextInput::backspace() {
    if (selectionStart != selectionEnd) {
        deleteSelection();
    } else if (cursorPosition > 0) {
        text.erase(cursorPosition - 1, 1);
        cursorPosition--;
    }
}

void TextInput::deleteChar() {
    if (selectionStart != selectionEnd) {
        deleteSelection();
    } else if (cursorPosition < text.length()) {
        text.erase(cursorPosition, 1);
    }
}

void TextInput::moveCursorLeft() {
    if (cursorPosition > 0) cursorPosition--;
}

void TextInput::moveCursorRight() {
    if (cursorPosition < text.length()) cursorPosition++;
}

void TextInput::moveCursorToStart() { cursorPosition = 0; }
void TextInput::moveCursorToEnd() { cursorPosition = text.length(); }

std::string TextInput::getSelectedText() const {
    size_t start = std::min(selectionStart, selectionEnd);
    size_t end = std::max(selectionStart, selectionEnd);
    return text.substr(start, end - start);
}

void TextInput::replaceSelection(const std::string& replaceText) {
    deleteSelection();
    insertText(replaceText);
}

void TextInput::setMaxLength(size_t length) {
    maxLength = length;
    if (maxLength > 0 && text.length() > maxLength) {
        text = text.substr(0, maxLength);
        cursorPosition = std::min(cursorPosition, maxLength);
    }
}

size_t TextInput::getMaxLength() const { return maxLength; }

} // namespace Graphics
} // namespace JJM
