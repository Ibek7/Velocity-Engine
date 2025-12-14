#pragma once

#include "math/Vector2D.h"
#include "ecs/Entity.h"
#include "camera/Camera.h"
#include "input/InputManager.h"
#include <memory>
#include <vector>
#include <functional>

namespace JJM {
namespace Editor {

enum class GizmoType {
    None,
    Translate,
    Rotate,
    Scale
};

enum class GizmoAxis {
    None = 0,
    X = 1,
    Y = 2,
    Z = 4,
    XY = X | Y,
    XZ = X | Z,
    YZ = Y | Z,
    XYZ = X | Y | Z
};

class EditorGizmo {
public:
    EditorGizmo();
    ~EditorGizmo();
    
    void setType(GizmoType type) { this->type = type; }
    GizmoType getType() const { return type; }
    
    void setPosition(const Math::Vector2D& pos) { position = pos; }
    const Math::Vector2D& getPosition() const { return position; }
    
    void setSize(float size) { this->size = size; }
    float getSize() const { return size; }
    
    void update(Input::InputManager* input, const JJM::Graphics::Camera* camera);
    void render();
    
    bool isHovered() const { return hovered; }
    bool isDragging() const { return dragging; }
    GizmoAxis getActiveAxis() const { return activeAxis; }

private:
    GizmoType type;
    Math::Vector2D position;
    float size;
    bool hovered;
    bool dragging;
    GizmoAxis activeAxis;
    Math::Vector2D dragStartPos;
    
    GizmoAxis getAxisAtPosition(const Math::Vector2D& pos, const JJM::Graphics::Camera* camera);
};

class SelectionTool {
public:
    SelectionTool();
    ~SelectionTool();
    
    void update(Input::InputManager* input, const JJM::Graphics::Camera* camera);
    void render();
    
    void addToSelection(ECS::Entity* entity);
    void removeFromSelection(ECS::Entity* entity);
    void clearSelection();
    
    bool isSelected(ECS::Entity* entity) const;
    const std::vector<ECS::Entity*>& getSelection() const { return selectedEntities; }
    
    void setMultiSelectEnabled(bool enabled) { multiSelectEnabled = enabled; }
    bool isMultiSelectEnabled() const { return multiSelectEnabled; }
    
    void setOnSelectionChanged(std::function<void()> callback) {
        onSelectionChanged = callback;
    }

private:
    std::vector<ECS::Entity*> selectedEntities;
    bool multiSelectEnabled;
    bool isDragging;
    Math::Vector2D dragStartPos;
    Math::Vector2D dragEndPos;
    std::function<void()> onSelectionChanged;
    
    void performSelection(const Math::Vector2D& worldPos);
    void performBoxSelection(const Math::Vector2D& start, const Math::Vector2D& end);
};

class EntityManipulator {
public:
    EntityManipulator();
    ~EntityManipulator();
    
    void setGizmoType(GizmoType type) { gizmo.setType(type); }
    GizmoType getGizmoType() const { return gizmo.getType(); }
    
    void update(Input::InputManager* input, const JJM::Graphics::Camera* camera,
                const std::vector<ECS::Entity*>& selectedEntities);
    
    void render();
    
    void setSnapEnabled(bool enabled) { snapEnabled = enabled; }
    bool isSnapEnabled() const { return snapEnabled; }
    
    void setSnapValue(float value) { snapValue = value; }
    float getSnapValue() const { return snapValue; }

private:
    EditorGizmo gizmo;
    bool snapEnabled;
    float snapValue;
    
    Math::Vector2D applySnapping(const Math::Vector2D& value);
    void applyTranslation(ECS::Entity* entity, const Math::Vector2D& delta);
    void applyRotation(ECS::Entity* entity, float angle);
    void applyScale(ECS::Entity* entity, const Math::Vector2D& scale);
};

class GridRenderer {
public:
    GridRenderer();
    ~GridRenderer();
    
    void render(const JJM::Graphics::Camera* camera);
    
    void setGridSize(float size) { gridSize = size; }
    float getGridSize() const { return gridSize; }
    
    void setMajorGridSize(float size) { majorGridSize = size; }
    float getMajorGridSize() const { return majorGridSize; }
    
    void setEnabled(bool enabled) { this->enabled = enabled; }
    bool isEnabled() const { return enabled; }
    
