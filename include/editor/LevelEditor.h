#ifndef LEVEL_EDITOR_H
#define LEVEL_EDITOR_H

#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <functional>
#include "math/Vector2D.h"
#include "graphics/Color.h"
#include "gui/GUISystem.h"
#include "ecs/Entity.h"
#include "scene/Scene.h"

namespace JJM {
namespace Editor {

enum class EditorMode {
    Select,
    Move,
    Rotate,
    Scale,
    Paint,
    Erase
};

enum class SnapMode {
    None,
    Grid,
    Object,
    Vertex
};

enum class EditorTool {
    Selection,
    Move,
    Rotate,
    Scale,
    Brush,
    Eraser,
    Eyedropper,
    Hand
};

class EditorCamera {
private:
    Math::Vector2D position;
    float zoom;
    Math::Vector2D viewportSize;
    
public:
    EditorCamera();
    
    void setPosition(const Math::Vector2D& pos) { position = pos; }
    const Math::Vector2D& getPosition() const { return position; }
    
    void setZoom(float z) { zoom = z; }
    float getZoom() const { return zoom; }
    
    void setViewportSize(const Math::Vector2D& size) { viewportSize = size; }
    const Math::Vector2D& getViewportSize() const { return viewportSize; }
    
    Math::Vector2D screenToWorld(const Math::Vector2D& screenPos) const;
    Math::Vector2D worldToScreen(const Math::Vector2D& worldPos) const;
    
    void pan(const Math::Vector2D& delta);
    void zoomAt(const Math::Vector2D& screenPos, float zoomDelta);
    
    bool isPointInView(const Math::Vector2D& worldPos) const;
    GUI::Rect getViewBounds() const;
};

class GridRenderer {
private:
    float gridSize;
    Math::Vector2D gridOffset;
    Graphics::Color gridColor;
    Graphics::Color majorGridColor;
    int majorGridSpacing;
    bool visible;
    
public:
    GridRenderer();
    
    void setGridSize(float size) { gridSize = size; }
    float getGridSize() const { return gridSize; }
    
    void setGridOffset(const Math::Vector2D& offset) { gridOffset = offset; }
    const Math::Vector2D& getGridOffset() const { return gridOffset; }
    
    void setVisible(bool vis) { visible = vis; }
    bool isVisible() const { return visible; }
    
    void render(const EditorCamera& camera);
    Math::Vector2D snapToGrid(const Math::Vector2D& position) const;
};

struct SelectionBox {
    Math::Vector2D start;
    Math::Vector2D end;
    bool active;
    Graphics::Color color;
    
    SelectionBox() : active(false), color(0.2f, 0.6f, 1.0f, 0.3f) {}
    
    GUI::Rect getBounds() const {
        float left = std::min(start.x, end.x);
        float top = std::min(start.y, end.y);
        float width = std::abs(end.x - start.x);
        float height = std::abs(end.y - start.y);
        return GUI::Rect(left, top, width, height);
    }
};

class EditorSelection {
private:
    std::vector<ECS::Entity*> selectedEntities;
    SelectionBox selectionBox;
    
public:
    void addEntity(ECS::Entity* entity);
    void removeEntity(ECS::Entity* entity);
    void clearSelection();
    void selectAll();
    
    bool isSelected(ECS::Entity* entity) const;
    const std::vector<ECS::Entity*>& getSelectedEntities() const { return selectedEntities; }
    size_t getSelectionCount() const { return selectedEntities.size(); }
    bool hasSelection() const { return !selectedEntities.empty(); }
    
    void startSelectionBox(const Math::Vector2D& start);
    void updateSelectionBox(const Math::Vector2D& end);
    void endSelectionBox();
    
    const SelectionBox& getSelectionBox() const { return selectionBox; }
    
    Math::Vector2D getSelectionCenter() const;
    GUI::Rect getSelectionBounds() const;
};

class Gizmo {
private:
    Math::Vector2D position;
    float size;
    EditorTool currentTool;
    bool visible;
    bool dragging;
    Math::Vector2D dragStart;
    Math::Vector2D dragOffset;
    
    enum class GizmoAxis {
        None,
        X,
        Y,
        XY
    };
    
    GizmoAxis hoveredAxis;
    GizmoAxis draggedAxis;
    
public:
    Gizmo();
    
