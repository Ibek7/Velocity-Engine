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
#include <variant>
#include <cmath>
#include <algorithm>

namespace JJM {
namespace UI {

// =============================================================================
// UI Layout System - Flexbox, Grid, and Constraint-based layouts
// =============================================================================

// Forward declarations
class UIElement;
class LayoutContainer;

// Layout measurement units
enum class LayoutUnit {
    Pixels,         // Absolute pixels
    Percent,        // Percentage of parent
    Auto,           // Content-based sizing
    Stretch,        // Fill available space
    AspectRatio     // Maintain aspect ratio
};

// Layout value with unit
struct LayoutValue {
    float value;
    LayoutUnit unit;
    
    LayoutValue() : value(0), unit(LayoutUnit::Auto) {}
    LayoutValue(float v, LayoutUnit u = LayoutUnit::Pixels) : value(v), unit(u) {}
    
    static LayoutValue px(float v) { return LayoutValue(v, LayoutUnit::Pixels); }
    static LayoutValue percent(float v) { return LayoutValue(v, LayoutUnit::Percent); }
    static LayoutValue autoSize() { return LayoutValue(0, LayoutUnit::Auto); }
    static LayoutValue stretch() { return LayoutValue(1, LayoutUnit::Stretch); }
    
    float resolve(float parentSize, float contentSize = 0) const {
        switch (unit) {
            case LayoutUnit::Pixels: return value;
            case LayoutUnit::Percent: return parentSize * value / 100.0f;
            case LayoutUnit::Auto: return contentSize;
            case LayoutUnit::Stretch: return parentSize;
            case LayoutUnit::AspectRatio: return contentSize * value;
            default: return value;
        }
    }
};

// Edge insets (padding, margin)
struct EdgeInsets {
    float top, right, bottom, left;
    
    EdgeInsets() : top(0), right(0), bottom(0), left(0) {}
    EdgeInsets(float all) : top(all), right(all), bottom(all), left(all) {}
    EdgeInsets(float vertical, float horizontal) 
        : top(vertical), right(horizontal), bottom(vertical), left(horizontal) {}
    EdgeInsets(float t, float r, float b, float l) 
        : top(t), right(r), bottom(b), left(l) {}
    
    float horizontal() const { return left + right; }
    float vertical() const { return top + bottom; }
};

// Alignment options
enum class Alignment {
    Start,
    Center,
    End,
    Stretch,
    SpaceBetween,
    SpaceAround,
    SpaceEvenly,
    Baseline
};

// Flex direction
enum class FlexDirection {
    Row,
    RowReverse,
    Column,
    ColumnReverse
};

// Flex wrap
enum class FlexWrap {
    NoWrap,
    Wrap,
    WrapReverse
};

// Layout constraints
struct LayoutConstraints {
    LayoutValue minWidth;
    LayoutValue maxWidth;
    LayoutValue minHeight;
    LayoutValue maxHeight;
    LayoutValue preferredWidth;
    LayoutValue preferredHeight;
    float aspectRatio;          // Width / Height, 0 = none
    
    LayoutConstraints()
        : minWidth(0, LayoutUnit::Pixels)
        , maxWidth(std::numeric_limits<float>::max(), LayoutUnit::Pixels)
        , minHeight(0, LayoutUnit::Pixels)
        , maxHeight(std::numeric_limits<float>::max(), LayoutUnit::Pixels)
        , preferredWidth(0, LayoutUnit::Auto)
        , preferredHeight(0, LayoutUnit::Auto)
        , aspectRatio(0)
    {}
};

// =============================================================================
// Flexbox Layout
// =============================================================================

struct FlexProperties {
    FlexDirection direction;
    FlexWrap wrap;
    Alignment justifyContent;   // Main axis alignment
    Alignment alignItems;       // Cross axis alignment
    Alignment alignContent;     // Multi-line cross axis alignment
    float gap;                  // Gap between items
    float rowGap;               // Gap between rows
    float columnGap;            // Gap between columns
    
