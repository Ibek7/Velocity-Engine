#ifndef GUI_SYSTEM_H
#define GUI_SYSTEM_H

#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <memory>
#include <queue>
#include "math/Vector2D.h"
#include "graphics/Color.h"
#include "input/InputManager.h"

namespace JJM {
namespace GUI {

struct Rect {
    float x, y, width, height;
    
    Rect() : x(0), y(0), width(0), height(0) {}
    Rect(float x, float y, float w, float h) : x(x), y(y), width(w), height(h) {}
    
    bool contains(float px, float py) const {
        return px >= x && px < x + width && py >= y && py < y + height;
    }
    
    bool contains(const Math::Vector2D& point) const {
        return contains(point.x, point.y);
    }
};

enum class WidgetState {
    Normal,
    Hovered,
    Pressed,
    Disabled,
    Focused
};

enum class Alignment {
    Left,
    Center,
    Right,
    Top,
    Middle,
    Bottom,
    TopLeft,
    TopCenter,
    TopRight,
    MiddleLeft,
    MiddleCenter,
    MiddleRight,
    BottomLeft,
    BottomCenter,
    BottomRight
};

enum class LayoutDirection {
    Horizontal,
    Vertical
};

struct Style {
    Graphics::Color backgroundColor;
    Graphics::Color borderColor;
    Graphics::Color textColor;
    Graphics::Color hoverColor;
    Graphics::Color pressedColor;
    Graphics::Color disabledColor;
    
    float borderWidth;
    float cornerRadius;
    float padding;
    float margin;
    
    std::string fontFamily;
    int fontSize;
    bool fontBold;
    bool fontItalic;
    
    Style() 
        : backgroundColor(0.2f, 0.2f, 0.2f, 1.0f),
          borderColor(0.5f, 0.5f, 0.5f, 1.0f),
          textColor(1.0f, 1.0f, 1.0f, 1.0f),
          hoverColor(0.3f, 0.3f, 0.3f, 1.0f),
          pressedColor(0.1f, 0.1f, 0.1f, 1.0f),
          disabledColor(0.15f, 0.15f, 0.15f, 0.5f),
          borderWidth(1.0f), cornerRadius(4.0f),
          padding(8.0f), margin(4.0f),
          fontFamily("Arial"), fontSize(14),
          fontBold(false), fontItalic(false) {}
};

class Widget {
protected:
    static uint32_t nextID;
    uint32_t id;
    Rect bounds;
    WidgetState state;
    bool visible;
    bool enabled;
    Style style;
    std::string tooltip;
    
    Widget* parent;
    std::vector<std::unique_ptr<Widget>> children;
    
public:
    Widget();
    virtual ~Widget() = default;
    
    uint32_t getID() const { return id; }
    const Rect& getBounds() const { return bounds; }
    void setBounds(const Rect& rect) { bounds = rect; }
    void setBounds(float x, float y, float w, float h) { bounds = Rect(x, y, w, h); }
    
    WidgetState getState() const { return state; }
    void setState(WidgetState newState) { state = newState; }
    
    bool isVisible() const { return visible; }
    void setVisible(bool vis) { visible = vis; }
    
    bool isEnabled() const { return enabled; }
    void setEnabled(bool en) { enabled = en; }
    
    const Style& getStyle() const { return style; }
    void setStyle(const Style& newStyle) { style = newStyle; }
    
    void setTooltip(const std::string& text) { tooltip = text; }
    const std::string& getTooltip() const { return tooltip; }
    
    void addChild(std::unique_ptr<Widget> child);
    void removeChild(uint32_t childID);
    const std::vector<std::unique_ptr<Widget>>& getChildren() const { return children; }
    