    void setPosition(const Math::Vector2D& pos) { position = pos; }
    const Math::Vector2D& getPosition() const { return position; }
    
    void setTool(EditorTool tool) { currentTool = tool; }
    EditorTool getTool() const { return currentTool; }
    
    void setVisible(bool vis) { visible = vis; }
    bool isVisible() const { return visible; }
    
    void render(const EditorCamera& camera);
    bool handleInput(const Math::Vector2D& mousePos, bool mousePressed);
    
    bool isDragging() const { return dragging; }
    Math::Vector2D getDragDelta() const;
    
private:
    void renderMoveGizmo(const EditorCamera& camera);
    void renderRotateGizmo(const EditorCamera& camera);
    void renderScaleGizmo(const EditorCamera& camera);
    
    GizmoAxis getAxisAtPosition(const Math::Vector2D& mousePos, const EditorCamera& camera) const;
    Graphics::Color getAxisColor(GizmoAxis axis) const;
};

struct AssetItem {
    std::string name;
    std::string path;
    std::string type;
    std::string category;
    void* thumbnail; // Would be texture pointer
    size_t fileSize;
    std::string lastModified;
    
    AssetItem() : thumbnail(nullptr), fileSize(0) {}
};

class AssetBrowser {
private:
    std::string currentDirectory;
    std::vector<AssetItem> assets;
    std::unordered_map<std::string, std::vector<AssetItem>> assetCache;
    
    std::string selectedAsset;
    bool showPreview;
    float thumbnailSize;
    std::string searchFilter;
    std::string typeFilter;
    
public:
    AssetBrowser();
    
    void setDirectory(const std::string& directory);
    const std::string& getCurrentDirectory() const { return currentDirectory; }
    
    void refreshAssets();
    void searchAssets(const std::string& filter);
    void filterByType(const std::string& type);
    
    void render();
    
    const AssetItem* getSelectedAsset() const;
    void setSelectedAsset(const std::string& assetName);
    
    void setThumbnailSize(float size) { thumbnailSize = size; }
    float getThumbnailSize() const { return thumbnailSize; }
    
private:
    void scanDirectory(const std::string& directory);
    void generateThumbnail(AssetItem& asset);
    AssetItem createAssetItem(const std::string& filePath);
    bool matchesFilter(const AssetItem& asset) const;
};

struct PropertyValue {
    enum Type {
        Bool,
        Int,
        Float,
        String,
        Vector2,
        Color,
        Entity
    } type;
    
    union Data {
        bool boolValue;
        int intValue;
        float floatValue;
        Math::Vector2D vector2Value;
        Graphics::Color colorValue;
        ECS::Entity* entityValue;
        
        Data() {}
    } data;
    
    std::string stringValue;
    
    PropertyValue() : type(Bool) { data.boolValue = false; }
    PropertyValue(bool value) : type(Bool) { data.boolValue = value; }
    PropertyValue(int value) : type(Int) { data.intValue = value; }
    PropertyValue(float value) : type(Float) { data.floatValue = value; }
    PropertyValue(const std::string& value) : type(String), stringValue(value) {}
    PropertyValue(const Math::Vector2D& value) : type(Vector2) { data.vector2Value = value; }
    PropertyValue(const Graphics::Color& value) : type(Color) { data.colorValue = value; }
    PropertyValue(ECS::Entity* value) : type(Entity) { data.entityValue = value; }
};

struct Property {
    std::string name;
    std::string displayName;
    PropertyValue value;
    std::string category;
    std::string tooltip;
    bool readOnly;
    
    Property() : readOnly(false) {}
    Property(const std::string& name, const PropertyValue& value) 
        : name(name), displayName(name), value(value), readOnly(false) {}
};

class PropertyPanel {
private:
    std::vector<Property> properties;
    std::string currentCategory;
    std::unordered_map<std::string, bool> categoryExpanded;
    
    std::function<void(const Property&)> onPropertyChanged;
    
public:
    PropertyPanel();
    
    void clearProperties();
    void addProperty(const Property& property);
    void updateProperty(const std::string& name, const PropertyValue& value);
    
    void setPropertyChangeCallback(std::function<void(const Property&)> callback) {
        onPropertyChanged = callback;
    }
    
