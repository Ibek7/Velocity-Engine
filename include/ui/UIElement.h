#ifndef UI_ELEMENT_H
#define UI_ELEMENT_H

#include "math/Vector2D.h"
#include "graphics/Color.h"
#include "graphics/Renderer.h"
#include "input/InputManager.h"
#include <string>
#include <vector>
#include <functional>
#include <memory>
#include <unordered_map>
#include <optional>

namespace JJM {
namespace UI {

// =============================================================================
// Accessibility - Forward Declarations
// =============================================================================

class AccessibilityInfo;
class FocusManager;
class ScreenReaderInterface;

// =============================================================================
// Accessibility Types
// =============================================================================

/**
 * @brief Accessibility roles for UI elements
 */
enum class AccessibilityRole {
    None,
    Button,
    Checkbox,
    RadioButton,
    Slider,
    TextInput,
    Label,
    Link,
    Image,
    List,
    ListItem,
    Menu,
    MenuItem,
    Tab,
    TabPanel,
    Tooltip,
    Dialog,
    Alert,
    ProgressBar,
    ScrollArea,
    Separator,
    Toolbar,
    TreeView,
    TreeItem,
    Grid,
    GridCell,
    Header,
    Footer,
    Navigation,
    Main,
    Complementary,
    Custom
};

/**
 * @brief Accessibility states
 */
struct AccessibilityState {
    bool focused;
    bool selected;
    bool expanded;
    bool checked;
    bool disabled;
    bool readonly;
    bool required;
    bool invalid;
    bool busy;
    bool pressed;
    bool hidden;
    
    AccessibilityState()
        : focused(false)
        , selected(false)
        , expanded(false)
        , checked(false)
        , disabled(false)
        , readonly(false)
        , required(false)
        , invalid(false)
        , busy(false)
        , pressed(false)
        , hidden(false)
    {}
};

/**
 * @brief Live region types for dynamic content
 */
enum class LiveRegionType {
    Off,        // No announcements
    Polite,     // Announce when idle
    Assertive   // Interrupt current speech
};

/**
 * @brief High contrast mode settings
 */
enum class ContrastMode {
    Normal,
    High,
    HighBlackBackground,
    HighWhiteBackground,
    Custom
};

/**
 * @brief Accessibility information for a UI element
 */
class AccessibilityInfo {
public:
    AccessibilityRole role;
    std::string label;              // Accessible name
    std::string description;        // Accessible description
    std::string hint;               // Usage hint
    std::string value;              // Current value (for sliders, etc.)
    std::string valueDescription;   // Human-readable value description
    AccessibilityState state;
    LiveRegionType liveRegion;
    
    // Relationships
    std::string labelledBy;         // ID of labeling element
    std::string describedBy;        // ID of describing element
    std::string controlledBy;       // ID of controlling element
    std::string flowTo;             // ID of next element in reading order
    
    // Range values (for sliders, progress bars)
    std::optional<float> valueMin;
    std::optional<float> valueMax;
    std::optional<float> valueCurrent;
    
    // Additional properties
    int positionInSet;              // Position in a set (1-based)
    int setSize;                    // Size of the set
    int level;                      // Hierarchical level
    
    std::unordered_map<std::string, std::string> customProperties;
    
    AccessibilityInfo()
        : role(AccessibilityRole::None)
        , liveRegion(LiveRegionType::Off)
        , positionInSet(0)
        , setSize(0)
        , level(0)
    {}
    
    std::string getAnnouncementText() const;
    bool hasValue() const { return !value.empty() || valueCurrent.has_value(); }
};

/**
 * @brief Focus navigation direction
 */
enum class FocusDirection {
    Next,
    Previous,
    Up,
    Down,
    Left,
    Right,
    First,
    Last
};

/**
 * @brief Focus manager for keyboard navigation
 */
class FocusManager {
private:
    static FocusManager* instance;
    