    FlexProperties()
        : direction(FlexDirection::Row)
        , wrap(FlexWrap::NoWrap)
        , justifyContent(Alignment::Start)
        , alignItems(Alignment::Stretch)
        , alignContent(Alignment::Start)
        , gap(0)
        , rowGap(0)
        , columnGap(0)
    {}
};

struct FlexItemProperties {
    float flexGrow;             // Growth factor
    float flexShrink;           // Shrink factor
    LayoutValue flexBasis;      // Initial size
    Alignment alignSelf;        // Override alignItems
    int order;                  // Display order
    
    FlexItemProperties()
        : flexGrow(0)
        , flexShrink(1)
        , flexBasis(0, LayoutUnit::Auto)
        , alignSelf(Alignment::Stretch)
        , order(0)
    {}
};

class FlexLayout {
private:
    FlexProperties properties;
    
    struct FlexLine {
        std::vector<size_t> itemIndices;
        float mainAxisSize;
        float crossAxisSize;
    };
    
public:
    FlexLayout() = default;
    
    void setDirection(FlexDirection dir) { properties.direction = dir; }
    void setWrap(FlexWrap w) { properties.wrap = w; }
    void setJustifyContent(Alignment align) { properties.justifyContent = align; }
    void setAlignItems(Alignment align) { properties.alignItems = align; }
    void setGap(float g) { properties.gap = g; properties.rowGap = g; properties.columnGap = g; }
    
    FlexProperties& getProperties() { return properties; }
    const FlexProperties& getProperties() const { return properties; }
    
    void calculateLayout(LayoutContainer* container, float parentWidth, float parentHeight);
    
private:
    bool isMainAxisHorizontal() const {
        return properties.direction == FlexDirection::Row || 
               properties.direction == FlexDirection::RowReverse;
    }
    
    std::vector<FlexLine> collectIntoLines(
        const std::vector<UIElement*>& children,
        const std::vector<FlexItemProperties>& itemProps,
        float containerMainSize);
    
    void resolveFlexibleLengths(
        std::vector<UIElement*>& children,
        const std::vector<FlexItemProperties>& itemProps,
        FlexLine& line,
        float containerMainSize);
    
    void alignMainAxis(
        std::vector<UIElement*>& children,
        FlexLine& line,
        float containerMainSize,
        float startOffset);
    
    void alignCrossAxis(
        std::vector<UIElement*>& children,
        const std::vector<FlexItemProperties>& itemProps,
        FlexLine& line,
        float lineOffset,
        float lineSize);
};

// =============================================================================
// Grid Layout
// =============================================================================

// Grid track sizing
struct GridTrackSize {
    enum class Type { Fixed, Fraction, MinContent, MaxContent, Auto, MinMax };
    Type type;
    float value;
    float minValue;
    float maxValue;
    
    GridTrackSize() : type(Type::Auto), value(0), minValue(0), maxValue(std::numeric_limits<float>::max()) {}
    
    static GridTrackSize fixed(float px) { 
        GridTrackSize t; t.type = Type::Fixed; t.value = px; return t; 
    }
    static GridTrackSize fr(float fraction) { 
        GridTrackSize t; t.type = Type::Fraction; t.value = fraction; return t; 
    }
    static GridTrackSize minContent() { 
        GridTrackSize t; t.type = Type::MinContent; return t; 
    }
    static GridTrackSize maxContent() { 
        GridTrackSize t; t.type = Type::MaxContent; return t; 
    }
    static GridTrackSize autoSize() { 
        GridTrackSize t; t.type = Type::Auto; return t; 
    }
    static GridTrackSize minMax(float min, float max) {
        GridTrackSize t; t.type = Type::MinMax; t.minValue = min; t.maxValue = max; return t;
    }
};

struct GridProperties {
    std::vector<GridTrackSize> columns;
    std::vector<GridTrackSize> rows;
    float columnGap;
    float rowGap;
    Alignment justifyItems;     // Horizontal alignment in cells
    Alignment alignItems;       // Vertical alignment in cells
    Alignment justifyContent;   // Horizontal alignment of grid
    Alignment alignContent;     // Vertical alignment of grid
    bool autoFlow;              // Auto-placement
    bool autoFlowDense;         // Dense packing
    
