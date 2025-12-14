#include "graphics/TextRenderer.h"
#include <algorithm>
#include <sstream>
#include <fstream>
#include <codecvt>
#include <locale>

namespace JJM {
namespace Graphics {

// Font implementation
Font::Font()
    : fontSize(16), lineHeight(16.0f), loaded(false), textureAtlas(nullptr) {}

Font::Font(const std::string& path, int size)
    : path(path), fontSize(size), lineHeight(static_cast<float>(size)), 
      loaded(false), textureAtlas(nullptr) {
    loadFromFile(path, size);
}

Font::~Font() {
    if (textureAtlas) {
        // Free texture
        textureAtlas = nullptr;
    }
}

bool Font::loadFromFile(const std::string& path, int size) {
    this->path = path;
    this->fontSize = size;
    
    // In a real implementation, this would use FreeType or similar
    // to load the font and generate the texture atlas
    
    loaded = generateAtlas();
    return loaded;
}

const Glyph* Font::getGlyph(char32_t character) const {
    auto it = glyphs.find(character);
    if (it != glyphs.end()) {
        return &it->second;
    }
    return nullptr;
}

Math::Vector2D Font::measureText(const std::string& text) const {
    std::wstring_convert<std::codecvt_utf8<char32_t>, char32_t> converter;
    std::u32string u32text = converter.from_bytes(text);
    return measureText(u32text);
}

Math::Vector2D Font::measureText(const std::u32string& text) const {
    float width = 0.0f;
    float height = lineHeight;
    float currentWidth = 0.0f;
    
    for (char32_t c : text) {
        if (c == '\n') {
            width = std::max(width, currentWidth);
            currentWidth = 0.0f;
            height += lineHeight;
            continue;
        }
        
        const Glyph* glyph = getGlyph(c);
        if (glyph) {
            currentWidth += glyph->advance;
        }
    }
    
    width = std::max(width, currentWidth);
    return Math::Vector2D(width, height);
}

bool Font::generateAtlas() {
    // Generate basic ASCII glyphs
    for (char32_t c = 32; c < 127; ++c) {
        Glyph glyph;
        glyph.character = c;
        glyph.size = Math::Vector2D(static_cast<float>(fontSize), static_cast<float>(fontSize));
        glyph.advance = static_cast<float>(fontSize);
        glyphs[c] = glyph;
    }
    
    return true;
}

// TextRenderer implementation
TextRenderer::TextRenderer()
    : scale(1.0f), lineSpacing(1.0f) {}

TextRenderer::~TextRenderer() {}

void TextRenderer::setFont(std::shared_ptr<Font> font) {
    this->font = font;
}

void TextRenderer::drawText(const std::string& text, const Math::Vector2D& position, const Color& color) {
    std::wstring_convert<std::codecvt_utf8<char32_t>, char32_t> converter;
    std::u32string u32text = converter.from_bytes(text);
    drawText(u32text, position, color);
}

void TextRenderer::drawText(const std::u32string& text, const Math::Vector2D& position, const Color& color) {
    if (!font || !font->isLoaded()) {
        return;
    }
    
    Math::Vector2D currentPos = position;
    
    for (char32_t c : text) {
        if (c == '\n') {
            currentPos.x = position.x;
            currentPos.y += font->getLineHeight() * lineSpacing * scale;
            continue;
        }
        
        const Glyph* glyph = font->getGlyph(c);
        if (glyph) {
            renderGlyph(*glyph, currentPos, color);
            currentPos.x += glyph->advance * scale;
        }
    }
}

Math::Vector2D TextRenderer::measureText(const std::string& text) const {
    if (!font) {
        return Math::Vector2D(0, 0);
    }
    
    Math::Vector2D size = font->measureText(text);
    return Math::Vector2D(size.x * scale, size.y * scale * lineSpacing);
}

void TextRenderer::renderGlyph(const Glyph& glyph, const Math::Vector2D& position, const Color& color) {
    // In a real implementation, this would render the glyph quad using the graphics API
    // Using the glyph's UV coordinates from the font atlas texture
}

// FormattedTextRenderer implementation
FormattedTextRenderer::FormattedTextRenderer()
    : alignment(TextAlignment::Left), maxWidth(0.0f), 
      color(255, 255, 255, 255), wordWrap(true) {}

FormattedTextRenderer::~FormattedTextRenderer() {}

void FormattedTextRenderer::setFont(std::shared_ptr<Font> font) {
    this->font = font;
}

void FormattedTextRenderer::drawText(const std::string& text, const Math::Vector2D& position) {
    if (!font || !font->isLoaded()) {
        return;
    }
    
    std::vector<std::string> lines = wordWrap ? wrapText(text) : std::vector<std::string>{text};
    
    Math::Vector2D currentPos = position;
    
    for (const auto& line : lines) {
        float lineWidth = getLineWidth(line);
        Math::Vector2D linePos = currentPos;
        
        switch (alignment) {
            case TextAlignment::Center:
                linePos.x += (maxWidth - lineWidth) / 2.0f;
                break;
            case TextAlignment::Right:
                linePos.x += maxWidth - lineWidth;
                break;
            case TextAlignment::Justify:
                // Justify would require spacing calculation
                break;
            default:
                break;
        }
        
        // Draw the line (would use TextRenderer here)
        
        currentPos.y += getLineHeight();
    }
}

float FormattedTextRenderer::getLineHeight() const {
    return font ? font->getLineHeight() : 16.0f;
}

Math::Vector2D FormattedTextRenderer::measureText(const std::string& text) const {
    if (!font) {
        return Math::Vector2D(0, 0);
    }
    
    std::vector<std::string> lines = wordWrap ? wrapText(text) : std::vector<std::string>{text};
    
    float maxLineWidth = 0.0f;
    for (const auto& line : lines) {
        maxLineWidth = std::max(maxLineWidth, getLineWidth(line));
    }
    
    return Math::Vector2D(maxLineWidth, getLineHeight() * lines.size());
}

std::vector<std::string> FormattedTextRenderer::wrapText(const std::string& text) const {
    std::vector<std::string> lines;
    std::istringstream stream(text);
    std::string word;
    std::string currentLine;
    
    while (stream >> word) {
        std::string testLine = currentLine.empty() ? word : currentLine + " " + word;
        float lineWidth = getLineWidth(testLine);
        
        if (lineWidth > maxWidth && !currentLine.empty()) {
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

float FormattedTextRenderer::getLineWidth(const std::string& line) const {
    if (!font) {
        return 0.0f;
    }
    return font->measureText(line).x;
}

// TextBatch implementation
TextBatch::TextBatch()
    : batching(false) {}

TextBatch::~TextBatch() {}

void TextBatch::begin() {
    batching = true;
    vertices.clear();
    indices.clear();
}

void TextBatch::end() {
    flush();
    batching = false;
}

void TextBatch::drawText(std::shared_ptr<Font> font, const std::string& text,
                        const Math::Vector2D& position, const Color& color) {
    if (!font || !font->isLoaded()) {
        return;
    }
    
    Math::Vector2D currentPos = position;
    
    std::wstring_convert<std::codecvt_utf8<char32_t>, char32_t> converter;
    std::u32string u32text = converter.from_bytes(text);
    
    for (char32_t c : u32text) {
        const Glyph* glyph = font->getGlyph(c);
        if (glyph) {
            addQuad(currentPos, glyph->size, glyph->uvMin, glyph->uvMax, color);
            currentPos.x += glyph->advance;
        }
    }
}

void TextBatch::flush() {
    // Render all batched text
    vertices.clear();
    indices.clear();
}

void TextBatch::clear() {
    vertices.clear();
    indices.clear();
}

void TextBatch::addQuad(const Math::Vector2D& pos, const Math::Vector2D& size,
                       const Math::Vector2D& uvMin, const Math::Vector2D& uvMax,
                       const Color& color) {
    uint32_t baseIndex = static_cast<uint32_t>(vertices.size());
    
    TextVertex v0 = {{pos.x, pos.y}, {uvMin.x, uvMin.y}, color};
    TextVertex v1 = {{pos.x + size.x, pos.y}, {uvMax.x, uvMin.y}, color};
    TextVertex v2 = {{pos.x + size.x, pos.y + size.y}, {uvMax.x, uvMax.y}, color};
    TextVertex v3 = {{pos.x, pos.y + size.y}, {uvMin.x, uvMax.y}, color};
    
    vertices.push_back(v0);
    vertices.push_back(v1);
    vertices.push_back(v2);
    vertices.push_back(v3);
    
    indices.push_back(baseIndex);
    indices.push_back(baseIndex + 1);
    indices.push_back(baseIndex + 2);
    indices.push_back(baseIndex);
    indices.push_back(baseIndex + 2);
    indices.push_back(baseIndex + 3);
}

// RichTextRenderer implementation
RichTextRenderer::RichTextRenderer()
    : defaultColor(255, 255, 255, 255) {}

RichTextRenderer::~RichTextRenderer() {}

void RichTextRenderer::setDefaultFont(std::shared_ptr<Font> font) {
    defaultFont = font;
}

void RichTextRenderer::drawRichText(const std::string& richText, const Math::Vector2D& position) {
    auto segments = parseRichText(richText);
    
    Math::Vector2D currentPos = position;
    
    for (const auto& segment : segments) {
        // Draw each segment with its formatting
        // In a real implementation, would use TextRenderer with appropriate font/color
    }
}

void RichTextRenderer::registerTag(const std::string& tag,
                                  std::function<void(const std::string&)> handler) {
    tagHandlers[tag] = handler;
}

std::vector<RichTextRenderer::TextSegment> RichTextRenderer::parseRichText(const std::string& richText) {
    std::vector<TextSegment> segments;
    
    // Simple parser for tags like <color=#FF0000>text</color>
    // In a real implementation, this would be more sophisticated
    
    TextSegment segment;
    segment.text = richText;
    segment.color = defaultColor;
    segment.font = defaultFont;
    segments.push_back(segment);
    
    return segments;
}

// BMFontLoader implementation
std::shared_ptr<Font> BMFontLoader::loadBMFont(const std::string& fntPath) {
    BMFontInfo info;
    std::unordered_map<char32_t, Glyph> glyphs;
    
    if (!parseFNT(fntPath, info, glyphs)) {
        return nullptr;
    }
    
    auto font = std::make_shared<Font>();
    // Set font properties from parsed data
    
    return font;
}

bool BMFontLoader::parseFNT(const std::string& path, BMFontInfo& info,
                            std::unordered_map<char32_t, Glyph>& glyphs) {
    std::ifstream file(path);
    if (!file.is_open()) {
        return false;
    }
    
    std::string line;
    while (std::getline(file, line)) {
        // Parse .fnt format
        // info face="Arial" size=32 ...
        // char id=65 x=0 y=0 width=32 height=32 ...
    }
    
    return true;
}

} // namespace Graphics
} // namespace JJM