    UIElement* currentFocus;
    std::vector<UIElement*> focusHistory;
    std::vector<UIElement*> focusableElements;
    bool trapFocus;
    UIElement* focusTrapContainer;
    
    // Tab index tracking
    std::map<int, std::vector<UIElement*>> tabIndexMap;
    
    // Focus styling
    Graphics::Color focusRingColor;
    float focusRingWidth;
    float focusRingOffset;
    bool showFocusRing;
    
    FocusManager();
    
public:
    static FocusManager* getInstance();
    static void cleanup();
    
    // Focus control
    void setFocus(UIElement* element);
    void clearFocus();
    UIElement* getFocusedElement() const { return currentFocus; }
    
    // Focus navigation
    bool moveFocus(FocusDirection direction);
    bool moveFocusToFirst();
    bool moveFocusToLast();
    
    // Focus registration
    void registerFocusable(UIElement* element, int tabIndex = 0);
    void unregisterFocusable(UIElement* element);
    void updateFocusOrder();
    
    // Focus trapping (for modals)
    void trapFocusIn(UIElement* container);
    void releaseFocusTrap();
    bool isFocusTrapped() const { return trapFocus; }
    
    // Focus history
    void pushFocusHistory();
    bool popFocusHistory();
    void clearFocusHistory();
    
    // Focus styling
    void setFocusRingColor(const Graphics::Color& color) { focusRingColor = color; }
    void setFocusRingWidth(float width) { focusRingWidth = width; }
    void setShowFocusRing(bool show) { showFocusRing = show; }
    void renderFocusRing(Graphics::Renderer* renderer);
    
    // Query
    bool isFocused(UIElement* element) const { return currentFocus == element; }
    bool hasFocus() const { return currentFocus != nullptr; }
};

/**
 * @brief Screen reader interface for accessibility
 */
class ScreenReaderInterface {
private:
    static ScreenReaderInterface* instance;
    
    bool enabled;
    bool available;
    std::string platformAPI;        // "MSAA", "UIA", "ATK", "NSAccessibility"
    
    std::queue<std::string> announcementQueue;
    LiveRegionType currentPriority;
    
    ScreenReaderInterface();
    
public:
    static ScreenReaderInterface* getInstance();
    static void cleanup();
    
    // Initialization
    bool initialize();
    bool isAvailable() const { return available; }
    bool isEnabled() const { return enabled; }
    void setEnabled(bool enable) { enabled = enable; }
    
    // Announcements
    void announce(const std::string& text, LiveRegionType priority = LiveRegionType::Polite);
    void announceImmediate(const std::string& text);
    void clearAnnouncements();
    
    // Element announcements
    void announceElement(UIElement* element);
    void announceFocusChange(UIElement* from, UIElement* to);
    void announceValueChange(UIElement* element, const std::string& oldValue, const std::string& newValue);
    void announceStateChange(UIElement* element, const std::string& stateName, bool newState);
    
    // Platform integration
    void* getNativeAccessibilityObject(UIElement* element);
    void updateAccessibilityTree(UIElement* root);
};

/**
 * @brief Accessibility theme for high contrast and color adjustments
 */
class AccessibilityTheme {
private:
    static AccessibilityTheme* instance;
    
    ContrastMode contrastMode;
    float textScaleFactor;
    bool reduceMotion;
    bool reduceTransparency;
    
    // Color overrides
    Graphics::Color textColor;
    Graphics::Color backgroundColor;
    Graphics::Color accentColor;
    Graphics::Color focusColor;
    Graphics::Color errorColor;
    Graphics::Color linkColor;
    Graphics::Color disabledColor;
    
    AccessibilityTheme();
    
public:
    static AccessibilityTheme* getInstance();
    static void cleanup();
    
    // Contrast mode
    void setContrastMode(ContrastMode mode);
    ContrastMode getContrastMode() const { return contrastMode; }
    bool isHighContrast() const { return contrastMode != ContrastMode::Normal; }
    
