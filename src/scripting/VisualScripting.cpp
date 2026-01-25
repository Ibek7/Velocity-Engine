#include "scripting/VisualScripting.h"

namespace JJM {
namespace Scripting {

VisualScriptNode::VisualScriptNode(int id, const std::string& type)
    : m_id(id), m_type(type) {
}

void VisualScriptNode::addInput(const std::string& name, PinType type) {
    Pin pin;
    pin.name = name;
    pin.type = type;
    pin.isInput = true;
    pin.index = static_cast<int>(m_inputs.size());
    m_inputs.push_back(pin);
}

void VisualScriptNode::addOutput(const std::string& name, PinType type) {
    Pin pin;
    pin.name = name;
    pin.type = type;
    pin.isInput = false;
    pin.index = static_cast<int>(m_outputs.size());
    m_outputs.push_back(pin);
}

VisualScript::VisualScript() : m_nextNodeId(1) {
}

VisualScript::~VisualScript() {
}

int VisualScript::addNode(const std::string& type) {
    int id = m_nextNodeId++;
    // m_nodes.push_back(NodeFactory::create(id, type));
    return id;
}

void VisualScript::removeNode(int nodeId) {
    m_nodes.erase(
        std::remove_if(m_nodes.begin(), m_nodes.end(),
                      [nodeId](const std::unique_ptr<VisualScriptNode>& node) {
                          return node->getId() == nodeId;
                      }),
        m_nodes.end()
    );
}

bool VisualScript::connectNodes(int sourceNode, int sourcePin,
                               int targetNode, int targetPin) {
    NodeConnection conn;
    conn.sourceNode = sourceNode;
    conn.sourcePin = sourcePin;
    conn.targetNode = targetNode;
    conn.targetPin = targetPin;
    m_connections.push_back(conn);
    return true;
}

void VisualScript::execute() {
    // Execute nodes in order
    for (const auto& node : m_nodes) {
        node->execute();
    }
}

bool VisualScript::compile() {
    // Compile graph to executable form
    return true;
}

void VisualScriptingSystem::registerScript(const std::string& name, VisualScript* script) {
    m_scripts[name] = script;
}

VisualScript* VisualScriptingSystem::getScript(const std::string& name) {
    auto it = m_scripts.find(name);
    return it != m_scripts.end() ? it->second : nullptr;
}

void VisualScriptingSystem::executeScript(const std::string& name) {
    VisualScript* script = getScript(name);
    if (script) {
        script->execute();
    }
}

} // namespace Scripting
} // namespace JJM
