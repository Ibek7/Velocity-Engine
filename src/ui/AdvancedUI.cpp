#include "ui/AdvancedUI.h"
#include <iostream>

namespace JJM {
namespace UI {

// TreeView implementation
TreeView::TreeView() : root(std::make_unique<TreeNode>("Root")) {
    root->expanded = true;
}

TreeNode* TreeView::addNode(const std::string& label, TreeNode* parent) {
    auto node = std::make_unique<TreeNode>(label);
    TreeNode* nodePtr = node.get();
    
    if (parent) {
        parent->children.push_back(std::move(node));
    } else {
        root->children.push_back(std::move(node));
    }
    
    return nodePtr;
}

void TreeView::removeNode(TreeNode* node) {
    // Simplified - would need recursive search
    (void)node;
}

void TreeView::clear() {
    root->children.clear();
}

void TreeView::render() {
    for (auto& child : root->children) {
        renderNode(child.get(), 0);
    }
}

void TreeView::renderNode(TreeNode* node, int depth) {
    std::cout << std::string(depth * 2, ' ') << (node->expanded ? "[-]" : "[+]") << " " << node->label << std::endl;
    
    if (node->expanded) {
        for (auto& child : node->children) {
            renderNode(child.get(), depth + 1);
        }
    }
}

// DataGrid implementation
DataGrid::DataGrid() : selectable(true), selectedRow(-1), sortColumn(-1), sortAscending(true) {
}

void DataGrid::addColumn(const std::string& header, float width) {
    columns.push_back({header, width, true, true});
}

void DataGrid::addRow(const std::vector<std::string>& cells) {
    rows.push_back(cells);
}

void DataGrid::clear() {
    rows.clear();
}

void DataGrid::render() {
    // Render header
    std::cout << "Grid:" << std::endl;
    for (const auto& col : columns) {
        std::cout << col.header << "\t";
    }
    std::cout << std::endl;
    
    // Render rows
    for (size_t i = 0; i < rows.size(); ++i) {
        if (selectable && static_cast<int>(i) == selectedRow) {
            std::cout << "> ";
        } else {
            std::cout << "  ";
        }
        
        for (const auto& cell : rows[i]) {
            std::cout << cell << "\t";
        }
        std::cout << std::endl;
    }
}

// TabContainer implementation
TabContainer::TabContainer() : activeTab(0) {
}

void TabContainer::addTab(const std::string& label, std::function<void()> renderCallback, bool closeable) {
    tabs.push_back({label, renderCallback, closeable});
}

void TabContainer::removeTab(int index) {
    if (index >= 0 && index < static_cast<int>(tabs.size())) {
        tabs.erase(tabs.begin() + index);
        if (activeTab >= static_cast<int>(tabs.size())) {
            activeTab = static_cast<int>(tabs.size()) - 1;
        }
    }
}

void TabContainer::setActiveTab(int index) {
    if (index >= 0 && index < static_cast<int>(tabs.size())) {
        activeTab = index;
    }
}

void TabContainer::render() {
    // Render tab buttons
    std::cout << "Tabs: ";
    for (size_t i = 0; i < tabs.size(); ++i) {
        if (static_cast<int>(i) == activeTab) {
            std::cout << "[" << tabs[i].label << "] ";
        } else {
            std::cout << " " << tabs[i].label << "  ";
        }
    }
    std::cout << std::endl;
    
    // Render active tab content
    if (activeTab >= 0 && activeTab < static_cast<int>(tabs.size())) {
        if (tabs[activeTab].renderCallback) {
            tabs[activeTab].renderCallback();
        }
    }
}

// MenuBar implementation
MenuBar::MenuBar() {
}

MenuBar::MenuItem& MenuBar::addMenu(const std::string& label) {
    menus.push_back(MenuItem(label));
    return menus.back();
}

void MenuBar::render() {
    std::cout << "Menu: ";
    for (auto& menu : menus) {
        std::cout << menu.label << " | ";
    }
    std::cout << std::endl;
}

void MenuBar::renderMenuItem(MenuItem& item) {
    std::cout << item.label;
    if (!item.shortcut.empty()) {
        std::cout << " (" << item.shortcut << ")";
    }
    std::cout << std::endl;
}

// Modal implementation
Modal::Modal(const std::string& title, int width, int height)
    : title(title), width(width), height(height), isOpen(false) {
}

void Modal::addButton(const std::string& label, std::function<void()> callback) {
    buttons.push_back({label, callback});
}

bool Modal::render() {
    if (!isOpen) return false;
    
    std::cout << "=== " << title << " ===" << std::endl;
    std::cout << "Size: " << width << "x" << height << std::endl;
    
    if (contentCallback) {
        contentCallback();
    }
    
    std::cout << "Buttons: ";
    for (const auto& btn : buttons) {
        std::cout << "[" << btn.label << "] ";
    }
    std::cout << std::endl;
    
    return isOpen;
}

// PropertyInspector implementation
PropertyInspector::PropertyInspector() : readOnly(false) {
}

void PropertyInspector::beginProperties() {
    std::cout << "--- Properties ---" << std::endl;
}

void PropertyInspector::endProperties() {
    std::cout << "--- End ---" << std::endl;
}

bool PropertyInspector::propertyFloat(const std::string& name, float* value, float min, float max) {
    std::cout << name << ": " << *value << " [" << min << " - " << max << "]" << std::endl;
    return !readOnly;
}

bool PropertyInspector::propertyInt(const std::string& name, int* value, int min, int max) {
    std::cout << name << ": " << *value << " [" << min << " - " << max << "]" << std::endl;
    return !readOnly;
}

bool PropertyInspector::propertyBool(const std::string& name, bool* value) {
    std::cout << name << ": " << (*value ? "true" : "false") << std::endl;
    return !readOnly;
}

bool PropertyInspector::propertyString(const std::string& name, std::string* value) {
    std::cout << name << ": " << *value << std::endl;
    return !readOnly;
}

bool PropertyInspector::propertyColor(const std::string& name, float* color) {
    std::cout << name << ": RGB(" << color[0] << ", " << color[1] << ", " << color[2] << ")" << std::endl;
    return !readOnly;
}

} // namespace UI
} // namespace JJM
