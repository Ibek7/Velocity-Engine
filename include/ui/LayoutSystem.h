#ifndef JJM_UI_LAYOUT_SYSTEM_H
#define JJM_UI_LAYOUT_SYSTEM_H

#include "math/Vector2D.h"
#include <string>
#include <vector>
#include <memory>

namespace JJM {
namespace UI {

enum class LayoutType { Horizontal, Vertical, Grid, Absolute };
enum class Alignment { Start, Center, End, Stretch };
enum class SizeMode { Fixed, Percentage, Auto, MinContent, MaxContent, FitContent };

/**
 * @brief Layout constraints for flexible sizing
 */
struct LayoutConstraints {
    // Size constraints
    SizeMode widthMode = SizeMode::Auto;
    SizeMode heightMode = SizeMode::Auto;
    float minWidth = 0.0f;
    float maxWidth = std::numeric_limits<float>::max();
    float minHeight = 0.0f;
    float maxHeight = std::numeric_limits<float>::max();
    float preferredWidth = 0.0f;
    float preferredHeight = 0.0f;
    
    // Aspect ratio constraint (0 = no constraint)
    float aspectRatio = 0.0f;
    
    // Flex grow/shrink
    float flexGrow = 0.0f;
    float flexShrink = 1.0f;
    float flexBasis = 0.0f;  // Initial size before flex
};

/**
 * @brief Anchor points for relative positioning
 */
struct AnchorPoint {
    float horizontal = 0.5f;  // 0.0 = left, 0.5 = center, 1.0 = right
    float vertical = 0.5f;    // 0.0 = top, 0.5 = center, 1.0 = bottom
};

struct Anchors {
    // Anchor to parent edges (0-1, negative = disabled)
    float left = -1.0f;
    float right = -1.0f;
    float top = -1.0f;
    float bottom = -1.0f;
    
    // Offsets from anchors
    float leftOffset = 0.0f;
    float rightOffset = 0.0f;
    float topOffset = 0.0f;
    float bottomOffset = 0.0f;
    
    bool isAnchored() const {
        return left >= 0 || right >= 0 || top >= 0 || bottom >= 0;
    }
};

class UIElement {
public:
    UIElement();
    virtual ~UIElement();
    
    void setPosition(const Math::Vector2D& pos);
    void setSize(const Math::Vector2D& size);
    Math::Vector2D getPosition() const;
    Math::Vector2D getSize() const;
    
    void setMargin(float top, float right, float bottom, float left);
    void setPadding(float top, float right, float bottom, float left);
    
    // Layout constraints
    void setConstraints(const LayoutConstraints& constraints) { this->constraints = constraints; }
    const LayoutConstraints& getConstraints() const { return constraints; }
    void setMinSize(const Math::Vector2D& minSize);
    void setMaxSize(const Math::Vector2D& maxSize);
    void setAspectRatio(float ratio) { constraints.aspectRatio = ratio; }
    
    // Anchor system
    void setAnchors(const Anchors& anchors) { this->anchors = anchors; }
    const Anchors& getAnchors() const { return anchors; }
    void anchorToParent(float left, float right, float top, float bottom);
    void centerInParent();
    
    // Auto-sizing
    void setSizeMode(SizeMode widthMode, SizeMode heightMode);
    Math::Vector2D calculateAutoSize() const;
    Math::Vector2D calculateMinSize() const;
    Math::Vector2D calculateMaxSize() const;
    
    virtual void layout() = 0;
    virtual void render() = 0;

protected:
    Math::Vector2D position;
    Math::Vector2D size;
    float marginTop, marginRight, marginBottom, marginLeft;
    float paddingTop, paddingRight, paddingBottom, paddingLeft;
    
    LayoutConstraints constraints;
    Anchors anchors;
};

class LayoutContainer : public UIElement {
public:
    LayoutContainer(LayoutType type);
    ~LayoutContainer();
    
    void addChild(std::shared_ptr<UIElement> child);
    void removeChild(std::shared_ptr<UIElement> child);
    void clear();
    
    void setSpacing(float spacing);
    void setAlignment(Alignment align);
    
    void layout() override;
    void render() override;

private:
    LayoutType layoutType;
    std::vector<std::shared_ptr<UIElement>> children;
    float spacing;
    Alignment alignment;
};

class FlexLayout {
public:
    FlexLayout();
    ~FlexLayout();
    
    void setDirection(bool horizontal);
    void setWrap(bool wrap);
    void setJustify(Alignment justify);
    void setAlignItems(Alignment align);
    
    void calculate(std::vector<std::shared_ptr<UIElement>>& elements,
                  const Math::Vector2D& containerSize);

private:
    bool horizontal;
    bool wrap;
    Alignment justify;
    Alignment alignItems;
};

class AnchorLayout {
public:
    struct Anchors {
        float left, right, top, bottom;
    };
    
    static void apply(UIElement* element, const Anchors& anchors,
                     const Math::Vector2D& parentSize);
};

} // namespace UI
} // namespace JJM

#endif