    GridProperties()
        : columnGap(0)
        , rowGap(0)
        , justifyItems(Alignment::Stretch)
        , alignItems(Alignment::Stretch)
        , justifyContent(Alignment::Start)
        , alignContent(Alignment::Start)
        , autoFlow(true)
        , autoFlowDense(false)
    {}
};

struct GridItemProperties {
    int columnStart, columnEnd;
    int rowStart, rowEnd;
    Alignment justifySelf;      // Override justifyItems
    Alignment alignSelf;        // Override alignItems
    
    GridItemProperties()
        : columnStart(-1), columnEnd(-1)
        , rowStart(-1), rowEnd(-1)
        , justifySelf(Alignment::Stretch)
        , alignSelf(Alignment::Stretch)
    {}
    
    void setColumn(int start, int span = 1) { columnStart = start; columnEnd = start + span; }
    void setRow(int start, int span = 1) { rowStart = start; rowEnd = start + span; }
    void setArea(int colStart, int rowStart, int colSpan = 1, int rowSpan = 1) {
        this->columnStart = colStart; this->columnEnd = colStart + colSpan;
        this->rowStart = rowStart; this->rowEnd = rowStart + rowSpan;
    }
};

class GridLayout {
private:
    GridProperties properties;
    std::vector<float> columnSizes;
    std::vector<float> rowSizes;
    std::vector<float> columnOffsets;
    std::vector<float> rowOffsets;
    
    // Grid cell occupation
    std::vector<std::vector<int>> cellOccupation; // -1 = empty, index = item index
    
public:
    GridLayout() = default;
    
    void setColumns(const std::vector<GridTrackSize>& cols) { properties.columns = cols; }
    void setRows(const std::vector<GridTrackSize>& rws) { properties.rows = rws; }
    void setGap(float gap) { properties.columnGap = gap; properties.rowGap = gap; }
    void setColumnGap(float gap) { properties.columnGap = gap; }
    void setRowGap(float gap) { properties.rowGap = gap; }
    
    // Shorthand for common patterns
    void setColumnsRepeat(int count, const GridTrackSize& size) {
        properties.columns.clear();
        for (int i = 0; i < count; ++i) properties.columns.push_back(size);
    }
    
    GridProperties& getProperties() { return properties; }
    const GridProperties& getProperties() const { return properties; }
    
    void calculateLayout(LayoutContainer* container, float parentWidth, float parentHeight);
    
private:
    void resolveTrackSizes(std::vector<float>& sizes, 
                          const std::vector<GridTrackSize>& tracks,
                          float availableSpace, float gap);
    
    void calculateOffsets(std::vector<float>& offsets,
                         const std::vector<float>& sizes, float gap);
    
    void autoPlaceItems(std::vector<UIElement*>& children,
                       std::vector<GridItemProperties>& itemProps);
    
    bool findEmptyCell(int& outCol, int& outRow, int colSpan, int rowSpan);
    void occupyCells(int col, int row, int colSpan, int rowSpan, int itemIndex);
};

// =============================================================================
// Constraint-based Layout (Similar to iOS Auto Layout)
// =============================================================================

enum class ConstraintAttribute {
    Left,
    Right,
    Top,
    Bottom,
    Leading,
    Trailing,
    Width,
    Height,
    CenterX,
    CenterY,
    Baseline
};

enum class ConstraintRelation {
    Equal,
    LessThanOrEqual,
    GreaterThanOrEqual
};

struct LayoutConstraint {
    UIElement* firstItem;
    ConstraintAttribute firstAttribute;
    ConstraintRelation relation;
    UIElement* secondItem;          // nullptr for constant
    ConstraintAttribute secondAttribute;
    float multiplier;
    float constant;
    int priority;                   // 1-1000, 1000 = required
    bool isActive;
    std::string identifier;
    
