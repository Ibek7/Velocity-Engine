#include "gui/GUISystem.h"
#include <algorithm>
#include <iostream>
#include <sstream>

namespace JJM {
namespace GUI {

// Static member initialization
uint32_t Widget::nextID = 1;
GUIManager* GUIManager::instance = nullptr;

// Widget implementation
Widget::Widget() 
    : id(nextID++), state(WidgetState::Normal), 
      visible(true), enabled(true), parent(nullptr) {}

void Widget::addChild(std::unique_ptr<Widget> child) {
    child->parent = this;
    children.push_back(std::move(child));
}

void Widget::removeChild(uint32_t childID) {
    children.erase(
        std::remove_if(children.begin(), children.end(),
            [childID](const std::unique_ptr<Widget>& child) {
                return child->getID() == childID;
            }),
        children.end()
    );
}

void Widget::update(float deltaTime) {
    if (!visible) return;
    
    for (auto& child : children) {
        child->update(deltaTime);
    }
}

void Widget::render() {
    if (!visible) return;
    
    // Render background
    Graphics::Color bgColor = getStateColor();
    // Mock rendering - would use actual renderer
    
    // Render border if needed
    if (style.borderWidth > 0) {
        // Mock border rendering
    }
    
    // Render children
    for (const auto& child : children) {
        child->render();
    }
}

bool Widget::handleEvent(const Input::InputEvent& event) {
    if (!visible || !enabled) return false;
    
    // Handle events for children first (reverse order for proper z-order)
    for (auto it = children.rbegin(); it != children.rend(); ++it) {
        if ((*it)->handleEvent(event)) {
            return true;
        }
    }
    
    return false;
}

Math::Vector2D Widget::getPreferredSize() const {
    return Math::Vector2D(100.0f, 30.0f); // Default size
}

void Widget::layout() {
    // Default layout does nothing
    for (auto& child : children) {
        child->layout();
    }
}

bool Widget::isPointInBounds(float x, float y) const {
    return bounds.contains(x, y);
}

Graphics::Color Widget::getStateColor() const {
    switch (state) {
        case WidgetState::Hovered: return style.hoverColor;
        case WidgetState::Pressed: return style.pressedColor;
        case WidgetState::Disabled: return style.disabledColor;
        default: return style.backgroundColor;
    }
}

// Button implementation
Button::Button(const std::string& text) : text(text), isPressed(false) {
    style.padding = 12.0f;
}

void Button::render() {
    if (!visible) return;
    
    Graphics::Color buttonColor = getStateColor();
    
    // Mock button rendering
    // Would render rounded rectangle with text centered
    std::cout << "Rendering button: " << text << " at (" 
              << bounds.x << ", " << bounds.y << ") "
              << bounds.width << "x" << bounds.height << std::endl;
    
    Widget::render();
}

bool Button::handleEvent(const Input::InputEvent& event) {
    if (!visible || !enabled) return false;
    
    if (Widget::handleEvent(event)) return true;
    
    // Mock event handling
    Math::Vector2D mousePos(0, 0); // Would get from event
    
    if (isPointInBounds(mousePos.x, mousePos.y)) {
        if (event.type == Input::InputEventType::MouseButtonPressed) {
            setState(WidgetState::Pressed);
            isPressed = true;
            onPress();
            return true;
        } else if (event.type == Input::InputEventType::MouseButtonReleased && isPressed) {
            setState(WidgetState::Hovered);
            isPressed = false;
            onRelease();
            if (onClick) onClick();
            return true;
        } else if (event.type == Input::InputEventType::MouseMoved) {
            if (state == WidgetState::Normal) {
                setState(WidgetState::Hovered);
            }
        }
    } else {
        if (state == WidgetState::Hovered || state == WidgetState::Pressed) {
            setState(WidgetState::Normal);
            isPressed = false;
        }
    }
    
    return false;
}

Math::Vector2D Button::getPreferredSize() const {
    // Calculate based on text size + padding
    float textWidth = text.length() * style.fontSize * 0.6f; // Rough estimate
    float textHeight = style.fontSize * 1.2f;
    
    return Math::Vector2D(
        textWidth + style.padding * 2,
        textHeight + style.padding * 2
    );
}

void Button::onPress() {
    std::cout << "Button pressed: " << text << std::endl;
}

void Button::onRelease() {
    std::cout << "Button released: " << text << std::endl;
}

// Label implementation
Label::Label(const std::string& text) : text(text), textAlignment(Alignment::Left) {}

void Label::render() {
    if (!visible) return;
    
    Math::Vector2D textPos = getTextPosition();
    
    // Mock text rendering
    std::cout << "Rendering label: " << text << " at (" 
              << textPos.x << ", " << textPos.y << ")" << std::endl;
    
    Widget::render();
}

Math::Vector2D Label::getPreferredSize() const {
    float textWidth = text.length() * style.fontSize * 0.6f;
    float textHeight = style.fontSize * 1.2f;
    
    return Math::Vector2D(textWidth, textHeight);
}

Math::Vector2D Label::getTextPosition() const {
    Math::Vector2D size = getPreferredSize();
    Math::Vector2D pos(bounds.x, bounds.y);
    
    switch (textAlignment) {
        case Alignment::Center:
        case Alignment::MiddleCenter:
            pos.x += (bounds.width - size.x) * 0.5f;
            pos.y += (bounds.height - size.y) * 0.5f;
            break;
        case Alignment::Right:
        case Alignment::MiddleRight:
            pos.x += bounds.width - size.x;
            pos.y += (bounds.height - size.y) * 0.5f;
            break;
        default: // Left alignment
            pos.y += (bounds.height - size.y) * 0.5f;
            break;
    }
    
    return pos;
}

// TextBox implementation
TextBox::TextBox(const std::string& placeholder) 
    : placeholder(placeholder), cursorPosition(0), selectionStart(0), 
      selectionEnd(0), isEditing(false), cursorBlinkTime(0.0f), showCursor(true) {}

void TextBox::setText(const std::string& newText) {
    text = newText;
    cursorPosition = std::min(cursorPosition, text.length());
    if (onTextChanged) onTextChanged(text);
}

void TextBox::update(float deltaTime) {
    Widget::update(deltaTime);
    
    if (isEditing) {
        cursorBlinkTime += deltaTime;
        if (cursorBlinkTime >= 0.5f) {
            showCursor = !showCursor;
            cursorBlinkTime = 0.0f;
        }
    }
}

void TextBox::render() {
    if (!visible) return;
    
    // Render background and border
    Graphics::Color bgColor = getStateColor();
    
    // Render text or placeholder
    std::string displayText = text.empty() ? placeholder : text;
    
    std::cout << "Rendering textbox: " << displayText << " at (" 
              << bounds.x << ", " << bounds.y << ")" << std::endl;
    
    // Render cursor if editing
    if (isEditing && showCursor) {
        float cursorX = bounds.x + style.padding + cursorPosition * style.fontSize * 0.6f;
        std::cout << "Cursor at x=" << cursorX << std::endl;
    }
    
    Widget::render();
}

bool TextBox::handleEvent(const Input::InputEvent& event) {
    if (!visible || !enabled) return false;
    
    if (Widget::handleEvent(event)) return true;
    
    Math::Vector2D mousePos(0, 0); // Would get from event
    
    if (isPointInBounds(mousePos.x, mousePos.y)) {
        if (event.type == Input::InputEventType::MouseButtonPressed) {
            onFocus();
            return true;
        }
    } else if (event.type == Input::InputEventType::MouseButtonPressed) {
        onBlur();
    }
    
    if (isEditing && event.type == Input::InputEventType::KeyPressed) {
        // Mock keyboard handling
        std::string keyName = ""; // Would get from event
        
        if (keyName == "Backspace") {
            if (cursorPosition > 0) {
                text.erase(cursorPosition - 1, 1);
                cursorPosition--;
                if (onTextChanged) onTextChanged(text);
            }
        } else if (keyName == "Delete") {
            if (cursorPosition < text.length()) {
                text.erase(cursorPosition, 1);
                if (onTextChanged) onTextChanged(text);
            }
        } else if (keyName == "Left") {
            moveCursor(-1);
        } else if (keyName == "Right") {
            moveCursor(1);
        } else if (keyName.length() == 1) {
            insertText(keyName);
        }
        
        return true;
    }
    
    return false;
}

Math::Vector2D TextBox::getPreferredSize() const {
    return Math::Vector2D(200.0f, style.fontSize * 1.8f + style.padding * 2);
}

void TextBox::onFocus() {
    isEditing = true;
    setState(WidgetState::Focused);
    showCursor = true;
    cursorBlinkTime = 0.0f;
}

void TextBox::onBlur() {
    isEditing = false;
    setState(WidgetState::Normal);
}

void TextBox::insertText(const std::string& newText) {
    text.insert(cursorPosition, newText);
    cursorPosition += newText.length();
    if (onTextChanged) onTextChanged(text);
}

void TextBox::moveCursor(int delta) {
    cursorPosition = std::max(0, std::min(static_cast<int>(text.length()), 
                                         static_cast<int>(cursorPosition) + delta));
    showCursor = true;
    cursorBlinkTime = 0.0f;
}

// Panel implementation
Panel::Panel() : clipChildren(true) {}

void Panel::render() {
    if (!visible) return;
    
    // Render panel background
    Graphics::Color bgColor = getStateColor();
    
    std::cout << "Rendering panel at (" << bounds.x << ", " << bounds.y << ") "
              << bounds.width << "x" << bounds.height << std::endl;
    
    // Set clipping if enabled
    if (clipChildren) {
        // Mock clipping setup
    }
    
    Widget::render();
    
    if (clipChildren) {
        // Mock clipping cleanup
    }
}

void Panel::layout() {
    // Apply layout manager if available
    Widget::layout();
}

bool Panel::handleEvent(const Input::InputEvent& event) {
    return Widget::handleEvent(event);
}

// FlowLayout implementation
FlowLayout::FlowLayout(LayoutDirection dir, float spacing) 
    : direction(dir), spacing(spacing), alignment(Alignment::TopLeft) {}

void FlowLayout::layoutChildren(Widget* parent) {
    if (!parent) return;
    
    const auto& children = parent->getChildren();
    const Rect& parentBounds = parent->getBounds();
    const Style& parentStyle = parent->getStyle();
    
    float currentX = parentBounds.x + parentStyle.padding;
    float currentY = parentBounds.y + parentStyle.padding;
    float maxHeight = 0.0f;
    float maxWidth = 0.0f;
    
    for (const auto& child : children) {
        if (!child->isVisible()) continue;
        
        Math::Vector2D childSize = child->getPreferredSize();
        
        if (direction == LayoutDirection::Horizontal) {
            // Check if we need to wrap to next line
            if (currentX + childSize.x > parentBounds.x + parentBounds.width - parentStyle.padding) {
                currentX = parentBounds.x + parentStyle.padding;
                currentY += maxHeight + spacing;
                maxHeight = 0.0f;
            }
            
            child->setBounds(currentX, currentY, childSize.x, childSize.y);
            currentX += childSize.x + spacing;
            maxHeight = std::max(maxHeight, childSize.y);
        } else {
            // Vertical layout
            child->setBounds(currentX, currentY, childSize.x, childSize.y);
            currentY += childSize.y + spacing;
            maxWidth = std::max(maxWidth, childSize.x);
        }
        
        child->layout();
    }
}

Math::Vector2D FlowLayout::calculatePreferredSize(const Widget* parent) const {
    if (!parent) return Math::Vector2D(0, 0);
    
    const auto& children = parent->getChildren();
    const Style& parentStyle = parent->getStyle();
    
    float totalWidth = 0.0f;
    float totalHeight = 0.0f;
    float currentLineWidth = 0.0f;
    float currentLineHeight = 0.0f;
    
    for (const auto& child : children) {
        if (!child->isVisible()) continue;
        
        Math::Vector2D childSize = child->getPreferredSize();
        
        if (direction == LayoutDirection::Horizontal) {
            currentLineWidth += childSize.x + spacing;
            currentLineHeight = std::max(currentLineHeight, childSize.y);
            totalWidth = std::max(totalWidth, currentLineWidth);
            totalHeight += currentLineHeight;
        } else {
            totalWidth = std::max(totalWidth, childSize.x);
            totalHeight += childSize.y + spacing;
        }
    }
    
    return Math::Vector2D(
        totalWidth + parentStyle.padding * 2,
        totalHeight + parentStyle.padding * 2
    );
}

// Theme implementation
Theme::Theme() {
    applyDarkTheme();
}

void Theme::applyDarkTheme() {
    primaryColor = Graphics::Color(0.2f, 0.4f, 0.8f, 1.0f);
    secondaryColor = Graphics::Color(0.3f, 0.3f, 0.3f, 1.0f);
    accentColor = Graphics::Color(0.8f, 0.4f, 0.2f, 1.0f);
    backgroundColor = Graphics::Color(0.1f, 0.1f, 0.1f, 1.0f);
    textColor = Graphics::Color(0.9f, 0.9f, 0.9f, 1.0f);
    
    defaultStyle.backgroundColor = backgroundColor;
    defaultStyle.textColor = textColor;
    
    buttonStyle = defaultStyle;
    buttonStyle.backgroundColor = primaryColor;
    buttonStyle.hoverColor = Graphics::Color(0.3f, 0.5f, 0.9f, 1.0f);
    buttonStyle.pressedColor = Graphics::Color(0.1f, 0.3f, 0.7f, 1.0f);
    
    labelStyle = defaultStyle;
    labelStyle.backgroundColor = Graphics::Color(0, 0, 0, 0); // Transparent
    
    textBoxStyle = defaultStyle;
    textBoxStyle.backgroundColor = Graphics::Color(0.2f, 0.2f, 0.2f, 1.0f);
    textBoxStyle.borderColor = primaryColor;
    
    panelStyle = defaultStyle;
    panelStyle.backgroundColor = secondaryColor;
}

void Theme::applyLightTheme() {
    primaryColor = Graphics::Color(0.2f, 0.4f, 0.8f, 1.0f);
    secondaryColor = Graphics::Color(0.9f, 0.9f, 0.9f, 1.0f);
    accentColor = Graphics::Color(0.8f, 0.4f, 0.2f, 1.0f);
    backgroundColor = Graphics::Color(1.0f, 1.0f, 1.0f, 1.0f);
    textColor = Graphics::Color(0.1f, 0.1f, 0.1f, 1.0f);
    
    // Update styles for light theme...
}

// GUIManager implementation
GUIManager::GUIManager() 
    : hoveredWidget(nullptr), focusedWidget(nullptr), draggedWidget(nullptr),
      mousePressed(false), inputManager(nullptr) {}

GUIManager* GUIManager::getInstance() {
    if (!instance) {
        instance = new GUIManager();
    }
    return instance;
}

GUIManager::~GUIManager() {
    shutdown();
}

void GUIManager::initialize(Input::InputManager* input) {
    inputManager = input;
    defaultLayout = std::make_unique<FlowLayout>();
}

void GUIManager::shutdown() {
    rootWidgets.clear();
    widgetRegistry.clear();
    defaultLayout.reset();
}

void GUIManager::addWidget(std::unique_ptr<Widget> widget) {
    registerWidget(widget.get());
    rootWidgets.push_back(std::move(widget));
}

void GUIManager::removeWidget(uint32_t widgetID) {
    auto it = std::find_if(rootWidgets.begin(), rootWidgets.end(),
        [widgetID](const std::unique_ptr<Widget>& widget) {
            return widget->getID() == widgetID;
        });
    
    if (it != rootWidgets.end()) {
        unregisterWidget(it->get());
        rootWidgets.erase(it);
    }
}

Widget* GUIManager::getWidget(uint32_t widgetID) {
    auto it = widgetRegistry.find(widgetID);
    return (it != widgetRegistry.end()) ? it->second : nullptr;
}

void GUIManager::setTheme(const Theme& theme) {
    currentTheme = theme;
    
    // Apply theme to all widgets
    for (auto& widget : rootWidgets) {
        // Would recursively apply theme styles
    }
}

void GUIManager::setFocus(Widget* widget) {
    if (focusedWidget) {
        focusedWidget->setState(WidgetState::Normal);
    }
    
    focusedWidget = widget;
    
    if (focusedWidget) {
        focusedWidget->setState(WidgetState::Focused);
    }
}

void GUIManager::update(float deltaTime) {
    // Update all widgets
    for (auto& widget : rootWidgets) {
        widget->update(deltaTime);
    }
    
    updateWidgetStates();
    executeDeferredActions();
}

void GUIManager::render() {
    // Render all root widgets (they will render their children)
    for (const auto& widget : rootWidgets) {
        widget->render();
    }
}

void GUIManager::handleEvent(const Input::InputEvent& event) {
    // Handle events in reverse order for proper z-order
    for (auto it = rootWidgets.rbegin(); it != rootWidgets.rend(); ++it) {
        if ((*it)->handleEvent(event)) {
            return; // Event consumed
        }
    }
}

void GUIManager::deferAction(std::function<void()> action) {
    deferredActions.push(action);
}

void GUIManager::updateWidgetStates() {
    // Mock mouse position update
    lastMousePosition = mousePosition;
    // mousePosition = inputManager->getMousePosition();
    
    processMouseEvents();
}

void GUIManager::processMouseEvents() {
    Widget* widgetUnderMouse = findWidgetAt(mousePosition.x, mousePosition.y);
    
    // Update hovered widget
    if (widgetUnderMouse != hoveredWidget) {
        if (hoveredWidget) {
            hoveredWidget->setState(WidgetState::Normal);
        }
        
        hoveredWidget = widgetUnderMouse;
        
        if (hoveredWidget && hoveredWidget->isEnabled()) {
            hoveredWidget->setState(WidgetState::Hovered);
        }
    }
}

Widget* GUIManager::findWidgetAt(float x, float y) {
    // Search in reverse order for proper z-order
    for (auto it = rootWidgets.rbegin(); it != rootWidgets.rend(); ++it) {
        // Would recursively search through widget hierarchy
        if ((*it)->isVisible() && (*it)->getBounds().contains(x, y)) {
            return it->get();
        }
    }
    
    return nullptr;
}

void GUIManager::executeDeferredActions() {
    while (!deferredActions.empty()) {
        deferredActions.front()();
        deferredActions.pop();
    }
}

void GUIManager::registerWidget(Widget* widget) {
    widgetRegistry[widget->getID()] = widget;
    
    // Recursively register children
    for (const auto& child : widget->getChildren()) {
        registerWidget(child.get());
    }
}

void GUIManager::unregisterWidget(Widget* widget) {
    // Recursively unregister children
    for (const auto& child : widget->getChildren()) {
        unregisterWidget(child.get());
    }
    
    widgetRegistry.erase(widget->getID());
}

// IMGUI immediate-mode functions
namespace IMGUI {
    static GUIManager* gui = nullptr;
    static Math::Vector2D currentPosition(10, 10);
    static Math::Vector2D nextPosition(-1, -1);
    static Math::Vector2D nextSize(-1, -1);
    static std::vector<Style> styleStack;
    static bool sameLine = false;
    
