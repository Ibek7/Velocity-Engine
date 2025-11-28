#include "editor/LevelEditor.h"
#include <iostream>
#include <sstream>
#include <algorithm>
#include <filesystem>
#include <fstream>

namespace JJM {
namespace Editor {

// Static initialization
LevelEditor* LevelEditor::instance = nullptr;

// EditorCamera implementation
EditorCamera::EditorCamera() : position(0, 0), zoom(1.0f), viewportSize(800, 600) {}

Math::Vector2D EditorCamera::screenToWorld(const Math::Vector2D& screenPos) const {
    Math::Vector2D worldPos;
    worldPos.x = (screenPos.x - viewportSize.x * 0.5f) / zoom + position.x;
    worldPos.y = (screenPos.y - viewportSize.y * 0.5f) / zoom + position.y;
    return worldPos;
}

Math::Vector2D EditorCamera::worldToScreen(const Math::Vector2D& worldPos) const {
    Math::Vector2D screenPos;
    screenPos.x = (worldPos.x - position.x) * zoom + viewportSize.x * 0.5f;
    screenPos.y = (worldPos.y - position.y) * zoom + viewportSize.y * 0.5f;
    return screenPos;
}

void EditorCamera::pan(const Math::Vector2D& delta) {
    position.x -= delta.x / zoom;
    position.y -= delta.y / zoom;
}

void EditorCamera::zoomAt(const Math::Vector2D& screenPos, float zoomDelta) {
    Math::Vector2D worldPos = screenToWorld(screenPos);
    
    zoom *= (1.0f + zoomDelta);
    zoom = std::clamp(zoom, 0.1f, 10.0f);
    
    // Adjust position to keep the same world point under the cursor
    Math::Vector2D newWorldPos = screenToWorld(screenPos);
    position.x += worldPos.x - newWorldPos.x;
    position.y += worldPos.y - newWorldPos.y;
}

bool EditorCamera::isPointInView(const Math::Vector2D& worldPos) const {
    GUI::Rect viewBounds = getViewBounds();
    return viewBounds.contains(worldPos.x, worldPos.y);
}

GUI::Rect EditorCamera::getViewBounds() const {
    float halfWidth = viewportSize.x * 0.5f / zoom;
    float halfHeight = viewportSize.y * 0.5f / zoom;
    
    return GUI::Rect(
        position.x - halfWidth,
        position.y - halfHeight,
        halfWidth * 2,
        halfHeight * 2
    );
}

// GridRenderer implementation
GridRenderer::GridRenderer() 
    : gridSize(32.0f), gridOffset(0, 0), 
      gridColor(0.3f, 0.3f, 0.3f, 0.5f),
      majorGridColor(0.5f, 0.5f, 0.5f, 0.8f),
      majorGridSpacing(4), visible(true) {}

void GridRenderer::render(const EditorCamera& camera) {
    if (!visible) return;
    
    GUI::Rect viewBounds = camera.getViewBounds();
    
    float scaledGridSize = gridSize * camera.getZoom();
    
    // Don't draw grid if it's too small or too large
    if (scaledGridSize < 4.0f || scaledGridSize > 200.0f) return;
    
    // Calculate grid start/end positions
    int startX = static_cast<int>((viewBounds.x - gridOffset.x) / gridSize) - 1;
    int endX = static_cast<int>((viewBounds.x + viewBounds.width - gridOffset.x) / gridSize) + 1;
    int startY = static_cast<int>((viewBounds.y - gridOffset.y) / gridSize) - 1;
    int endY = static_cast<int>((viewBounds.y + viewBounds.height - gridOffset.y) / gridSize) + 1;
    
    // Mock grid rendering
    std::cout << "Rendering grid: " << startX << "," << startY << " to " << endX << "," << endY 
              << " (grid size: " << gridSize << ")" << std::endl;
    
    // Would actually render grid lines here
    for (int x = startX; x <= endX; x++) {
        bool isMajor = (x % majorGridSpacing) == 0;
        Graphics::Color color = isMajor ? majorGridColor : gridColor;
        
        float worldX = x * gridSize + gridOffset.x;
        Math::Vector2D start = camera.worldToScreen(Math::Vector2D(worldX, viewBounds.y));
        Math::Vector2D end = camera.worldToScreen(Math::Vector2D(worldX, viewBounds.y + viewBounds.height));
        
        // Mock line drawing
    }
    
    for (int y = startY; y <= endY; y++) {
        bool isMajor = (y % majorGridSpacing) == 0;
        Graphics::Color color = isMajor ? majorGridColor : gridColor;
        
        float worldY = y * gridSize + gridOffset.y;
        Math::Vector2D start = camera.worldToScreen(Math::Vector2D(viewBounds.x, worldY));
        Math::Vector2D end = camera.worldToScreen(Math::Vector2D(viewBounds.x + viewBounds.width, worldY));
        
        // Mock line drawing
    }
}

Math::Vector2D GridRenderer::snapToGrid(const Math::Vector2D& position) const {
    Math::Vector2D snapped;
    snapped.x = std::round((position.x - gridOffset.x) / gridSize) * gridSize + gridOffset.x;
    snapped.y = std::round((position.y - gridOffset.y) / gridSize) * gridSize + gridOffset.y;
    return snapped;
}

// EditorSelection implementation
void EditorSelection::addEntity(ECS::Entity* entity) {
    if (!entity) return;
    
    auto it = std::find(selectedEntities.begin(), selectedEntities.end(), entity);
    if (it == selectedEntities.end()) {
        selectedEntities.push_back(entity);
        std::cout << "Selected entity: " << entity << std::endl;
    }
}

void EditorSelection::removeEntity(ECS::Entity* entity) {
    auto it = std::find(selectedEntities.begin(), selectedEntities.end(), entity);
    if (it != selectedEntities.end()) {
        selectedEntities.erase(it);
        std::cout << "Deselected entity: " << entity << std::endl;
    }
}

void EditorSelection::clearSelection() {
    selectedEntities.clear();
    std::cout << "Selection cleared" << std::endl;
}

void EditorSelection::selectAll() {
    // Would select all entities in the current scene
    std::cout << "Select all entities" << std::endl;
}

bool EditorSelection::isSelected(ECS::Entity* entity) const {
    auto it = std::find(selectedEntities.begin(), selectedEntities.end(), entity);
    return it != selectedEntities.end();
}

void EditorSelection::startSelectionBox(const Math::Vector2D& start) {
    selectionBox.start = start;
    selectionBox.end = start;
    selectionBox.active = true;
}

void EditorSelection::updateSelectionBox(const Math::Vector2D& end) {
    selectionBox.end = end;
}

void EditorSelection::endSelectionBox() {
    selectionBox.active = false;
    
    // Would perform box selection here
    GUI::Rect bounds = selectionBox.getBounds();
    std::cout << "Selection box completed: " << bounds.x << "," << bounds.y 
              << " " << bounds.width << "x" << bounds.height << std::endl;
}

Math::Vector2D EditorSelection::getSelectionCenter() const {
    if (selectedEntities.empty()) return Math::Vector2D(0, 0);
    
    Math::Vector2D center(0, 0);
    for (const auto* entity : selectedEntities) {
        // Would get entity position and add to center
        center.x += 0; // entity->getPosition().x
        center.y += 0; // entity->getPosition().y
    }
    
    center.x /= selectedEntities.size();
    center.y /= selectedEntities.size();
    
    return center;
}

GUI::Rect EditorSelection::getSelectionBounds() const {
    if (selectedEntities.empty()) return GUI::Rect(0, 0, 0, 0);
    
    // Would calculate actual bounds from selected entities
    return GUI::Rect(0, 0, 100, 100);
}

// Gizmo implementation
Gizmo::Gizmo() 
    : position(0, 0), size(50.0f), currentTool(EditorTool::Move),
      visible(false), dragging(false), dragStart(0, 0), dragOffset(0, 0),
      hoveredAxis(GizmoAxis::None), draggedAxis(GizmoAxis::None) {}

void Gizmo::render(const EditorCamera& camera) {
    if (!visible) return;
    
    switch (currentTool) {
        case EditorTool::Move:
            renderMoveGizmo(camera);
            break;
        case EditorTool::Rotate:
            renderRotateGizmo(camera);
            break;
        case EditorTool::Scale:
            renderScaleGizmo(camera);
            break;
        default:
            break;
    }
}

bool Gizmo::handleInput(const Math::Vector2D& mousePos, bool mousePressed) {
    if (!visible) return false;
    
    if (!dragging && mousePressed) {
        // Start dragging if mouse is over gizmo
        hoveredAxis = getAxisAtPosition(mousePos, EditorCamera()); // Would pass actual camera
        
        if (hoveredAxis != GizmoAxis::None) {
            dragging = true;
            draggedAxis = hoveredAxis;
            dragStart = mousePos;
            dragOffset = Math::Vector2D(0, 0);
            return true;
        }
    } else if (dragging && !mousePressed) {
        // End dragging
        dragging = false;
        draggedAxis = GizmoAxis::None;
        return true;
    } else if (dragging) {
        // Update drag
        dragOffset = mousePos - dragStart;
        return true;
    } else {
        // Update hover state
        hoveredAxis = getAxisAtPosition(mousePos, EditorCamera());
    }
    
    return false;
}

Math::Vector2D Gizmo::getDragDelta() const {
    return dragOffset;
}

void Gizmo::renderMoveGizmo(const EditorCamera& camera) {
    Math::Vector2D screenPos = camera.worldToScreen(position);
    
    // Mock rendering of move gizmo with X and Y axes
    std::cout << "Rendering move gizmo at screen pos: " << screenPos.x << "," << screenPos.y << std::endl;
    
    // Would render X axis (red arrow)
    Graphics::Color xColor = getAxisColor(GizmoAxis::X);
    
    // Would render Y axis (green arrow)  
    Graphics::Color yColor = getAxisColor(GizmoAxis::Y);
    
    // Would render center square
    Graphics::Color centerColor = getAxisColor(GizmoAxis::XY);
}

void Gizmo::renderRotateGizmo(const EditorCamera& camera) {
    Math::Vector2D screenPos = camera.worldToScreen(position);
    
    std::cout << "Rendering rotate gizmo at screen pos: " << screenPos.x << "," << screenPos.y << std::endl;
    
    // Would render rotation circle
}

void Gizmo::renderScaleGizmo(const EditorCamera& camera) {
    Math::Vector2D screenPos = camera.worldToScreen(position);
    
    std::cout << "Rendering scale gizmo at screen pos: " << screenPos.x << "," << screenPos.y << std::endl;
    
    // Would render scale handles
}

Gizmo::GizmoAxis Gizmo::getAxisAtPosition(const Math::Vector2D& mousePos, const EditorCamera& camera) const {
    Math::Vector2D screenPos = camera.worldToScreen(position);
    Math::Vector2D delta = mousePos - screenPos;
    
    float threshold = 10.0f;
    
    if (std::abs(delta.x) < threshold && std::abs(delta.y) < threshold) {
        return GizmoAxis::XY; // Center
    } else if (std::abs(delta.y) < threshold && delta.x > 0 && delta.x < size) {
        return GizmoAxis::X; // X axis
    } else if (std::abs(delta.x) < threshold && delta.y > 0 && delta.y < size) {
        return GizmoAxis::Y; // Y axis
    }
    
    return GizmoAxis::None;
}

Graphics::Color Gizmo::getAxisColor(GizmoAxis axis) const {
    bool isHovered = (axis == hoveredAxis);
    bool isDragged = (axis == draggedAxis);
    
    switch (axis) {
        case GizmoAxis::X:
            return isDragged ? Graphics::Color(1.0f, 0.7f, 0.7f, 1.0f) :
                   isHovered ? Graphics::Color(1.0f, 0.5f, 0.5f, 1.0f) :
                              Graphics::Color(1.0f, 0.3f, 0.3f, 1.0f);
        case GizmoAxis::Y:
            return isDragged ? Graphics::Color(0.7f, 1.0f, 0.7f, 1.0f) :
                   isHovered ? Graphics::Color(0.5f, 1.0f, 0.5f, 1.0f) :
                              Graphics::Color(0.3f, 1.0f, 0.3f, 1.0f);
        case GizmoAxis::XY:
            return isDragged ? Graphics::Color(0.7f, 0.7f, 1.0f, 1.0f) :
                   isHovered ? Graphics::Color(0.5f, 0.5f, 1.0f, 1.0f) :
                              Graphics::Color(0.3f, 0.3f, 1.0f, 1.0f);
        default:
            return Graphics::Color(0.5f, 0.5f, 0.5f, 1.0f);
    }
}

// AssetBrowser implementation
AssetBrowser::AssetBrowser() 
    : showPreview(true), thumbnailSize(64.0f), 
      currentDirectory("./assets") {
    refreshAssets();
}

void AssetBrowser::setDirectory(const std::string& directory) {
    currentDirectory = directory;
    refreshAssets();
}

void AssetBrowser::refreshAssets() {
    assets.clear();
    
    // Check if directory exists
    if (!std::filesystem::exists(currentDirectory)) {
        std::cout << "Asset directory does not exist: " << currentDirectory << std::endl;
        return;
    }
    
    scanDirectory(currentDirectory);
    
    std::cout << "Loaded " << assets.size() << " assets from " << currentDirectory << std::endl;
}

void AssetBrowser::searchAssets(const std::string& filter) {
    searchFilter = filter;
    // Would filter the displayed assets
}

void AssetBrowser::filterByType(const std::string& type) {
    typeFilter = type;
    // Would filter the displayed assets by type
}

void AssetBrowser::render() {
    // Mock asset browser rendering
    std::cout << "Rendering asset browser with " << assets.size() << " assets" << std::endl;
    
    if (!searchFilter.empty()) {
        std::cout << "Search filter: " << searchFilter << std::endl;
    }
    
    if (!typeFilter.empty()) {
        std::cout << "Type filter: " << typeFilter << std::endl;
    }
    
    // Would render asset grid/list here
    for (const auto& asset : assets) {
        if (matchesFilter(asset)) {
            std::cout << "Asset: " << asset.name << " (" << asset.type << ")" << std::endl;
        }
    }
}

const AssetItem* AssetBrowser::getSelectedAsset() const {
    if (selectedAsset.empty()) return nullptr;
    
    for (const auto& asset : assets) {
        if (asset.name == selectedAsset) {
            return &asset;
        }
    }
    
    return nullptr;
}

void AssetBrowser::setSelectedAsset(const std::string& assetName) {
    selectedAsset = assetName;
    std::cout << "Selected asset: " << assetName << std::endl;
}

void AssetBrowser::scanDirectory(const std::string& directory) {
    try {
        for (const auto& entry : std::filesystem::directory_iterator(directory)) {
            if (entry.is_regular_file()) {
                AssetItem asset = createAssetItem(entry.path().string());
                assets.push_back(asset);
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Error scanning directory " << directory << ": " << e.what() << std::endl;
    }
}

AssetItem AssetBrowser::createAssetItem(const std::string& filePath) {
    AssetItem asset;
    asset.path = filePath;
    asset.name = EditorUtils::getFileName(filePath);
    
    std::string ext = EditorUtils::getFileExtension(filePath);
    
    if (EditorUtils::isImageFile(filePath)) {
        asset.type = "Image";
        asset.category = "Graphics";
    } else if (EditorUtils::isAudioFile(filePath)) {
        asset.type = "Audio";
        asset.category = "Audio";
    } else if (EditorUtils::isScriptFile(filePath)) {
        asset.type = "Script";
        asset.category = "Scripts";
    } else if (EditorUtils::isSceneFile(filePath)) {
        asset.type = "Scene";
        asset.category = "Scenes";
    } else {
        asset.type = "Unknown";
        asset.category = "Other";
    }
    
    try {
        asset.fileSize = std::filesystem::file_size(filePath);
        auto ftime = std::filesystem::last_write_time(filePath);
        // Would convert to string representation
        asset.lastModified = "2024-01-01 12:00:00";
    } catch (const std::exception&) {
        asset.fileSize = 0;
        asset.lastModified = "Unknown";
    }
    
    return asset;
}

bool AssetBrowser::matchesFilter(const AssetItem& asset) const {
    if (!searchFilter.empty()) {
        std::string lowerName = asset.name;
        std::string lowerFilter = searchFilter;
        std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);
        std::transform(lowerFilter.begin(), lowerFilter.end(), lowerFilter.begin(), ::tolower);
        
        if (lowerName.find(lowerFilter) == std::string::npos) {
            return false;
        }
    }
    
    if (!typeFilter.empty() && asset.type != typeFilter) {
        return false;
    }
    
    return true;
}

// PropertyPanel implementation
PropertyPanel::PropertyPanel() {}

void PropertyPanel::clearProperties() {
    properties.clear();
}

void PropertyPanel::addProperty(const Property& property) {
    properties.push_back(property);
}

void PropertyPanel::updateProperty(const std::string& name, const PropertyValue& value) {
    for (auto& prop : properties) {
        if (prop.name == name) {
            prop.value = value;
            break;
        }
    }
}

void PropertyPanel::render() {
    std::cout << "Rendering property panel with " << properties.size() << " properties" << std::endl;
    
    auto categories = getCategories();
    
    for (const auto& category : categories) {
        bool expanded = categoryExpanded[category];
        
        std::cout << "Category: " << category << (expanded ? " (expanded)" : " (collapsed)") << std::endl;
        
        if (expanded) {
            for (auto& property : properties) {
                if (property.category == category || (category == "General" && property.category.empty())) {
                    renderProperty(property);
                }
            }
        }
    }
}

void PropertyPanel::inspectEntity(ECS::Entity* entity) {
    clearProperties();
    
    if (!entity) return;
    
    // Would inspect entity components and add properties
    std::cout << "Inspecting entity: " << entity << std::endl;
    
    // Add basic entity properties
    addProperty(Property("Name", PropertyValue("Entity")));
    addProperty(Property("Position", PropertyValue(Math::Vector2D(0, 0))));
    addProperty(Property("Visible", PropertyValue(true)));
}

void PropertyPanel::renderProperty(Property& property) {
    std::cout << "Property: " << property.displayName << std::endl;
    
    switch (property.value.type) {
        case PropertyValue::Bool:
            renderBoolProperty(property);
            break;
        case PropertyValue::Int:
            renderIntProperty(property);
            break;
        case PropertyValue::Float:
            renderFloatProperty(property);
            break;
        case PropertyValue::String:
            renderStringProperty(property);
            break;
        case PropertyValue::Vector2:
            renderVector2Property(property);
            break;
        case PropertyValue::Color:
            renderColorProperty(property);
            break;
        case PropertyValue::Entity:
            renderEntityProperty(property);
            break;
    }
}

void PropertyPanel::renderBoolProperty(Property& property) {
    // Mock checkbox rendering
    std::cout << "  Checkbox: " << (property.value.data.boolValue ? "true" : "false") << std::endl;
}

void PropertyPanel::renderIntProperty(Property& property) {
    // Mock integer input rendering
    std::cout << "  Integer: " << property.value.data.intValue << std::endl;
}

void PropertyPanel::renderFloatProperty(Property& property) {
    // Mock float input rendering
    std::cout << "  Float: " << property.value.data.floatValue << std::endl;
}

void PropertyPanel::renderStringProperty(Property& property) {
    // Mock text input rendering
    std::cout << "  String: " << property.value.stringValue << std::endl;
}

void PropertyPanel::renderVector2Property(Property& property) {
    // Mock Vector2 input rendering
    Math::Vector2D& vec = property.value.data.vector2Value;
    std::cout << "  Vector2: (" << vec.x << ", " << vec.y << ")" << std::endl;
}

void PropertyPanel::renderColorProperty(Property& property) {
    // Mock color picker rendering
    Graphics::Color& color = property.value.data.colorValue;
    std::cout << "  Color: (" << color.r << ", " << color.g << ", " << color.b << ", " << color.a << ")" << std::endl;
}

void PropertyPanel::renderEntityProperty(Property& property) {
    // Mock entity reference rendering
    std::cout << "  Entity: " << property.value.data.entityValue << std::endl;
}

std::vector<std::string> PropertyPanel::getCategories() const {
    std::vector<std::string> categories;
    
    for (const auto& property : properties) {
        std::string category = property.category.empty() ? "General" : property.category;
        
        if (std::find(categories.begin(), categories.end(), category) == categories.end()) {
            categories.push_back(category);
        }
    }
    
    if (categories.empty()) {
        categories.push_back("General");
    }
    
    return categories;
}

// SceneHierarchy implementation
SceneHierarchy::SceneHierarchy() 
    : currentScene(nullptr), selectedEntity(nullptr), showOnlyVisible(false) {}

void SceneHierarchy::render() {
    std::cout << "Rendering scene hierarchy" << std::endl;
    
    if (!currentScene) {
        std::cout << "No scene loaded" << std::endl;
        return;
    }
    
    // Would render entity tree here
    std::cout << "Scene entities:" << std::endl;
    
    // Mock entity rendering
    for (int i = 0; i < 5; i++) {
        std::cout << "  Entity_" << i << std::endl;
    }
}

ECS::Entity* SceneHierarchy::createEntity(const std::string& name) {
    if (!currentScene) return nullptr;
    
    // Would create actual entity
    std::cout << "Creating entity: " << name << std::endl;
    
    return nullptr; // Would return actual entity
}

void SceneHierarchy::deleteEntity(ECS::Entity* entity) {
    if (!entity || !currentScene) return;
    
    std::cout << "Deleting entity: " << entity << std::endl;
    
    if (selectedEntity == entity) {
        selectedEntity = nullptr;
    }
}

void SceneHierarchy::duplicateEntity(ECS::Entity* entity) {
    if (!entity || !currentScene) return;
    
    std::cout << "Duplicating entity: " << entity << std::endl;
}

void SceneHierarchy::renderEntityNode(ECS::Entity* entity) {
    if (!entity) return;
    
    bool isSelected = (entity == selectedEntity);
    bool isVisible = true; // Would get from entity
    
    if (showOnlyVisible && !isVisible) return;
    if (!matchesFilter(entity)) return;
    
    std::cout << "  Entity node: " << entity << (isSelected ? " (selected)" : "") << std::endl;
}

bool SceneHierarchy::matchesFilter(ECS::Entity* entity) const {
    if (searchFilter.empty()) return true;
    
    // Would check entity name against filter
    return true;
}

void SceneHierarchy::handleEntityContextMenu(ECS::Entity* entity) {
    std::cout << "Entity context menu for: " << entity << std::endl;
    
    // Would show context menu with options:
    // - Duplicate
    // - Delete
    // - Rename
    // - Add Component
    // etc.
}

// UndoRedoSystem implementation
UndoRedoSystem::UndoRedoSystem(size_t maxSize) : maxHistorySize(maxSize), currentIndex(0) {}

void UndoRedoSystem::executeAction(const EditorAction& action) {
    // Remove any redo history
    pruneRedoStack();
    
    // Apply the action
    applyAction(action);
    
    // Add to undo stack
    undoStack.push_back(action);
    currentIndex++;
    
    // Limit stack size
    if (undoStack.size() > maxHistorySize) {
        undoStack.erase(undoStack.begin());
        currentIndex--;
    }
    
    std::cout << "Executed action: " << action.description << std::endl;
}

void UndoRedoSystem::undo() {
    if (!canUndo()) return;
    
    currentIndex--;
    const EditorAction& action = undoStack[currentIndex];
    
    // Apply reverse action
    applyAction(action, true);
    
    std::cout << "Undid action: " << action.description << std::endl;
}

void UndoRedoSystem::redo() {
    if (!canRedo()) return;
    
    const EditorAction& action = undoStack[currentIndex];
    currentIndex++;
    
    // Apply action again
    applyAction(action);
    
    std::cout << "Redid action: " << action.description << std::endl;
}

void UndoRedoSystem::clearHistory() {
    undoStack.clear();
    redoStack.clear();
    currentIndex = 0;
    std::cout << "Cleared undo/redo history" << std::endl;
}

std::string UndoRedoSystem::getUndoDescription() const {
    if (!canUndo()) return "";
    return undoStack[currentIndex - 1].description;
}

std::string UndoRedoSystem::getRedoDescription() const {
    if (!canRedo()) return "";
    return undoStack[currentIndex].description;
}

void UndoRedoSystem::applyAction(const EditorAction& action, bool isUndo) {
    // Would apply the action to the scene/entities
    std::cout << "Applying action: " << action.description << (isUndo ? " (undo)" : "") << std::endl;
}

void UndoRedoSystem::pruneRedoStack() {
    if (currentIndex < undoStack.size()) {
        undoStack.erase(undoStack.begin() + currentIndex, undoStack.end());
    }
}

// LevelEditor implementation
LevelEditor::LevelEditor() 
    : currentScene(nullptr), currentMode(EditorMode::Select), 
      currentTool(EditorTool::Selection), snapMode(SnapMode::Grid),
      showGrid(true), showGizmos(true), showBounds(false), showOrigins(false),
      isDragging(false), isPanning(false) {}

LevelEditor* LevelEditor::getInstance() {
    if (!instance) {
        instance = new LevelEditor();
    }
    return instance;
}

LevelEditor::~LevelEditor() {
    shutdown();
}

void LevelEditor::initialize() {
    std::cout << "Initializing Level Editor" << std::endl;
    
    assetBrowser = std::make_unique<AssetBrowser>();
    propertyPanel = std::make_unique<PropertyPanel>();
    sceneHierarchy = std::make_unique<SceneHierarchy>();
    undoRedo = std::make_unique<UndoRedoSystem>();
    
    setupUI();
    createDefaultLevel();
    
    // Set up callbacks
    propertyPanel->setPropertyChangeCallback(
        [this](const Property& prop) { onPropertyChanged(prop); }
    );
    
    sceneHierarchy->setEntitySelectedCallback(
        [this](ECS::Entity* entity) { onEntitySelected(entity); }
    );
}

void LevelEditor::shutdown() {
    std::cout << "Shutting down Level Editor" << std::endl;
    
    assetBrowser.reset();
    propertyPanel.reset();
    sceneHierarchy.reset();
    undoRedo.reset();
}

void LevelEditor::setScene(Scene::Scene* scene) {
    currentScene = scene;
    sceneHierarchy->setScene(scene);
    selection.clearSelection();
}

void LevelEditor::update(float deltaTime) {
    // Update editor systems
    if (showGrid) {
        // Update grid if needed
    }
    
    updateGizmo();
    updateSelection();
}

void LevelEditor::render() {
    // Render viewport
    renderViewport();
    
    // Render UI panels
    renderToolbar();
    
    // Render individual panels
    if (assetBrowser) assetBrowser->render();
    if (propertyPanel) propertyPanel->render();
    if (sceneHierarchy) sceneHierarchy->render();
    
    renderStatusBar();
}

void LevelEditor::handleInput() {
    handleViewportInput();
    handleCameraControls();
    handleEntityManipulation();
}

void LevelEditor::setTool(EditorTool tool) {
    currentTool = tool;
    gizmo.setTool(tool);
    std::cout << "Changed tool to: " << static_cast<int>(tool) << std::endl;
}

Math::Vector2D LevelEditor::snapPosition(const Math::Vector2D& position) const {
    switch (snapMode) {
        case SnapMode::Grid:
            return grid.snapToGrid(position);
        case SnapMode::Object:
            // Would snap to nearby objects
            return position;
        case SnapMode::Vertex:
            // Would snap to vertices
            return position;
        default:
            return position;
    }
}

bool LevelEditor::openLevel(const std::string& filename) {
    std::cout << "Opening level: " << filename << std::endl;
    
    // Would load level from file
    // For now, just create a default level
    createDefaultLevel();
    
    return true;
}

bool LevelEditor::saveLevel(const std::string& filename) {
    std::string saveFilename = filename.empty() ? "untitled.level" : filename;
    
    std::cout << "Saving level: " << saveFilename << std::endl;
    
    // Would serialize current scene to file
    
    return true;
}

ECS::Entity* LevelEditor::createEntity(const std::string& name) {
    if (!currentScene) return nullptr;
    
    EditorAction action;
    action.command = EditorCommand::CreateEntity;
    action.description = "Create " + name;
    
    undoRedo->executeAction(action);
    
    // Would create actual entity
    std::cout << "Created entity: " << name << std::endl;
    
    return nullptr; // Would return actual entity
}

void LevelEditor::deleteSelectedEntities() {
    if (selection.getSelectionCount() == 0) return;
    
    EditorAction action;
    action.command = EditorCommand::DeleteEntity;
    action.description = "Delete " + std::to_string(selection.getSelectionCount()) + " entities";
    action.affectedEntities = selection.getSelectedEntities();
    
    undoRedo->executeAction(action);
    
    selection.clearSelection();
}

void LevelEditor::setupUI() {
    // Set up UI layout
    viewportRect = GUI::Rect(200, 50, 600, 450);
    toolbarRect = GUI::Rect(0, 0, 800, 50);
    propertiesRect = GUI::Rect(600, 50, 200, 300);
    hierarchyRect = GUI::Rect(0, 50, 200, 300);
    assetsRect = GUI::Rect(0, 350, 200, 250);
}

void LevelEditor::renderToolbar() {
    std::cout << "Rendering editor toolbar" << std::endl;
    
    // Would render tool buttons, mode selectors, etc.
}

void LevelEditor::renderViewport() {
    std::cout << "Rendering editor viewport" << std::endl;
    
    // Render grid
    if (showGrid) {
        grid.render(camera);
    }
    
    // Render scene entities
    if (currentScene) {
        // Would render scene here
    }
    
    // Render selection
    if (selection.getSelectionBox().active) {
        // Render selection box
    }
    
    // Render selected entity bounds
    if (showBounds && selection.hasSelection()) {
        // Render selection bounds
    }
    
    // Render gizmo
    if (showGizmos && selection.hasSelection()) {
        gizmo.render(camera);
    }
}

void LevelEditor::renderStatusBar() {
    std::cout << "Rendering status bar" << std::endl;
    
    // Would show current tool, snap mode, camera position, etc.
}

void LevelEditor::updateGizmo() {
    if (selection.hasSelection()) {
        Math::Vector2D center = selection.getSelectionCenter();
        gizmo.setPosition(center);
        gizmo.setVisible(showGizmos);
    } else {
        gizmo.setVisible(false);
    }
}

void LevelEditor::updateSelection() {
    // Update selection visualization
}

void LevelEditor::onEntitySelected(ECS::Entity* entity) {
    selection.clearSelection();
    if (entity) {
        selection.addEntity(entity);
    }
    
    propertyPanel->inspectEntity(entity);
}

void LevelEditor::onPropertyChanged(const Property& property) {
    EditorAction action;
    action.command = EditorCommand::ModifyProperty;
    action.description = "Change " + property.displayName;
    
    undoRedo->executeAction(action);
}

void LevelEditor::createDefaultLevel() {
    std::cout << "Creating default level" << std::endl;
    
    // Would create a basic scene with some default entities
}

std::string LevelEditor::generateUniqueEntityName(const std::string& baseName) const {
    int counter = 1;
    std::string name;
    
    do {
        name = baseName + "_" + std::to_string(counter++);
    } while (false); // Would check if name exists
    
    return name;
}

// EditorUtils implementation
namespace EditorUtils {
    std::string formatFileSize(size_t bytes) {
        const char* units[] = {"B", "KB", "MB", "GB"};
        int unitIndex = 0;
        double size = static_cast<double>(bytes);
        
        while (size >= 1024.0 && unitIndex < 3) {
            size /= 1024.0;
            unitIndex++;
        }
        
        std::stringstream ss;
        ss.precision(2);
        ss << std::fixed << size << " " << units[unitIndex];
        return ss.str();
    }
    
    std::string getFileExtension(const std::string& filename) {
        size_t dot = filename.find_last_of('.');
        if (dot != std::string::npos) {
            return filename.substr(dot + 1);
        }
        return "";
    }
    
    std::string getFileName(const std::string& path) {
        size_t slash = path.find_last_of("/\\");
        if (slash != std::string::npos) {
            return path.substr(slash + 1);
        }
        return path;
    }
    
    std::string getDirectoryPath(const std::string& path) {
        size_t slash = path.find_last_of("/\\");
        if (slash != std::string::npos) {
            return path.substr(0, slash);
        }
        return "";
    }
    
    bool isImageFile(const std::string& filename) {
        std::string ext = getFileExtension(filename);
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
        return ext == "png" || ext == "jpg" || ext == "jpeg" || ext == "bmp" || ext == "tga";
    }
    
    bool isAudioFile(const std::string& filename) {
        std::string ext = getFileExtension(filename);
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
        return ext == "wav" || ext == "ogg" || ext == "mp3" || ext == "flac";
    }
    
    bool isScriptFile(const std::string& filename) {
        std::string ext = getFileExtension(filename);
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
        return ext == "lua" || ext == "js" || ext == "py" || ext == "cs";
    }
    
    bool isSceneFile(const std::string& filename) {
        std::string ext = getFileExtension(filename);
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
        return ext == "scene" || ext == "level" || ext == "map";
    }
    
    Graphics::Color lerp(const Graphics::Color& a, const Graphics::Color& b, float t) {
        return Graphics::Color(
            a.r + (b.r - a.r) * t,
            a.g + (b.g - a.g) * t,
            a.b + (b.b - a.b) * t,
            a.a + (b.a - a.a) * t
        );
    }
    
    float ease(float t) {
        return t * t * (3.0f - 2.0f * t);
    }
    
    GUI::Rect expandRect(const GUI::Rect& rect, float amount) {
        return GUI::Rect(
            rect.x - amount,
            rect.y - amount,
            rect.width + amount * 2,
            rect.height + amount * 2
        );
    }
    
    bool rectContainsPoint(const GUI::Rect& rect, const Math::Vector2D& point) {
        return rect.contains(point.x, point.y);
    }
    
    bool rectIntersects(const GUI::Rect& a, const GUI::Rect& b) {
        return !(a.x + a.width < b.x || b.x + b.width < a.x ||
                 a.y + a.height < b.y || b.y + b.height < a.y);
    }
    
    void drawWireRect(const GUI::Rect& rect, const Graphics::Color& color) {
        // Mock wire rectangle drawing
        std::cout << "Drawing wire rect: " << rect.x << "," << rect.y 
                  << " " << rect.width << "x" << rect.height << std::endl;
    }
    
    void drawFilledRect(const GUI::Rect& rect, const Graphics::Color& color) {
        // Mock filled rectangle drawing
        std::cout << "Drawing filled rect: " << rect.x << "," << rect.y 
                  << " " << rect.width << "x" << rect.height << std::endl;
    }
    
    void drawLine(const Math::Vector2D& start, const Math::Vector2D& end, const Graphics::Color& color) {
        // Mock line drawing
        std::cout << "Drawing line: (" << start.x << "," << start.y 
                  << ") to (" << end.x << "," << end.y << ")" << std::endl;
    }
    
    void drawCircle(const Math::Vector2D& center, float radius, const Graphics::Color& color) {
        // Mock circle drawing
        std::cout << "Drawing circle: center(" << center.x << "," << center.y 
                  << ") radius(" << radius << ")" << std::endl;
    }
    
    void drawArrow(const Math::Vector2D& start, const Math::Vector2D& end, const Graphics::Color& color) {
        // Mock arrow drawing
        std::cout << "Drawing arrow: (" << start.x << "," << start.y 
                  << ") to (" << end.x << "," << end.y << ")" << std::endl;
    }
}

} // namespace Editor
} // namespace JJM