    LayoutConstraint()
        : firstItem(nullptr)
        , firstAttribute(ConstraintAttribute::Left)
        , relation(ConstraintRelation::Equal)
        , secondItem(nullptr)
        , secondAttribute(ConstraintAttribute::Left)
        , multiplier(1.0f)
        , constant(0)
        , priority(1000)
        , isActive(true)
    {}
    
    static LayoutConstraint create(UIElement* item1, ConstraintAttribute attr1,
                                   ConstraintRelation rel,
                                   UIElement* item2, ConstraintAttribute attr2,
                                   float mult = 1.0f, float constant = 0) {
        LayoutConstraint c;
        c.firstItem = item1;
        c.firstAttribute = attr1;
        c.relation = rel;
        c.secondItem = item2;
        c.secondAttribute = attr2;
        c.multiplier = mult;
        c.constant = constant;
        return c;
    }
};

class ConstraintLayout {
private:
    std::vector<LayoutConstraint> constraints;
    int maxIterations;
    float tolerance;
    
public:
    ConstraintLayout() : maxIterations(100), tolerance(0.1f) {}
    
    void addConstraint(const LayoutConstraint& constraint) {
        constraints.push_back(constraint);
    }
    
    void removeConstraint(const std::string& identifier) {
        constraints.erase(
            std::remove_if(constraints.begin(), constraints.end(),
                [&identifier](const LayoutConstraint& c) { 
                    return c.identifier == identifier; 
                }),
            constraints.end()
        );
    }
    
    void clearConstraints() { constraints.clear(); }
    
    // Constraint builder helpers
    static LayoutConstraint pin(UIElement* item, ConstraintAttribute attr, float constant) {
        return LayoutConstraint::create(item, attr, ConstraintRelation::Equal, 
                                        nullptr, attr, 1.0f, constant);
    }
    
    static LayoutConstraint align(UIElement* item1, UIElement* item2, 
                                  ConstraintAttribute attr, float offset = 0) {
        return LayoutConstraint::create(item1, attr, ConstraintRelation::Equal,
                                        item2, attr, 1.0f, offset);
    }
    
    static LayoutConstraint matchSize(UIElement* item1, UIElement* item2,
                                      ConstraintAttribute sizeAttr, float multiplier = 1.0f) {
        return LayoutConstraint::create(item1, sizeAttr, ConstraintRelation::Equal,
                                        item2, sizeAttr, multiplier, 0);
    }
    
    void calculateLayout(LayoutContainer* container, float parentWidth, float parentHeight);
    
private:
    float getAttributeValue(UIElement* item, ConstraintAttribute attr, float parentWidth, float parentHeight);
    void setAttributeValue(UIElement* item, ConstraintAttribute attr, float value, float parentWidth, float parentHeight);
};

// =============================================================================
// Stack Layout (Simple vertical/horizontal stacking)
// =============================================================================

enum class StackDirection {
    Horizontal,
    Vertical,
    ZStack          // Overlay children
};

struct StackProperties {
    StackDirection direction;
    Alignment alignment;
    float spacing;
    bool distributeEvenly;
    
    StackProperties()
        : direction(StackDirection::Vertical)
        , alignment(Alignment::Start)
        , spacing(0)
        , distributeEvenly(false)
    {}
};

class StackLayout {
private:
    StackProperties properties;
    
public:
    StackLayout() = default;
    
    void setDirection(StackDirection dir) { properties.direction = dir; }
    void setAlignment(Alignment align) { properties.alignment = align; }
    void setSpacing(float space) { properties.spacing = space; }
    void setDistributeEvenly(bool distribute) { properties.distributeEvenly = distribute; }
    