    // Text scaling
    void setTextScaleFactor(float factor) { textScaleFactor = factor; }
    float getTextScaleFactor() const { return textScaleFactor; }
    float scaleText(float baseSize) const { return baseSize * textScaleFactor; }
    
    // Motion reduction
    void setReduceMotion(bool reduce) { reduceMotion = reduce; }
    bool shouldReduceMotion() const { return reduceMotion; }
    
    // Transparency
    void setReduceTransparency(bool reduce) { reduceTransparency = reduce; }
    bool shouldReduceTransparency() const { return reduceTransparency; }
    
    // Color getters
    const Graphics::Color& getTextColor() const { return textColor; }
    const Graphics::Color& getBackgroundColor() const { return backgroundColor; }
    const Graphics::Color& getAccentColor() const { return accentColor; }
    const Graphics::Color& getFocusColor() const { return focusColor; }
    
    // Apply theme
    void applyToElement(UIElement* element);
    void loadSystemPreferences();
};

// =============================================================================
// UI Element Base Class
// =============================================================================

class UIElement {
protected:
    Math::Vector2D position;
    Math::Vector2D size;
    bool visible;
    bool enabled;
    std::string id;
    UIElement* parent;
    std::vector<std::unique_ptr<UIElement>> children;
    
    // Accessibility
    std::unique_ptr<AccessibilityInfo> accessibilityInfo;
    int tabIndex;
    bool focusable;
    
public:
    UIElement(const Math::Vector2D& pos, const Math::Vector2D& sz);
    virtual ~UIElement();
    
    virtual void update(float deltaTime);
    virtual void render(Graphics::Renderer* renderer);
    virtual void handleInput(const Input::InputManager& input);
    
    void addChild(std::unique_ptr<UIElement> child);
    void removeChild(UIElement* child);
    
    Math::Vector2D getAbsolutePosition() const;
    bool containsPoint(const Math::Vector2D& point) const;
    
    void setPosition(const Math::Vector2D& pos) { position = pos; }
    void setSize(const Math::Vector2D& sz) { size = sz; }
    void setVisible(bool vis);
    void setEnabled(bool en);
    void setId(const std::string& identifier) { id = identifier; }
    
    const Math::Vector2D& getPosition() const { return position; }
    const Math::Vector2D& getSize() const { return size; }
    bool isVisible() const { return visible; }
    bool isEnabled() const { return enabled; }
    const std::string& getId() const { return id; }
    
    // Accessibility methods
    void setAccessibilityLabel(const std::string& label);
    void setAccessibilityDescription(const std::string& description);
    void setAccessibilityHint(const std::string& hint);
    void setAccessibilityRole(AccessibilityRole role);
    void setAccessibilityValue(const std::string& value);
    void setAccessibilityState(const AccessibilityState& state);
    void setLiveRegion(LiveRegionType type);
    
    const AccessibilityInfo* getAccessibilityInfo() const { return accessibilityInfo.get(); }
    AccessibilityInfo* getAccessibilityInfo() { return accessibilityInfo.get(); }
    
    void setTabIndex(int index);
    int getTabIndex() const { return tabIndex; }
    
    void setFocusable(bool canFocus);
    bool isFocusable() const { return focusable && enabled && visible; }
    bool isFocused() const;
    void focus();
    void blur();
    
    // Keyboard event handlers (for accessibility)
    virtual bool onKeyDown(int keyCode);
    virtual bool onKeyUp(int keyCode);
    
    // Focus event handlers
    virtual void onFocus();
    virtual void onBlur();
    
protected:
    void initAccessibility();
    void announceToScreenReader(const std::string& text, LiveRegionType priority = LiveRegionType::Polite);
};

class Button : public UIElement {
private:
    std::string text;
    Graphics::Color normalColor;
    Graphics::Color hoverColor;
    Graphics::Color pressedColor;
    Graphics::Color textColor;
    