    virtual void update(float deltaTime);
    virtual void render();
    virtual bool handleEvent(const Input::InputEvent& event);
    virtual Math::Vector2D getPreferredSize() const;
    virtual void layout();
    
protected:
    virtual void onHover() {}
    virtual void onPress() {}
    virtual void onRelease() {}
    virtual void onFocus() {}
    virtual void onBlur() {}
    virtual void onStateChange(WidgetState oldState, WidgetState newState) {}
    
    bool isPointInBounds(float x, float y) const;
    Graphics::Color getStateColor() const;
};

class Button : public Widget {
private:
    std::string text;
    std::function<void()> onClick;
    bool isPressed;
    
public:
    Button(const std::string& text = "");
    
    void setText(const std::string& newText) { text = newText; }
    const std::string& getText() const { return text; }
    
    void setOnClick(std::function<void()> callback) { onClick = callback; }
    
    void render() override;
    bool handleEvent(const Input::InputEvent& event) override;
    Math::Vector2D getPreferredSize() const override;
    
protected:
    void onPress() override;
    void onRelease() override;
};

class Label : public Widget {
private:
    std::string text;
    Alignment textAlignment;
    
public:
    Label(const std::string& text = "");
    
    void setText(const std::string& newText) { text = newText; }
    const std::string& getText() const { return text; }
    
    void setTextAlignment(Alignment align) { textAlignment = align; }
    Alignment getTextAlignment() const { return textAlignment; }
    
    void render() override;
    Math::Vector2D getPreferredSize() const override;
    
private:
    Math::Vector2D getTextPosition() const;
};

class TextBox : public Widget {
private:
    std::string text;
    std::string placeholder;
    size_t cursorPosition;
    size_t selectionStart;
    size_t selectionEnd;
    bool isEditing;
    float cursorBlinkTime;
    bool showCursor;
    std::function<void(const std::string&)> onTextChanged;
    
public:
    TextBox(const std::string& placeholder = "");
    
    void setText(const std::string& newText);
    const std::string& getText() const { return text; }
    
    void setPlaceholder(const std::string& ph) { placeholder = ph; }
    const std::string& getPlaceholder() const { return placeholder; }
    
    void setOnTextChanged(std::function<void(const std::string&)> callback) { onTextChanged = callback; }
    
    void update(float deltaTime) override;
    void render() override;
    bool handleEvent(const Input::InputEvent& event) override;
    Math::Vector2D getPreferredSize() const override;
    
protected:
    void onFocus() override;
    void onBlur() override;
    
private:
    void insertText(const std::string& newText);
    void deleteSelection();
    void moveCursor(int delta);
    void selectAll();
    void copy();
    void paste();
    void cut();
};

class Panel : public Widget {
private:
    bool clipChildren;
    
public:
    Panel();
    
    void setClipChildren(bool clip) { clipChildren = clip; }
    bool getClipChildren() const { return clipChildren; }
    
    void render() override;
    void layout() override;
    bool handleEvent(const Input::InputEvent& event) override;
};

class LayoutManager {
public:
    virtual ~LayoutManager() = default;
    virtual void layoutChildren(Widget* parent) = 0;
    virtual Math::Vector2D calculatePreferredSize(const Widget* parent) const = 0;
};

class FlowLayout : public LayoutManager {
private:
    LayoutDirection direction;
    float spacing;
    Alignment alignment;
    
public:
    FlowLayout(LayoutDirection dir = LayoutDirection::Horizontal, float spacing = 4.0f);
    
    void setDirection(LayoutDirection dir) { direction = dir; }
    void setSpacing(float space) { spacing = space; }
    void setAlignment(Alignment align) { alignment = align; }
    
    void layoutChildren(Widget* parent) override;
    Math::Vector2D calculatePreferredSize(const Widget* parent) const override;
};

class GridLayout : public LayoutManager {
private:
    int rows;
    int columns;
    float horizontalSpacing;
    float verticalSpacing;
    
public:
    GridLayout(int rows, int cols, float hSpacing = 4.0f, float vSpacing = 4.0f);
    
