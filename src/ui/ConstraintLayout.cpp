#include "../../include/ui/ConstraintLayout.h"
#include <algorithm>

namespace JJM {
namespace UI {

UIElement::UIElement(const std::string& id)
    : m_id(id), m_x(0), m_y(0), m_width(0), m_height(0), m_parent(nullptr), m_visible(true) {
}

void UIElement::addChild(std::shared_ptr<UIElement> child) {
    child->setParent(this);
    m_children.push_back(child);
}

void UIElement::removeChild(const std::string& childId) {
    m_children.erase(
        std::remove_if(m_children.begin(), m_children.end(),
            [&childId](const std::shared_ptr<UIElement>& child) {
                return child->getId() == childId;
            }),
        m_children.end()
    );
}

void UIElement::layout() {
    for (auto& child : m_children) {
        child->layout();
    }
}

void ConstraintSolver::solve(UIElement* root) {
    if (!root) return;
    
    std::unordered_map<std::string, UIElement*> elements;
    std::function<void(UIElement*)> collectElements = [&](UIElement* elem) {
        elements[elem->getId()] = elem;
        for (auto& child : elem->getChildren()) {
            collectElements(child.get());
        }
    };
    collectElements(root);
    
    // Resolve constraints
    for (auto& pair : elements) {
        resolveConstraints(pair.second, elements);
    }
}

void ConstraintSolver::resolveConstraints(UIElement* element,
                                         std::unordered_map<std::string, UIElement*>& elements) {
    auto& params = element->getLayoutParams();
    
    // Calculate size
    float width = params.width;
    float height = params.height;
    
    if (params.widthMode == SizeMode::MATCH_PARENT && element->getParent()) {
        width = element->getParent()->getWidth();
    } else if (params.widthMode == SizeMode::PERCENT && element->getParent()) {
        width = element->getParent()->getWidth() * (params.width / 100.0f);
    }
    
    if (params.heightMode == SizeMode::MATCH_PARENT && element->getParent()) {
        height = element->getParent()->getHeight();
    } else if (params.heightMode == SizeMode::PERCENT && element->getParent()) {
        height = element->getParent()->getHeight() * (params.height / 100.0f);
    }
    
    // Apply constraints
    width = std::clamp(width, params.minWidth, params.maxWidth);
    height = std::clamp(height, params.minHeight, params.maxHeight);
    
    element->setSize(width, height);
    
    // Position based on constraints
    float x = element->getX();
    float y = element->getY();
    
    for (const auto& constraint : params.constraints) {
        auto targetIt = elements.find(constraint.targetId);
        if (targetIt != elements.end()) {
            UIElement* target = targetIt->second;
            
            float targetPos = getAnchorPosition(target, constraint.targetAnchor,
                constraint.sourceAnchor == Anchor::LEFT || constraint.sourceAnchor == Anchor::RIGHT);
            float sourceOffset = getAnchorPosition(element, constraint.sourceAnchor,
                constraint.sourceAnchor == Anchor::LEFT || constraint.sourceAnchor == Anchor::RIGHT);
            
            if (constraint.sourceAnchor == Anchor::LEFT || constraint.sourceAnchor == Anchor::RIGHT) {
                x = targetPos - sourceOffset + constraint.offset;
            } else {
                y = targetPos - sourceOffset + constraint.offset;
            }
        }
    }
    
    element->setPosition(x + params.marginLeft, y + params.marginTop);
}

float ConstraintSolver::getAnchorPosition(UIElement* element, Anchor anchor, bool isHorizontal) {
    if (isHorizontal) {
        switch (anchor) {
            case Anchor::LEFT: return element->getX();
            case Anchor::RIGHT: return element->getX() + element->getWidth();
            case Anchor::CENTER_X: return element->getX() + element->getWidth() * 0.5f;
            default: return element->getX();
        }
    } else {
        switch (anchor) {
            case Anchor::TOP: return element->getY();
            case Anchor::BOTTOM: return element->getY() + element->getHeight();
            case Anchor::CENTER_Y: return element->getY() + element->getHeight() * 0.5f;
            default: return element->getY();
        }
    }
}

} // namespace UI
} // namespace JJM