    void render();
    void inspectEntity(ECS::Entity* entity);
    
private:
    void renderProperty(Property& property);
    void renderBoolProperty(Property& property);
    void renderIntProperty(Property& property);
    void renderFloatProperty(Property& property);
    void renderStringProperty(Property& property);
    void renderVector2Property(Property& property);
    void renderColorProperty(Property& property);
    void renderEntityProperty(Property& property);
    
    std::vector<std::string> getCategories() const;
};

class SceneHierarchy {
private:
    Scene::Scene* currentScene;
    ECS::Entity* selectedEntity;
    std::string searchFilter;
    bool showOnlyVisible;
    
    std::function<void(ECS::Entity*)> onEntitySelected;
    std::function<void(ECS::Entity*)> onEntityDoubleClicked;
    
public:
    SceneHierarchy();
    
    void setScene(Scene::Scene* scene) { currentScene = scene; }
    Scene::Scene* getScene() const { return currentScene; }
    
    void setSelectedEntity(ECS::Entity* entity) { selectedEntity = entity; }
    ECS::Entity* getSelectedEntity() const { return selectedEntity; }
    
    void setSearchFilter(const std::string& filter) { searchFilter = filter; }
    void setShowOnlyVisible(bool show) { showOnlyVisible = show; }
    
    void setEntitySelectedCallback(std::function<void(ECS::Entity*)> callback) {
        onEntitySelected = callback;
    }
    
    void setEntityDoubleClickCallback(std::function<void(ECS::Entity*)> callback) {
        onEntityDoubleClicked = callback;
    }
    
    void render();
    
    ECS::Entity* createEntity(const std::string& name = "Entity");
    void deleteEntity(ECS::Entity* entity);
    void duplicateEntity(ECS::Entity* entity);
    
private:
    void renderEntityNode(ECS::Entity* entity);
    bool matchesFilter(ECS::Entity* entity) const;
    void handleEntityContextMenu(ECS::Entity* entity);
};

enum class EditorCommand {
    None,
    CreateEntity,
    DeleteEntity,
    MoveEntity,
    RotateEntity,
    ScaleEntity,
    ModifyProperty,
    DuplicateEntity,
    SelectEntity
};

struct EditorAction {
    EditorCommand command;
    std::string description;
    std::unordered_map<std::string, PropertyValue> beforeState;
    std::unordered_map<std::string, PropertyValue> afterState;
    std::vector<ECS::Entity*> affectedEntities;
    std::chrono::high_resolution_clock::time_point timestamp;
    
    EditorAction() : command(EditorCommand::None), 
                    timestamp(std::chrono::high_resolution_clock::now()) {}
};

class UndoRedoSystem {
private:
    std::vector<EditorAction> undoStack;
    std::vector<EditorAction> redoStack;
    size_t maxHistorySize;
    size_t currentIndex;
    
public:
    UndoRedoSystem(size_t maxSize = 100);
    
    void executeAction(const EditorAction& action);
    void undo();
    void redo();
    
    bool canUndo() const { return currentIndex > 0; }
    bool canRedo() const { return currentIndex < undoStack.size(); }
    
    void clearHistory();
    size_t getHistorySize() const { return undoStack.size(); }
    
    std::string getUndoDescription() const;
    std::string getRedoDescription() const;
    
private:
    void applyAction(const EditorAction& action, bool isUndo = false);
    void pruneRedoStack();
};

class LevelEditor {
private:
    static LevelEditor* instance;
    
    EditorCamera camera;
    GridRenderer grid;
    EditorSelection selection;
    Gizmo gizmo;
    
    std::unique_ptr<AssetBrowser> assetBrowser;
    std::unique_ptr<PropertyPanel> propertyPanel;
    std::unique_ptr<SceneHierarchy> sceneHierarchy;
    std::unique_ptr<UndoRedoSystem> undoRedo;
    
    Scene::Scene* currentScene;
    
    EditorMode currentMode;
    EditorTool currentTool;
    SnapMode snapMode;
    
    bool showGrid;
    bool showGizmos;
    bool showBounds;
    bool showOrigins;
    
    Math::Vector2D lastMousePosition;
    bool isDragging;
    bool isPanning;
    
    // UI Layout
    GUI::Rect viewportRect;
    GUI::Rect toolbarRect;
    GUI::Rect propertiesRect;
    GUI::Rect hierarchyRect;
    GUI::Rect assetsRect;
    