    void beginFrame() {
        gui = GUIManager::getInstance();
        currentPosition = Math::Vector2D(10, 10);
        sameLine = false;
    }
    
    void endFrame() {
        // Frame cleanup
    }
    
    bool button(const std::string& text, float x, float y, float width, float height) {
        if (x >= 0) currentPosition.x = x;
        if (y >= 0) currentPosition.y = y;
        
        if (nextPosition.x >= 0) {
            currentPosition = nextPosition;
            nextPosition = Math::Vector2D(-1, -1);
        }
        
        auto button = std::make_unique<Button>(text);
        
        Math::Vector2D size = button->getPreferredSize();
        if (width > 0) size.x = width;
        if (height > 0) size.y = height;
        
        if (nextSize.x > 0) {
            size = nextSize;
            nextSize = Math::Vector2D(-1, -1);
        }
        
        button->setBounds(currentPosition.x, currentPosition.y, size.x, size.y);
        
        bool clicked = false;
        button->setOnClick([&clicked]() { clicked = true; });
        
        // Mock immediate rendering and input handling
        button->render();
        
        if (!sameLine) {
            currentPosition.y += size.y + 8.0f;
        } else {
            currentPosition.x += size.x + 8.0f;
            sameLine = false;
        }
        
        return clicked;
    }
    
    void label(const std::string& text, float x, float y) {
        if (x >= 0) currentPosition.x = x;
        if (y >= 0) currentPosition.y = y;
        
        auto label = std::make_unique<Label>(text);
        Math::Vector2D size = label->getPreferredSize();
        
        label->setBounds(currentPosition.x, currentPosition.y, size.x, size.y);
        label->render();
        
        if (!sameLine) {
            currentPosition.y += size.y + 4.0f;
        } else {
            currentPosition.x += size.x + 8.0f;
            sameLine = false;
        }
    }
    
    void setNextPosition(float x, float y) {
        nextPosition = Math::Vector2D(x, y);
    }
    
    void setNextSize(float width, float height) {
        nextSize = Math::Vector2D(width, height);
    }
    
    void sameLine() {
        sameLine = true;
    }
    
    void newLine() {
        currentPosition.x = 10.0f;
        currentPosition.y += 25.0f;
    }
    
    void separator() {
        currentPosition.y += 10.0f;
        // Draw line
        currentPosition.y += 10.0f;
    }
    
    void spacing(float amount) {
        currentPosition.y += amount;
    }
}

} // namespace GUI
} // namespace JJM