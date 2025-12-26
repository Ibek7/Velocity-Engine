#include "ui/UILayout.h"
#include <algorithm>
#include <cmath>

namespace Engine {

// LayoutContainer implementation
LayoutContainer::LayoutContainer()
    : m_direction(LayoutDirection::Horizontal)
    , m_alignment(LayoutAlignment::Start)
    , m_justify(LayoutJustify::Start)
    , m_paddingTop(0.0f)
    , m_paddingRight(0.0f)
    , m_paddingBottom(0.0f)
    , m_paddingLeft(0.0f)
    , m_gap(0.0f)
    , m_wrap(false)
    , m_computedWidth(0.0f)
    , m_computedHeight(0.0f)
{
}

LayoutContainer::~LayoutContainer() {
}

void LayoutContainer::setPadding(float top, float right, float bottom, float left) {
    m_paddingTop = top;
    m_paddingRight = right;
    m_paddingBottom = bottom;
    m_paddingLeft = left;
}

void LayoutContainer::addChild(UIElement* element, float flexGrow, float flexShrink) {
    if (!element) return;
    
    ChildInfo info;
    info.element = element;
    info.flexGrow = flexGrow;
    info.flexShrink = flexShrink;
    info.baseSize = 0.0f;
    info.computedSize = 0.0f;
    
    m_children.push_back(info);
}

void LayoutContainer::removeChild(UIElement* element) {
    m_children.erase(
        std::remove_if(m_children.begin(), m_children.end(),
            [element](const ChildInfo& info) { return info.element == element; }),
        m_children.end()
    );
}

void LayoutContainer::clearChildren() {
    m_children.clear();
}

void LayoutContainer::layout(float availableWidth, float availableHeight) {
    m_computedWidth = availableWidth;
    m_computedHeight = availableHeight;
    
    computeLayout();
    positionChildren();
}

void LayoutContainer::computeLayout() {
    if (m_children.empty()) return;
    
    // Calculate available space (minus padding)
    float availableSpace;
    if (m_direction == LayoutDirection::Horizontal) {
        availableSpace = m_computedWidth - m_paddingLeft - m_paddingRight;
        availableSpace -= m_gap * (m_children.size() - 1);
    } else {
        availableSpace = m_computedHeight - m_paddingTop - m_paddingBottom;
        availableSpace -= m_gap * (m_children.size() - 1);
    }
    
    // Calculate base sizes
    for (auto& child : m_children) {
        if (!child.element->isVisible()) continue;
        
        float w, h;
        child.element->getSize(w, h);
        
        if (m_direction == LayoutDirection::Horizontal) {
            child.baseSize = w;
        } else {
            child.baseSize = h;
        }
    }
    
    // Distribute space
    distributeSpace(availableSpace);
}

float LayoutContainer::calculateTotalBaseSize() const {
    float total = 0.0f;
    
    for (const auto& child : m_children) {
        if (child.element->isVisible()) {
            total += child.baseSize;
        }
    }
    
    return total;
}

void LayoutContainer::distributeSpace(float availableSpace) {
    float totalBaseSize = calculateTotalBaseSize();
    float remainingSpace = availableSpace - totalBaseSize;
    
    if (remainingSpace > 0.0f) {
        // Grow: distribute to elements with flexGrow
        float totalGrow = 0.0f;
        for (const auto& child : m_children) {
            if (child.element->isVisible()) {
                totalGrow += child.flexGrow;
            }
        }
        
        if (totalGrow > 0.0f) {
            for (auto& child : m_children) {
                if (child.element->isVisible() && child.flexGrow > 0.0f) {
                    float extraSpace = (child.flexGrow / totalGrow) * remainingSpace;
                    child.computedSize = child.baseSize + extraSpace;
                } else {
                    child.computedSize = child.baseSize;
                }
            }
        } else {
            // No flex grow, use base sizes
            for (auto& child : m_children) {
                child.computedSize = child.baseSize;
            }
        }
    } else if (remainingSpace < 0.0f) {
        // Shrink: reduce elements with flexShrink
        float totalShrink = 0.0f;
        for (const auto& child : m_children) {
            if (child.element->isVisible()) {
                totalShrink += child.flexShrink * child.baseSize;
            }
        }
        
        if (totalShrink > 0.0f) {
            for (auto& child : m_children) {
                if (child.element->isVisible() && child.flexShrink > 0.0f) {
                    float shrinkAmount = (child.flexShrink * child.baseSize / totalShrink) * std::abs(remainingSpace);
                    child.computedSize = std::max(0.0f, child.baseSize - shrinkAmount);
                } else {
                    child.computedSize = child.baseSize;
                }
            }
        } else {
            for (auto& child : m_children) {
                child.computedSize = child.baseSize;
            }
        }
    } else {
        // Perfect fit
        for (auto& child : m_children) {
            child.computedSize = child.baseSize;
        }
    }
}

void LayoutContainer::positionChildren() {
    if (m_children.empty()) return;
    
    float currentPos = 0.0f;
    
    // Calculate starting position based on justify
    float totalSize = 0.0f;
    for (const auto& child : m_children) {
        if (child.element->isVisible()) {
            totalSize += child.computedSize;
        }
    }
    totalSize += m_gap * (m_children.size() - 1);
    
    float availableSpace = (m_direction == LayoutDirection::Horizontal) ?
        (m_computedWidth - m_paddingLeft - m_paddingRight) :
        (m_computedHeight - m_paddingTop - m_paddingBottom);
    
    switch (m_justify) {
        case LayoutJustify::Center:
            currentPos = (availableSpace - totalSize) * 0.5f;
            break;
        case LayoutJustify::End:
            currentPos = availableSpace - totalSize;
            break;
        case LayoutJustify::SpaceBetween:
            if (m_children.size() > 1) {
                m_gap = (availableSpace - totalSize + m_gap * (m_children.size() - 1)) / (m_children.size() - 1);
            }
            break;
        default:
            break;
    }
    
    // Position each child
    for (auto& child : m_children) {
        if (!child.element->isVisible()) continue;
        
        float x, y, w, h;
        child.element->getSize(w, h);
        
        if (m_direction == LayoutDirection::Horizontal) {
            x = m_paddingLeft + currentPos;
            
            // Align vertically
            switch (m_alignment) {
                case LayoutAlignment::Center:
                    y = m_paddingTop + (m_computedHeight - m_paddingTop - m_paddingBottom - h) * 0.5f;
                    break;
                case LayoutAlignment::End:
                    y = m_computedHeight - m_paddingBottom - h;
                    break;
                case LayoutAlignment::Stretch:
                    y = m_paddingTop;
                    h = m_computedHeight - m_paddingTop - m_paddingBottom;
                    break;
                default:
                    y = m_paddingTop;
                    break;
            }
            
            w = child.computedSize;
            currentPos += child.computedSize + m_gap;
        } else {
            y = m_paddingTop + currentPos;
            
            // Align horizontally
            switch (m_alignment) {
                case LayoutAlignment::Center:
                    x = m_paddingLeft + (m_computedWidth - m_paddingLeft - m_paddingRight - w) * 0.5f;
                    break;
                case LayoutAlignment::End:
                    x = m_computedWidth - m_paddingRight - w;
                    break;
                case LayoutAlignment::Stretch:
                    x = m_paddingLeft;
                    w = m_computedWidth - m_paddingLeft - m_paddingRight;
                    break;
                default:
                    x = m_paddingLeft;
                    break;
            }
            
            h = child.computedSize;
            currentPos += child.computedSize + m_gap;
        }
        
        child.element->onLayout(x, y, w, h);
    }
}

void LayoutContainer::getSize(float& width, float& height) const {
    width = m_computedWidth;
    height = m_computedHeight;
}

UIElement* LayoutContainer::getChild(int index) const {
    if (index >= 0 && index < static_cast<int>(m_children.size())) {
        return m_children[index].element;
    }
    return nullptr;
}

// UIElement implementation
UIElement::UIElement()
    : m_x(0.0f)
    , m_y(0.0f)
    , m_width(100.0f)
    , m_height(100.0f)
    , m_visible(true)
{
    m_constraints.minWidth = 0.0f;
    m_constraints.minHeight = 0.0f;
    m_constraints.maxWidth = 10000.0f;
    m_constraints.maxHeight = 10000.0f;
    m_constraints.preferredWidth = 100.0f;
    m_constraints.preferredHeight = 100.0f;
}

UIElement::~UIElement() {
}

void UIElement::setPosition(float x, float y) {
    m_x = x;
    m_y = y;
}

void UIElement::setSize(float width, float height) {
    m_width = std::max(m_constraints.minWidth, std::min(m_constraints.maxWidth, width));
    m_height = std::max(m_constraints.minHeight, std::min(m_constraints.maxHeight, height));
}

void UIElement::getPosition(float& x, float& y) const {
    x = m_x;
    y = m_y;
}

void UIElement::getSize(float& width, float& height) const {
    width = m_width;
    height = m_height;
}

void UIElement::setConstraints(const LayoutConstraints& constraints) {
    m_constraints = constraints;
}

void UIElement::enableLayout() {
    if (!m_layoutContainer) {
        m_layoutContainer = std::make_unique<LayoutContainer>();
    }
}

void UIElement::onLayout(float x, float y, float width, float height) {
    setPosition(x, y);
    setSize(width, height);
    
    if (m_layoutContainer) {
        m_layoutContainer->layout(width, height);
    }
}

void UIElement::render() {
    // TODO: Render element
}

// UILayoutSystem implementation
UILayoutSystem::UILayoutSystem()
    : m_root(nullptr)
{
}

UILayoutSystem& UILayoutSystem::getInstance() {
    static UILayoutSystem instance;
    return instance;
}

void UILayoutSystem::setRoot(UIElement* root) {
    m_root = root;
}

void UILayoutSystem::update(float viewportWidth, float viewportHeight) {
    if (m_root) {
        m_root->onLayout(0.0f, 0.0f, viewportWidth, viewportHeight);
    }
}

void UILayoutSystem::render() {
    if (m_root) {
        m_root->render();
    }
}

void UILayoutSystem::registerElement(const std::string& id, UIElement* element) {
    m_elements[id] = element;
}

void UILayoutSystem::unregisterElement(const std::string& id) {
    m_elements.erase(id);
}

UIElement* UILayoutSystem::getElementById(const std::string& id) {
    auto it = m_elements.find(id);
    if (it != m_elements.end()) {
        return it->second;
    }
    return nullptr;
}

void UILayoutSystem::queryElementsInRect(float x, float y, float width, float height,
                                        std::vector<UIElement*>& results) {
    // TODO: Spatial query
    (void)x; (void)y; (void)width; (void)height; (void)results;
}

} // namespace Engine