    bool isHovered;
    bool isPressed;
    
    std::function<void()> onClick;
    
public:
    Button(const Math::Vector2D& pos, const Math::Vector2D& size, const std::string& txt);
    
    void update(float deltaTime) override;
    void render(Graphics::Renderer* renderer) override;
    void handleInput(const Input::InputManager& input) override;
    
    void setText(const std::string& txt) { text = txt; }
    void setOnClick(const std::function<void()>& callback) { onClick = callback; }
    void setColors(const Graphics::Color& normal, const Graphics::Color& hover, const Graphics::Color& pressed);
};

class Label : public UIElement {
private:
    std::string text;
    Graphics::Color textColor;
    int fontSize;
    
public:
    Label(const Math::Vector2D& pos, const std::string& txt);
    
    void render(Graphics::Renderer* renderer) override;
    
    void setText(const std::string& txt) { text = txt; }
    void setTextColor(const Graphics::Color& color) { textColor = color; }
    void setFontSize(int size) { fontSize = size; }
};

class Panel : public UIElement {
private:
    Graphics::Color backgroundColor;
    Graphics::Color borderColor;
    int borderWidth;
    
public:
    Panel(const Math::Vector2D& pos, const Math::Vector2D& size);
    
    void render(Graphics::Renderer* renderer) override;
    
    void setBackgroundColor(const Graphics::Color& color) { backgroundColor = color; }
    void setBorderColor(const Graphics::Color& color) { borderColor = color; }
    void setBorderWidth(int width) { borderWidth = width; }
};

class Slider : public UIElement {
private:
    float value;
    float minValue;
    float maxValue;
    Graphics::Color trackColor;
    Graphics::Color thumbColor;
    bool isDragging;
    
    std::function<void(float)> onValueChanged;
    
public:
    Slider(const Math::Vector2D& pos, float width, float min, float max);
    
    void update(float deltaTime) override;
    void render(Graphics::Renderer* renderer) override;
    void handleInput(const Input::InputManager& input) override;
    
    void setValue(float val);
    float getValue() const { return value; }
    void setOnValueChanged(const std::function<void(float)>& callback) { onValueChanged = callback; }
};

class Checkbox : public UIElement {
private:
    bool checked;
    Graphics::Color boxColor;
    Graphics::Color checkColor;
    std::function<void(bool)> onToggle;
    
public:
    Checkbox(const Math::Vector2D& pos, float size);
    
    void render(Graphics::Renderer* renderer) override;
    void handleInput(const Input::InputManager& input) override;
    
    void setChecked(bool ch);
    bool isChecked() const { return checked; }
    void setOnToggle(const std::function<void(bool)>& callback) { onToggle = callback; }
};

// =============================================================================
// Skip Link - For keyboard navigation
// =============================================================================

class SkipLink : public UIElement {
private:
    std::string targetId;
    std::string text;
    bool visibleOnFocus;
    
public:
    SkipLink(const std::string& target, const std::string& linkText = "Skip to main content");
    
    void render(Graphics::Renderer* renderer) override;
    bool onKeyDown(int keyCode) override;
    void onFocus() override;
    void onBlur() override;
};

// =============================================================================
// Accessible Tooltip
// =============================================================================

class AccessibleTooltip : public UIElement {
private:
    std::string text;
    UIElement* targetElement;
    float showDelay;
    float hideDelay;
    float timer;
    bool showing;
    
public:
    AccessibleTooltip(const std::string& tooltipText);
    
    void update(float deltaTime) override;
    void render(Graphics::Renderer* renderer) override;
    
    void attachTo(UIElement* element);
    void show();
    void hide();
    
    void setShowDelay(float delay) { showDelay = delay; }
    void setHideDelay(float delay) { hideDelay = delay; }
};

} // namespace UI
} // namespace JJM

#endif // UI_ELEMENT_H
