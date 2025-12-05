#ifndef ADVANCED_UI_H
#define ADVANCED_UI_H

#include <string>
#include <vector>
#include <functional>
#include <memory>

namespace JJM {
namespace UI {

// Tree view widget
struct TreeNode {
    std::string label;
    bool expanded;
    std::vector<std::unique_ptr<TreeNode>> children;
    void* userData;
    
    TreeNode(const std::string& label) : label(label), expanded(false), userData(nullptr) {}
};

class TreeView {
public:
    TreeView();
    
    void render();
    TreeNode* addNode(const std::string& label, TreeNode* parent = nullptr);
    void removeNode(TreeNode* node);
    void clear();
    
    void setOnNodeClicked(std::function<void(TreeNode*)> callback) { onNodeClicked = callback; }
    void setOnNodeExpanded(std::function<void(TreeNode*, bool)> callback) { onNodeExpanded = callback; }
    
private:
    std::unique_ptr<TreeNode> root;
    std::function<void(TreeNode*)> onNodeClicked;
    std::function<void(TreeNode*, bool)> onNodeExpanded;
    
    void renderNode(TreeNode* node, int depth);
};

// Grid/Table widget
class DataGrid {
public:
    struct Column {
        std::string header;
        float width;
        bool sortable;
        bool resizable;
    };
    
    DataGrid();
    
    void addColumn(const std::string& header, float width = 100.0f);
    void addRow(const std::vector<std::string>& cells);
    void clear();
    void render();
    
    void setSelectable(bool selectable) { this->selectable = selectable; }
    int getSelectedRow() const { return selectedRow; }
    
private:
    std::vector<Column> columns;
    std::vector<std::vector<std::string>> rows;
    bool selectable;
    int selectedRow;
    int sortColumn;
    bool sortAscending;
};

// Tab container
class TabContainer {
public:
    struct Tab {
        std::string label;
        std::function<void()> renderCallback;
        bool closeable;
    };
    
    TabContainer();
    
    void addTab(const std::string& label, std::function<void()> renderCallback, bool closeable = false);
    void removeTab(int index);
    void setActiveTab(int index);
    int getActiveTab() const { return activeTab; }
    
    void render();
    
private:
    std::vector<Tab> tabs;
    int activeTab;
};

// Menu bar
class MenuBar {
public:
    struct MenuItem {
        std::string label;
        std::function<void()> callback;
        std::string shortcut;
        bool enabled;
        std::vector<MenuItem> subItems;
        
        MenuItem(const std::string& label) : label(label), enabled(true) {}
    };
    
    MenuBar();
    
    MenuItem& addMenu(const std::string& label);
    void render();
    
private:
    std::vector<MenuItem> menus;
    void renderMenuItem(MenuItem& item);
};

// Modal dialog
class Modal {
public:
    Modal(const std::string& title, int width = 400, int height = 300);
    
    void setContent(std::function<void()> renderCallback) { contentCallback = renderCallback; }
    void addButton(const std::string& label, std::function<void()> callback);
    
    bool render(); // Returns true if still open
    void open() { isOpen = true; }
    void close() { isOpen = false; }
    bool isOpened() const { return isOpen; }
    
private:
    std::string title;
    int width, height;
    bool isOpen;
    std::function<void()> contentCallback;
    
    struct Button {
        std::string label;
        std::function<void()> callback;
    };
    std::vector<Button> buttons;
};

// Property inspector
class PropertyInspector {
public:
    PropertyInspector();
    
    void beginProperties();
    void endProperties();
    
    bool propertyFloat(const std::string& name, float* value, float min = 0, float max = 100);
    bool propertyInt(const std::string& name, int* value, int min = 0, int max = 100);
    bool propertyBool(const std::string& name, bool* value);
    bool propertyString(const std::string& name, std::string* value);
    bool propertyColor(const std::string& name, float* color);
    
    void setReadOnly(bool readOnly) { this->readOnly = readOnly; }
    
private:
    bool readOnly;
};

} // namespace UI
} // namespace JJM

#endif // ADVANCED_UI_H