    void setColor(float r, float g, float b, float a) {
        color[0] = r; color[1] = g; color[2] = b; color[3] = a;
    }
    
    void setMajorColor(float r, float g, float b, float a) {
        majorColor[0] = r; majorColor[1] = g; majorColor[2] = b; majorColor[3] = a;
    }

private:
    float gridSize;
    float majorGridSize;
    bool enabled;
    float color[4];
    float majorColor[4];
    
    void renderGrid(const JJM::Graphics::Camera* camera, float size, const float* gridColor);
};

class RulerTool {
public:
    RulerTool();
    ~RulerTool();
    
    void update(Input::InputManager* input, const JJM::Graphics::Camera* camera);
    void render();
    
    void startMeasurement(const Math::Vector2D& start);
    void endMeasurement(const Math::Vector2D& end);
    void clearMeasurement();
    
    bool isActive() const { return active; }
    float getDistance() const;
    const Math::Vector2D& getStartPoint() const { return startPoint; }
    const Math::Vector2D& getEndPoint() const { return endPoint; }

private:
    bool active;
    Math::Vector2D startPoint;
    Math::Vector2D endPoint;
};

class LevelEditorToolbar {
public:
    LevelEditorToolbar();
    ~LevelEditorToolbar();
    
    void update(Input::InputManager* input);
    void render();
    
    void addTool(const std::string& name, std::function<void()> callback);
    void removeTool(const std::string& name);
    
    void setActiveTool(const std::string& name);
    std::string getActiveTool() const { return activeTool; }
    
    void setEnabled(bool enabled) { this->enabled = enabled; }
    bool isEnabled() const { return enabled; }

private:
    struct Tool {
        std::string name;
        std::function<void()> callback;
        bool enabled;
    };
    
    std::vector<Tool> tools;
    std::string activeTool;
    bool enabled;
};

class EntityBrush {
public:
    EntityBrush();
    ~EntityBrush();
    
    void setTemplate(const std::string& templateName) { this->templateName = templateName; }
    const std::string& getTemplate() const { return templateName; }
    
    void setBrushSize(float size) { brushSize = size; }
    float getBrushSize() const { return brushSize; }
    
    void setSpacing(float spacing) { this->spacing = spacing; }
    float getSpacing() const { return spacing; }
    
    void setRandomRotation(bool enabled) { randomRotation = enabled; }
    bool isRandomRotation() const { return randomRotation; }
    
    void setRandomScale(bool enabled, float min = 0.8f, float max = 1.2f) {
        randomScale = enabled;
        scaleMin = min;
        scaleMax = max;
    }
    
    void update(Input::InputManager* input, const JJM::Graphics::Camera* camera);
    void paint(const Math::Vector2D& worldPos);

private:
    std::string templateName;
    float brushSize;
    float spacing;
    bool randomRotation;
    bool randomScale;
    float scaleMin;
    float scaleMax;
    Math::Vector2D lastPaintPos;
    float distanceSinceLastPaint;
};

class LevelEditorTools {
public:
    static LevelEditorTools& getInstance();
    
    void initialize();
    void shutdown();
    
    void update(Input::InputManager* input, const JJM::Graphics::Camera* camera,
                const std::vector<ECS::Entity*>& entities);
    void render();
    
    SelectionTool& getSelectionTool() { return selectionTool; }
    EntityManipulator& getManipulator() { return manipulator; }
    GridRenderer& getGridRenderer() { return gridRenderer; }
    RulerTool& getRulerTool() { return rulerTool; }
    LevelEditorToolbar& getToolbar() { return toolbar; }
    EntityBrush& getEntityBrush() { return entityBrush; }
    
    void setEnabled(bool enabled) { this->enabled = enabled; }
    bool isEnabled() const { return enabled; }

private:
    LevelEditorTools() {}
    ~LevelEditorTools() {}
    LevelEditorTools(const LevelEditorTools&) = delete;
    LevelEditorTools& operator=(const LevelEditorTools&) = delete;
    
    SelectionTool selectionTool;
    EntityManipulator manipulator;
    GridRenderer gridRenderer;
    RulerTool rulerTool;
    LevelEditorToolbar toolbar;
    EntityBrush entityBrush;
    bool enabled;
};

} // namespace Editor
} // namespace JJM
