#include "ui/LayoutSystem.h"
#include <algorithm>

namespace JJM {
namespace UI {

// UIElement implementation
UIElement::UIElement() 
    : marginTop(0), marginRight(0), marginBottom(0), marginLeft(0),
      paddingTop(0), paddingRight(0), paddingBottom(0), paddingLeft(0) {
}

UIElement::~UIElement() {
}

void UIElement::setPosition(const Math::Vector2D& pos) {
    position = pos;
}

void UIElement::setSize(const Math::Vector2D& sz) {
    size = sz;
}

Math::Vector2D UIElement::getPosition() const {
    return position;
}

Math::Vector2D UIElement::getSize() const {
    return size;
}

void UIElement::setMargin(float top, float right, float bottom, float left) {
    marginTop = top;
    marginRight = right;
    marginBottom = bottom;
    marginLeft = left;
}

void UIElement::setPadding(float top, float right, float bottom, float left) {
    paddingTop = top;
    paddingRight = right;
    paddingBottom = bottom;
    paddingLeft = left;
}

// LayoutContainer implementation
LayoutContainer::LayoutContainer(LayoutType type)
    : layoutType(type), spacing(0), alignment(Alignment::Start) {
}

LayoutContainer::~LayoutContainer() {
}

void LayoutContainer::addChild(std::shared_ptr<UIElement> child) {
    children.push_back(child);
}

void LayoutContainer::removeChild(std::shared_ptr<UIElement> child) {
    children.erase(std::remove(children.begin(), children.end(), child), children.end());
}

void LayoutContainer::clear() {
    children.clear();
}

void LayoutContainer::setSpacing(float sp) {
    spacing = sp;
}

void LayoutContainer::setAlignment(Alignment align) {
    alignment = align;
}

void LayoutContainer::layout() {
    float currentX = position.x + paddingLeft;
    float currentY = position.y + paddingTop;
    
    if (layoutType == LayoutType::Horizontal) {
        for (auto& child : children) {
            child->setPosition(Math::Vector2D(currentX, currentY));
            child->layout();
            currentX += child->getSize().x + spacing;
        }
    } else if (layoutType == LayoutType::Vertical) {
        for (auto& child : children) {
            child->setPosition(Math::Vector2D(currentX, currentY));
            child->layout();
            currentY += child->getSize().y + spacing;
        }
    }
}

void LayoutContainer::render() {
    for (auto& child : children) {
        child->render();
    }
}

// FlexLayout implementation
FlexLayout::FlexLayout()
    : horizontal(true), wrap(false), justify(Alignment::Start), alignItems(Alignment::Start) {
}

FlexLayout::~FlexLayout() {
}

void FlexLayout::setDirection(bool horiz) {
    horizontal = horiz;
}

void FlexLayout::setWrap(bool w) {
    wrap = w;
}

void FlexLayout::setJustify(Alignment j) {
    justify = j;
}

void FlexLayout::setAlignItems(Alignment align) {
    alignItems = align;
}

void FlexLayout::calculate(std::vector<std::shared_ptr<UIElement>>& elements,
                           const Math::Vector2D& containerSize) {
    float position = 0;
    for (auto& elem : elements) {
        if (horizontal) {
            elem->setPosition(Math::Vector2D(position, 0));
            position += elem->getSize().x;
        } else {
            elem->setPosition(Math::Vector2D(0, position));
            position += elem->getSize().y;
        }
    }
    (void)containerSize;
}

// AnchorLayout implementation
void AnchorLayout::apply(UIElement* element, const Anchors& anchors,
                        const Math::Vector2D& parentSize) {
    float x = anchors.left * parentSize.x;
    float y = anchors.top * parentSize.y;
    float width = (anchors.right - anchors.left) * parentSize.x;
    float height = (anchors.bottom - anchors.top) * parentSize.y;
    
    element->setPosition(Math::Vector2D(x, y));
    element->setSize(Math::Vector2D(width, height));
}

} // namespace UI
} // namespace JJM
