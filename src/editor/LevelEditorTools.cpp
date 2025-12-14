#include "editor/LevelEditorTools.h"
#include <cmath>
#include <algorithm>

namespace JJM {
namespace Editor {

// EditorGizmo implementation
EditorGizmo::EditorGizmo()
    : type(GizmoType::None), size(50.0f), hovered(false), 
      dragging(false), activeAxis(GizmoAxis::None) {}

EditorGizmo::~EditorGizmo() {}

void EditorGizmo::update(Input::InputManager* input, const JJM::Graphics::Camera* camera) {
    if (!input || !camera) return;
    
    // Get mouse position in world space
    Math::Vector2D mousePos;
    
    // Check if mouse is hovering over gizmo
    activeAxis = getAxisAtPosition(mousePos, camera);
    hovered = (activeAxis != GizmoAxis::None);
    
    // Handle dragging
    bool mouseDown = false; // input->isMouseButtonDown(MouseButton::Left)
    if (mouseDown && hovered && !dragging) {
        dragging = true;
        dragStartPos = mousePos;
    } else if (!mouseDown && dragging) {
        dragging = false;
    }
}

void EditorGizmo::render() {
    // Render gizmo based on type
    switch (type) {
        case GizmoType::Translate:
            // Draw translation arrows
            break;
        case GizmoType::Rotate:
            // Draw rotation circles
            break;
        case GizmoType::Scale:
            // Draw scale handles
            break;
        default:
            break;
    }
}

GizmoAxis EditorGizmo::getAxisAtPosition(const Math::Vector2D& pos, const JJM::Graphics::Camera* camera) {
    if (!camera) return GizmoAxis::None;
    
    float distance = std::sqrt((pos.x - position.x) * (pos.x - position.x) +
                              (pos.y - position.y) * (pos.y - position.y));
    
    if (distance > size) return GizmoAxis::None;
    
    // Determine which axis is closest
    return GizmoAxis::X;
}

// SelectionTool implementation
SelectionTool::SelectionTool()
    : multiSelectEnabled(false), isDragging(false) {}

SelectionTool::~SelectionTool() {}

void SelectionTool::update(Input::InputManager* input, const JJM::Graphics::Camera* camera) {
    if (!input || !camera) return;
    
    // Handle selection input
    bool mouseClicked = false; // input->isMouseButtonPressed(MouseButton::Left)
    
    if (mouseClicked) {
        Math::Vector2D mousePos;
        performSelection(mousePos);
    }
    
    // Handle box selection
    bool mouseDown = false; // input->isMouseButtonDown(MouseButton::Left)
    if (mouseDown && !isDragging) {
        isDragging = true;
    } else if (!mouseDown && isDragging) {
        performBoxSelection(dragStartPos, dragEndPos);
        isDragging = false;
    }
}

void SelectionTool::render() {
    if (isDragging) {
        // Draw selection box
    }
}

void SelectionTool::addToSelection(ECS::Entity* entity) {
    if (!entity) return;
    
    if (!isSelected(entity)) {
        selectedEntities.push_back(entity);
        if (onSelectionChanged) {
            onSelectionChanged();
        }
    }
}

void SelectionTool::removeFromSelection(ECS::Entity* entity) {
    auto it = std::find(selectedEntities.begin(), selectedEntities.end(), entity);
    if (it != selectedEntities.end()) {
        selectedEntities.erase(it);
        if (onSelectionChanged) {
            onSelectionChanged();
        }
    }
}

void SelectionTool::clearSelection() {
    if (!selectedEntities.empty()) {
        selectedEntities.clear();
        if (onSelectionChanged) {
            onSelectionChanged();
        }
    }
}

bool SelectionTool::isSelected(ECS::Entity* entity) const {
    return std::find(selectedEntities.begin(), selectedEntities.end(), entity) 
           != selectedEntities.end();
}

void SelectionTool::performSelection(const Math::Vector2D& worldPos) {
    // Ray cast to find entity at position
    // If found, add to selection (or replace if not multi-select)
}

void SelectionTool::performBoxSelection(const Math::Vector2D& start, const Math::Vector2D& end) {
    // Find all entities within box bounds
    // Add to selection
}

// EntityManipulator implementation
EntityManipulator::EntityManipulator()
    : snapEnabled(false), snapValue(1.0f) {}

EntityManipulator::~EntityManipulator() {}

void EntityManipulator::update(Input::InputManager* input, const JJM::Graphics::Camera* camera,
                               const std::vector<ECS::Entity*>& selectedEntities) {
    if (!input || !camera || selectedEntities.empty()) return;
    
    // Update gizmo position to selected entity
    gizmo.update(input, camera);
    
    // Apply transformation if gizmo is dragging
    if (gizmo.isDragging()) {
        Math::Vector2D delta;
        
        if (snapEnabled) {
            delta = applySnapping(delta);
        }
        
        for (auto* entity : selectedEntities) {
            if (!entity) continue;
            
            switch (gizmo.getType()) {
                case GizmoType::Translate:
                    applyTranslation(entity, delta);
                    break;
                case GizmoType::Rotate:
                    applyRotation(entity, delta.x);
                    break;
                case GizmoType::Scale:
                    applyScale(entity, delta);
                    break;
                default:
                    break;
            }
        }
    }
}

void EntityManipulator::render() {
    gizmo.render();
}

Math::Vector2D EntityManipulator::applySnapping(const Math::Vector2D& value) {
    return Math::Vector2D(
        std::round(value.x / snapValue) * snapValue,
        std::round(value.y / snapValue) * snapValue
    );
}

void EntityManipulator::applyTranslation(ECS::Entity* entity, const Math::Vector2D& delta) {
    if (!entity) return;
    // Apply translation to entity's transform component
}

void EntityManipulator::applyRotation(ECS::Entity* entity, float angle) {
    if (!entity) return;
    // Apply rotation to entity's transform component
}

void EntityManipulator::applyScale(ECS::Entity* entity, const Math::Vector2D& scale) {
    if (!entity) return;
    // Apply scale to entity's transform component
}

// GridRenderer implementation
GridRenderer::GridRenderer()
    : gridSize(32.0f), majorGridSize(128.0f), enabled(true) {
    color[0] = 0.3f; color[1] = 0.3f; color[2] = 0.3f; color[3] = 0.5f;
    majorColor[0] = 0.4f; majorColor[1] = 0.4f; majorColor[2] = 0.4f; majorColor[3] = 0.8f;
}

GridRenderer::~GridRenderer() {}

void GridRenderer::render(const JJM::Graphics::Camera* camera) {
    if (!enabled || !camera) return;
    
    // Render minor grid
    renderGrid(camera, gridSize, color);
    
    // Render major grid
    renderGrid(camera, majorGridSize, majorColor);
}

void GridRenderer::renderGrid(const JJM::Graphics::Camera* camera, float size, const float* gridColor) {
    if (!camera) return;
    
    // Calculate visible grid bounds from camera
    // Draw grid lines
}

// RulerTool implementation
RulerTool::RulerTool() : active(false) {}

RulerTool::~RulerTool() {}

void RulerTool::update(Input::InputManager* input, const JJM::Graphics::Camera* camera) {
    if (!input || !camera) return;
    
    // Handle ruler input
}

void RulerTool::render() {
    if (!active) return;
    
    // Draw line from start to end
    // Draw distance text
}

void RulerTool::startMeasurement(const Math::Vector2D& start) {
    startPoint = start;
    active = true;
}

void RulerTool::endMeasurement(const Math::Vector2D& end) {
    endPoint = end;
}

void RulerTool::clearMeasurement() {
    active = false;
}

float RulerTool::getDistance() const {
    float dx = endPoint.x - startPoint.x;
    float dy = endPoint.y - startPoint.y;
    return std::sqrt(dx * dx + dy * dy);
}

// LevelEditorToolbar implementation
LevelEditorToolbar::LevelEditorToolbar() : enabled(true) {}

LevelEditorToolbar::~LevelEditorToolbar() {}

void LevelEditorToolbar::update(Input::InputManager* input) {
    if (!input || !enabled) return;
    
    // Handle toolbar input
}

void LevelEditorToolbar::render() {
    if (!enabled) return;
    
    // Render toolbar buttons
}

void LevelEditorToolbar::addTool(const std::string& name, std::function<void()> callback) {
    Tool tool;
    tool.name = name;
    tool.callback = callback;
    tool.enabled = true;
    tools.push_back(tool);
}

void LevelEditorToolbar::removeTool(const std::string& name) {
    tools.erase(
        std::remove_if(tools.begin(), tools.end(),
            [&name](const Tool& tool) { return tool.name == name; }),
        tools.end());
}

void LevelEditorToolbar::setActiveTool(const std::string& name) {
    activeTool = name;
    
    // Execute tool callback
    for (auto& tool : tools) {
        if (tool.name == name && tool.callback) {
            tool.callback();
            break;
        }
    }
}

// EntityBrush implementation
EntityBrush::EntityBrush()
    : brushSize(32.0f), spacing(16.0f), randomRotation(false), 
      randomScale(false), scaleMin(0.8f), scaleMax(1.2f), 
      distanceSinceLastPaint(0.0f) {}

EntityBrush::~EntityBrush() {}

void EntityBrush::update(Input::InputManager* input, const JJM::Graphics::Camera* camera) {
    if (!input || !camera) return;
    
    // Handle brush input
    bool mouseDown = false; // input->isMouseButtonDown(MouseButton::Left)
    if (mouseDown) {
        Math::Vector2D mousePos;
        
        float distance = std::sqrt(
            (mousePos.x - lastPaintPos.x) * (mousePos.x - lastPaintPos.x) +
            (mousePos.y - lastPaintPos.y) * (mousePos.y - lastPaintPos.y)
        );
        
        distanceSinceLastPaint += distance;
        
        if (distanceSinceLastPaint >= spacing) {
            paint(mousePos);
            lastPaintPos = mousePos;
            distanceSinceLastPaint = 0.0f;
        }
    }
}

void EntityBrush::paint(const Math::Vector2D& worldPos) {
    // Create entity from template at position
    // Apply random rotation/scale if enabled
}

// LevelEditorTools implementation
LevelEditorTools& LevelEditorTools::getInstance() {
    static LevelEditorTools instance;
    return instance;
}

void LevelEditorTools::initialize() {
    enabled = true;
    gridRenderer.setEnabled(true);
}

void LevelEditorTools::shutdown() {
    enabled = false;
}

void LevelEditorTools::update(Input::InputManager* input, const JJM::Graphics::Camera* camera,
                              const std::vector<ECS::Entity*>& entities) {
    if (!enabled || !input || !camera) return;
    
    // Update all tools
    selectionTool.update(input, camera);
    manipulator.update(input, camera, selectionTool.getSelection());
    rulerTool.update(input, camera);
    toolbar.update(input);
    entityBrush.update(input, camera);
}

void LevelEditorTools::render() {
    if (!enabled) return;
    
    // Render all tools
    gridRenderer.render(nullptr);
    selectionTool.render();
    manipulator.render();
    rulerTool.render();
    toolbar.render();
}

} // namespace Editor
} // namespace JJM
