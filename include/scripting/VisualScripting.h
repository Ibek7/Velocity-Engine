#ifndef VISUAL_SCRIPTING_H
#define VISUAL_SCRIPTING_H

#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <functional>

namespace JJM {
namespace Scripting {

enum class PinType {
    EXEC,    // Execution flow
    FLOAT,
    INT,
    BOOL,
    STRING,
    OBJECT,
    VECTOR3
};

struct Pin {
    std::string name;
    PinType type;
    bool isInput;
    int index;
};

class VisualScriptNode {
public:
    VisualScriptNode(int id, const std::string& type);
    virtual ~VisualScriptNode() = default;
    
    int getId() const { return m_id; }
    const std::string& getType() const { return m_type; }
    const std::vector<Pin>& getInputs() const { return m_inputs; }
    const std::vector<Pin>& getOutputs() const { return m_outputs; }
    
    virtual void execute() = 0;
    
protected:
    void addInput(const std::string& name, PinType type);
    void addOutput(const std::string& name, PinType type);
    
    int m_id;
    std::string m_type;
    std::vector<Pin> m_inputs;
    std::vector<Pin> m_outputs;
};

struct NodeConnection {
    int sourceNode;
    int sourcePin;
    int targetNode;
    int targetPin;
};

class VisualScript {
public:
    VisualScript();
    ~VisualScript();
    
    int addNode(const std::string& type);
    void removeNode(int nodeId);
    bool connectNodes(int sourceNode, int sourcePin, int targetNode, int targetPin);
    
    void execute();
    
    bool compile();
    
private:
    std::vector<std::unique_ptr<VisualScriptNode>> m_nodes;
    std::vector<NodeConnection> m_connections;
    int m_nextNodeId;
};

class VisualScriptingSystem {
public:
    void registerScript(const std::string& name, VisualScript* script);
    VisualScript* getScript(const std::string& name);
    void executeScript(const std::string& name);
    
private:
    std::unordered_map<std::string, VisualScript*> m_scripts;
};

} // namespace Scripting
} // namespace JJM

#endif