    void setGridSize(int rows, int cols) { this->rows = rows; this->columns = cols; }
    void setSpacing(float hSpacing, float vSpacing) { 
        horizontalSpacing = hSpacing; 
        verticalSpacing = vSpacing; 
    }
    
    void layoutChildren(Widget* parent) override;
    Math::Vector2D calculatePreferredSize(const Widget* parent) const override;
};

class BorderLayout : public LayoutManager {
public:
    enum class Region {
        North,
        South,
        East,
        West,
        Center
    };
    
private:
    std::unordered_map<Region, Widget*> regions;
    float spacing;
    
public:
    BorderLayout(float spacing = 4.0f);
    
    void addWidget(Widget* widget, Region region);
    void removeWidget(Region region);
    
    void layoutChildren(Widget* parent) override;
    Math::Vector2D calculatePreferredSize(const Widget* parent) const override;
};

struct Theme {
    Style defaultStyle;
    Style buttonStyle;
    Style labelStyle;
    Style textBoxStyle;
    Style panelStyle;
    
    Graphics::Color primaryColor;
    Graphics::Color secondaryColor;
    Graphics::Color accentColor;
    Graphics::Color backgroundColor;
    Graphics::Color textColor;
    
    Theme();
    void applyDarkTheme();
    void applyLightTheme();
    void applyCustomTheme(const Graphics::Color& primary, const Graphics::Color& secondary);
};

class GUIManager {
private:
    static GUIManager* instance;
    
    std::vector<std::unique_ptr<Widget>> rootWidgets;
    std::unordered_map<uint32_t, Widget*> widgetRegistry;
    std::unique_ptr<LayoutManager> defaultLayout;
    
    Widget* hoveredWidget;
    Widget* focusedWidget;
    Widget* draggedWidget;
    
    Theme currentTheme;
    
    Math::Vector2D mousePosition;
    Math::Vector2D lastMousePosition;
    bool mousePressed;
    
    std::queue<std::function<void()>> deferredActions;
    
    Input::InputManager* inputManager;
    
    GUIManager();
    
public:
    static GUIManager* getInstance();
    ~GUIManager();
    
    void initialize(Input::InputManager* input);
    void shutdown();
    
    void addWidget(std::unique_ptr<Widget> widget);
    void removeWidget(uint32_t widgetID);
    Widget* getWidget(uint32_t widgetID);
    
    void setDefaultLayout(std::unique_ptr<LayoutManager> layout);
    
    void setTheme(const Theme& theme);
    const Theme& getTheme() const { return currentTheme; }
    
    void setFocus(Widget* widget);
    Widget* getFocusedWidget() const { return focusedWidget; }
    
    void update(float deltaTime);
    void render();
    void handleEvent(const Input::InputEvent& event);
    
    void deferAction(std::function<void()> action);
    
    Math::Vector2D getMousePosition() const { return mousePosition; }
    
private:
    void updateWidgetStates();
    void processMouseEvents();
    Widget* findWidgetAt(float x, float y);
    void executeDeferredActions();
    void registerWidget(Widget* widget);
    void unregisterWidget(Widget* widget);
};

// Helper functions for immediate-mode GUI
namespace IMGUI {
    void beginFrame();
    void endFrame();
    
    bool button(const std::string& text, float x, float y, float width = 0, float height = 0);
    void label(const std::string& text, float x, float y);
    bool textBox(std::string& text, float x, float y, float width = 200, float height = 30);
    
    void pushStyle(const Style& style);
    void popStyle();
    
    void setNextPosition(float x, float y);
    void setNextSize(float width, float height);
    void sameLine();
    void newLine();
    void separator();
    void spacing(float amount);
    
    bool beginPanel(const std::string& title, float x, float y, float width, float height);
    void endPanel();
    
    bool beginWindow(const std::string& title, bool* open = nullptr);
    void endWindow();
}

} // namespace GUI
} // namespace JJM

#endif // GUI_SYSTEM_H