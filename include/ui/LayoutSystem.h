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
    
    virtual void layout() = 0;
    virtual void render() = 0;

protected:
    Math::Vector2D position;
    Math::Vector2D size;
    float marginTop, marginRight, marginBottom, marginLeft;
    float paddingTop, paddingRight, paddingBottom, paddingLeft;
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