    LevelEditor();
    
public:
    static LevelEditor* getInstance();
    ~LevelEditor();
    
    void initialize();
    void shutdown();
    
    void setScene(Scene::Scene* scene);
    Scene::Scene* getCurrentScene() const { return currentScene; }
    
    void update(float deltaTime);
    void render();
    void handleInput();
    
    // Mode and Tool Management
    void setMode(EditorMode mode) { currentMode = mode; }
    EditorMode getMode() const { return currentMode; }
    
    void setTool(EditorTool tool);
    EditorTool getTool() const { return currentTool; }
    
    void setSnapMode(SnapMode mode) { snapMode = mode; }
    SnapMode getSnapMode() const { return snapMode; }
    
    // Display Options
    void setShowGrid(bool show) { showGrid = show; grid.setVisible(show); }
    bool getShowGrid() const { return showGrid; }
    
    void setShowGizmos(bool show) { showGizmos = show; gizmo.setVisible(show); }
    bool getShowGizmos() const { return showGizmos; }
    
    // Camera Control
    EditorCamera& getCamera() { return camera; }
    const EditorCamera& getCamera() const { return camera; }
    
    // Selection Management
    EditorSelection& getSelection() { return selection; }
    const EditorSelection& getSelection() const { return selection; }
    
    // Grid and Snapping
    GridRenderer& getGrid() { return grid; }
    Math::Vector2D snapPosition(const Math::Vector2D& position) const;
    
    // Undo/Redo
    UndoRedoSystem& getUndoRedo() { return *undoRedo; }
    
    // File Operations
    void newLevel();
    bool openLevel(const std::string& filename);
    bool saveLevel(const std::string& filename = "");
    bool saveLevelAs(const std::string& filename);
    
    void exportLevel(const std::string& filename);
    bool importAssets(const std::string& directory);
    
    // Entity Operations
    ECS::Entity* createEntity(const std::string& name = "Entity");
    void deleteSelectedEntities();
    void duplicateSelectedEntities();
    
    // Transform Operations
    void moveSelectedEntities(const Math::Vector2D& delta);
    void rotateSelectedEntities(float angleDelta);
    void scaleSelectedEntities(const Math::Vector2D& scaleDelta);
    
private:
    void setupUI();
    void renderToolbar();
    void renderViewport();
    void renderStatusBar();
    
    void handleViewportInput();
    void handleCameraControls();
    void handleEntityManipulation();
    
    void updateGizmo();
    void updateSelection();
    
    ECS::Entity* getEntityAtPosition(const Math::Vector2D& worldPos) const;
    void performRaycast(const Math::Vector2D& worldPos, std::vector<ECS::Entity*>& hits) const;
    
    void onEntitySelected(ECS::Entity* entity);
    void onPropertyChanged(const Property& property);
    
    void createDefaultLevel();
    std::string generateUniqueEntityName(const std::string& baseName) const;
};

// Editor utilities and helpers
namespace EditorUtils {
    std::string formatFileSize(size_t bytes);
    std::string getFileExtension(const std::string& filename);
    std::string getFileName(const std::string& path);
    std::string getDirectoryPath(const std::string& path);
    
    bool isImageFile(const std::string& filename);
    bool isAudioFile(const std::string& filename);
    bool isScriptFile(const std::string& filename);
    bool isSceneFile(const std::string& filename);
    
    Graphics::Color lerp(const Graphics::Color& a, const Graphics::Color& b, float t);
    float ease(float t);
    
    GUI::Rect expandRect(const GUI::Rect& rect, float amount);
    bool rectContainsPoint(const GUI::Rect& rect, const Math::Vector2D& point);
    bool rectIntersects(const GUI::Rect& a, const GUI::Rect& b);
    
    void drawWireRect(const GUI::Rect& rect, const Graphics::Color& color);
    void drawFilledRect(const GUI::Rect& rect, const Graphics::Color& color);
    void drawLine(const Math::Vector2D& start, const Math::Vector2D& end, const Graphics::Color& color);
    void drawCircle(const Math::Vector2D& center, float radius, const Graphics::Color& color);
    void drawArrow(const Math::Vector2D& start, const Math::Vector2D& end, const Graphics::Color& color);
}

} // namespace Editor
} // namespace JJM

#endif // LEVEL_EDITOR_H