    StackProperties& getProperties() { return properties; }
    const StackProperties& getProperties() const { return properties; }
    
    void calculateLayout(LayoutContainer* container, float parentWidth, float parentHeight);
};

// =============================================================================
// Layout Container - Base class for layouting elements
// =============================================================================

enum class LayoutType {
    None,
    Flex,
    Grid,
    Constraint,
    Stack
};

class LayoutContainer : public UIElement {
protected:
    LayoutType layoutType;
    std::unique_ptr<FlexLayout> flexLayout;
    std::unique_ptr<GridLayout> gridLayout;
    std::unique_ptr<ConstraintLayout> constraintLayout;
    std::unique_ptr<StackLayout> stackLayout;
    
    // Item properties for flex/grid
    std::vector<FlexItemProperties> flexItemProps;
    std::vector<GridItemProperties> gridItemProps;
    
    EdgeInsets padding;
    EdgeInsets margin;
    LayoutConstraints constraints;
    
    bool needsLayout;
    bool clipChildren;
    
public:
    LayoutContainer(const Math::Vector2D& pos, const Math::Vector2D& size);
    virtual ~LayoutContainer();
    
    // Layout type selection
    void setLayoutType(LayoutType type);
    LayoutType getLayoutType() const { return layoutType; }
    
    // Access layout systems
    FlexLayout* getFlexLayout() { return flexLayout.get(); }
    GridLayout* getGridLayout() { return gridLayout.get(); }
    ConstraintLayout* getConstraintLayout() { return constraintLayout.get(); }
    StackLayout* getStackLayout() { return stackLayout.get(); }
    
    // Flex item properties
    void setFlexItemProperties(size_t childIndex, const FlexItemProperties& props);
    FlexItemProperties& getFlexItemProperties(size_t childIndex);
    
    // Grid item properties
    void setGridItemProperties(size_t childIndex, const GridItemProperties& props);
    GridItemProperties& getGridItemProperties(size_t childIndex);
    
    // Spacing
    void setPadding(const EdgeInsets& p) { padding = p; setNeedsLayout(); }
    void setMargin(const EdgeInsets& m) { margin = m; setNeedsLayout(); }
    const EdgeInsets& getPadding() const { return padding; }
    const EdgeInsets& getMargin() const { return margin; }
    
    // Constraints
    void setConstraints(const LayoutConstraints& c) { constraints = c; setNeedsLayout(); }
    LayoutConstraints& getConstraints() { return constraints; }
    
    // Layout control
    void setNeedsLayout() { needsLayout = true; }
    void layoutIfNeeded();
    virtual void performLayout();
    
    // Clipping
    void setClipChildren(bool clip) { clipChildren = clip; }
    bool getClipChildren() const { return clipChildren; }
    
    // Content size
    virtual Math::Vector2D getContentSize() const;
    Math::Vector2D getInnerSize() const;
    
    // Override update to handle layout
    void update(float deltaTime) override;
    void render(Graphics::Renderer* renderer) override;
    
    // Child access for layouts
    std::vector<UIElement*> getChildPointers();
};

// =============================================================================
// Scroll View - Scrollable container
// =============================================================================

enum class ScrollDirection {
    Vertical,
    Horizontal,
    Both
};

class ScrollView : public LayoutContainer {
private:
    Math::Vector2D scrollOffset;
    Math::Vector2D scrollVelocity;
    Math::Vector2D contentSize;
    
    ScrollDirection scrollDirection;
    bool showScrollBars;
    bool bounceEnabled;
    bool pagingEnabled;
    
    float scrollBarWidth;
    float scrollDeceleration;
    bool isDragging;
    Math::Vector2D dragStartPos;
    Math::Vector2D scrollStartOffset;
    
    // Scroll bar visibility
    float verticalScrollBarAlpha;
    float horizontalScrollBarAlpha;
    float scrollBarFadeDelay;
    float scrollBarFadeTimer;
    
public:
    ScrollView(const Math::Vector2D& pos, const Math::Vector2D& size);
    
    void update(float deltaTime) override;
    void render(Graphics::Renderer* renderer) override;
    void handleInput(const Input::InputManager& input) override;
    
    // Scroll control
    void scrollTo(const Math::Vector2D& offset, bool animated = true);
    void scrollToTop(bool animated = true);
    void scrollToBottom(bool animated = true);
    void scrollToChild(UIElement* child, bool animated = true);
    
    // Properties
    void setScrollDirection(ScrollDirection dir) { scrollDirection = dir; }
    void setShowScrollBars(bool show) { showScrollBars = show; }
    void setBounceEnabled(bool bounce) { bounceEnabled = bounce; }
    void setPagingEnabled(bool paging) { pagingEnabled = paging; }
    void setContentSize(const Math::Vector2D& size) { contentSize = size; }
    
    const Math::Vector2D& getScrollOffset() const { return scrollOffset; }
    const Math::Vector2D& getContentSize() const { return contentSize; }
    
    Math::Vector2D getMaxScrollOffset() const;
    bool canScrollVertically() const;
    bool canScrollHorizontally() const;
    
private:
    void clampScrollOffset();
    void renderScrollBars(Graphics::Renderer* renderer);
    void updateScrollBarVisibility(float deltaTime);
};

// =============================================================================
// Responsive Layout - Breakpoint-based layouts
// =============================================================================

struct Breakpoint {
    std::string name;
    float minWidth;
    float maxWidth;
    
    Breakpoint() : minWidth(0), maxWidth(std::numeric_limits<float>::max()) {}
    Breakpoint(const std::string& n, float min, float max = std::numeric_limits<float>::max())
        : name(n), minWidth(min), maxWidth(max) {}
};

class ResponsiveLayout {
private:
    std::vector<Breakpoint> breakpoints;
    std::string currentBreakpoint;
    
    // Layout configurations per breakpoint
    std::unordered_map<std::string, FlexProperties> flexConfigs;
    std::unordered_map<std::string, GridProperties> gridConfigs;
    std::unordered_map<std::string, StackProperties> stackConfigs;
    
public:
    ResponsiveLayout() {
        // Default breakpoints (similar to Bootstrap)
        breakpoints.push_back(Breakpoint("xs", 0, 576));
        breakpoints.push_back(Breakpoint("sm", 576, 768));
        breakpoints.push_back(Breakpoint("md", 768, 992));
        breakpoints.push_back(Breakpoint("lg", 992, 1200));
        breakpoints.push_back(Breakpoint("xl", 1200, 1400));
        breakpoints.push_back(Breakpoint("xxl", 1400));
    }
    
    void addBreakpoint(const Breakpoint& bp) { breakpoints.push_back(bp); }
    void clearBreakpoints() { breakpoints.clear(); }
    
    std::string getBreakpointForWidth(float width) const {
        for (const auto& bp : breakpoints) {
            if (width >= bp.minWidth && width < bp.maxWidth) {
                return bp.name;
            }
        }
        return breakpoints.empty() ? "" : breakpoints.back().name;
    }
    
    void setFlexConfig(const std::string& breakpoint, const FlexProperties& props) {
        flexConfigs[breakpoint] = props;
    }
    
    void setGridConfig(const std::string& breakpoint, const GridProperties& props) {
        gridConfigs[breakpoint] = props;
    }
    
    bool hasFlexConfig(const std::string& breakpoint) const {
        return flexConfigs.find(breakpoint) != flexConfigs.end();
    }
    
    const FlexProperties& getFlexConfig(const std::string& breakpoint) const {
        return flexConfigs.at(breakpoint);
    }
    
    const GridProperties& getGridConfig(const std::string& breakpoint) const {
        return gridConfigs.at(breakpoint);
    }
    
    void applyToContainer(LayoutContainer* container, float containerWidth);
};

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
