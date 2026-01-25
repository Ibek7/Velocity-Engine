#include "graphics/ShaderGraph.h"
#include "graphics/Shader.h"
#include "graphics/Material.h"
#include <algorithm>
#include <sstream>
#include <stack>
#include <fstream>

namespace JJM {
namespace Graphics {

// ShaderNode implementation
ShaderNode::ShaderNode(int id, NodeType type)
    : m_id(id), m_type(type), m_posX(0), m_posY(0) {
}

void ShaderNode::addInput(const std::string& name, DataType type) {
    m_inputs.emplace_back(name, type, true, static_cast<int>(m_inputs.size()));
}

void ShaderNode::addOutput(const std::string& name, DataType type) {
    m_outputs.emplace_back(name, type, false, static_cast<int>(m_outputs.size()));
}

NodePin* ShaderNode::getInput(int index) {
    if (index >= 0 && index < static_cast<int>(m_inputs.size())) {
        return &m_inputs[index];
    }
    return nullptr;
}

NodePin* ShaderNode::getOutput(int index) {
    if (index >= 0 && index < static_cast<int>(m_outputs.size())) {
        return &m_outputs[index];
    }
    return nullptr;
}

// MasterNode implementation
MasterNode::MasterNode(int id) : ShaderNode(id, NodeType::MASTER_NODE) {
    addInput("Albedo", DataType::VEC3);
    addInput("Metallic", DataType::FLOAT);
    addInput("Roughness", DataType::FLOAT);
    addInput("Normal", DataType::VEC3);
    addInput("Emission", DataType::VEC3);
    addInput("AO", DataType::FLOAT);
    addInput("Alpha", DataType::FLOAT);
}

std::string MasterNode::generateCode(const std::string& outputVar,
                                    const std::vector<std::string>& inputVars) const {
    std::ostringstream code;
    
    // Output assignments for master node
    if (inputVars.size() >= 7) {
        code << "    vec3 albedo = " << inputVars[0] << ";\n";
        code << "    float metallic = " << inputVars[1] << ";\n";
        code << "    float roughness = " << inputVars[2] << ";\n";
        code << "    vec3 normal = " << inputVars[3] << ";\n";
        code << "    vec3 emission = " << inputVars[4] << ";\n";
        code << "    float ao = " << inputVars[5] << ";\n";
        code << "    float alpha = " << inputVars[6] << ";\n";
        code << "    \n";
        code << "    // Final output\n";
        code << "    fragColor = vec4(albedo, alpha);\n";
        code << "    // Additional outputs for deferred rendering can be added here\n";
    }
    
    return code.str();
}

// TextureSampleNode implementation
TextureSampleNode::TextureSampleNode(int id) : ShaderNode(id, NodeType::SAMPLE_TEXTURE_2D) {
    addInput("UV", DataType::VEC2);
    addOutput("RGBA", DataType::VEC4);
    addOutput("RGB", DataType::VEC3);
    addOutput("R", DataType::FLOAT);
    addOutput("G", DataType::FLOAT);
    addOutput("B", DataType::FLOAT);
    addOutput("A", DataType::FLOAT);
}

std::string TextureSampleNode::generateCode(const std::string& outputVar,
                                           const std::vector<std::string>& inputVars) const {
    std::ostringstream code;
    std::string texName = m_textureName.empty() ? "mainTexture" : m_textureName;
    std::string uv = inputVars.empty() ? "texCoord" : inputVars[0];
    
    code << "    vec4 " << outputVar << "_rgba = texture(" << texName << ", " << uv << ");\n";
    code << "    vec3 " << outputVar << "_rgb = " << outputVar << "_rgba.rgb;\n";
    code << "    float " << outputVar << "_r = " << outputVar << "_rgba.r;\n";
    code << "    float " << outputVar << "_g = " << outputVar << "_rgba.g;\n";
    code << "    float " << outputVar << "_b = " << outputVar << "_rgba.b;\n";
    code << "    float " << outputVar << "_a = " << outputVar << "_rgba.a;\n";
    
    return code.str();
}

// MathNode implementation
MathNode::MathNode(int id, NodeType mathOp) : ShaderNode(id, mathOp) {
    switch (mathOp) {
        case NodeType::ADD:
        case NodeType::SUBTRACT:
        case NodeType::MULTIPLY:
        case NodeType::DIVIDE:
        case NodeType::DOT:
        case NodeType::CROSS:
        case NodeType::DISTANCE:
            addInput("A", DataType::FLOAT);
            addInput("B", DataType::FLOAT);
            addOutput("Result", DataType::FLOAT);
            break;
        case NodeType::LERP:
        case NodeType::SMOOTHSTEP:
        case NodeType::CLAMP:
            addInput("A", DataType::FLOAT);
            addInput("B", DataType::FLOAT);
            addInput("T", DataType::FLOAT);
            addOutput("Result", DataType::FLOAT);
            break;
        case NodeType::NORMALIZE:
        case NodeType::LENGTH:
        case NodeType::ABS:
        case NodeType::SQRT:
            addInput("Value", DataType::FLOAT);
            addOutput("Result", DataType::FLOAT);
            break;
        default:
            addInput("Value", DataType::FLOAT);
            addOutput("Result", DataType::FLOAT);
            break;
    }
}

std::string MathNode::generateCode(const std::string& outputVar,
                                  const std::vector<std::string>& inputVars) const {
    std::ostringstream code;
    std::string result = outputVar + "_result";
    
    if (inputVars.size() < 1) return "";
    
    switch (m_type) {
        case NodeType::ADD:
            if (inputVars.size() >= 2)
                code << "    auto " << result << " = " << inputVars[0] << " + " << inputVars[1] << ";\n";
            break;
        case NodeType::SUBTRACT:
            if (inputVars.size() >= 2)
                code << "    auto " << result << " = " << inputVars[0] << " - " << inputVars[1] << ";\n";
            break;
        case NodeType::MULTIPLY:
            if (inputVars.size() >= 2)
                code << "    auto " << result << " = " << inputVars[0] << " * " << inputVars[1] << ";\n";
            break;
        case NodeType::DIVIDE:
            if (inputVars.size() >= 2)
                code << "    auto " << result << " = " << inputVars[0] << " / " << inputVars[1] << ";\n";
            break;
        case NodeType::DOT:
            if (inputVars.size() >= 2)
                code << "    float " << result << " = dot(" << inputVars[0] << ", " << inputVars[1] << ");\n";
            break;
        case NodeType::CROSS:
            if (inputVars.size() >= 2)
                code << "    vec3 " << result << " = cross(" << inputVars[0] << ", " << inputVars[1] << ");\n";
            break;
        case NodeType::NORMALIZE:
            code << "    auto " << result << " = normalize(" << inputVars[0] << ");\n";
            break;
        case NodeType::LENGTH:
            code << "    float " << result << " = length(" << inputVars[0] << ");\n";
            break;
        case NodeType::LERP:
            if (inputVars.size() >= 3)
                code << "    auto " << result << " = mix(" << inputVars[0] << ", " << inputVars[1] << ", " << inputVars[2] << ");\n";
            break;
        case NodeType::SMOOTHSTEP:
            if (inputVars.size() >= 3)
                code << "    auto " << result << " = smoothstep(" << inputVars[0] << ", " << inputVars[1] << ", " << inputVars[2] << ");\n";
            break;
        case NodeType::CLAMP:
            if (inputVars.size() >= 3)
                code << "    auto " << result << " = clamp(" << inputVars[0] << ", " << inputVars[1] << ", " << inputVars[2] << ");\n";
            break;
        case NodeType::ABS:
            code << "    auto " << result << " = abs(" << inputVars[0] << ");\n";
            break;
        case NodeType::SQRT:
            code << "    auto " << result << " = sqrt(" << inputVars[0] << ");\n";
            break;
        default:
            code << "    auto " << result << " = " << inputVars[0] << ";\n";
            break;
    }
    
    return code.str();
}

// NoiseNode implementation
NoiseNode::NoiseNode(int id, NodeType noiseType) : ShaderNode(id, noiseType) {
    addInput("Position", DataType::VEC3);
    addInput("Scale", DataType::FLOAT);
    addOutput("Value", DataType::FLOAT);
}

std::string NoiseNode::generateCode(const std::string& outputVar,
                                   const std::vector<std::string>& inputVars) const {
    std::ostringstream code;
    std::string pos = inputVars.empty() ? "vec3(0.0)" : inputVars[0];
    std::string scale = (inputVars.size() > 1) ? inputVars[1] : std::to_string(m_scale);
    
    code << "    // Noise function (simplified - would use actual noise implementation)\n";
    code << "    float " << outputVar << "_value = fract(sin(dot(" << pos << " * " << scale;
    code << ", vec3(12.9898, 78.233, 45.164))) * 43758.5453);\n";
    
    return code.str();
}

// FresnelNode implementation
FresnelNode::FresnelNode(int id) : ShaderNode(id, NodeType::FRESNEL) {
    addInput("Normal", DataType::VEC3);
    addInput("ViewDir", DataType::VEC3);
    addInput("Power", DataType::FLOAT);
    addOutput("Fresnel", DataType::FLOAT);
}

std::string FresnelNode::generateCode(const std::string& outputVar,
                                     const std::vector<std::string>& inputVars) const {
    std::ostringstream code;
    std::string normal = inputVars.empty() ? "normal" : inputVars[0];
    std::string viewDir = (inputVars.size() > 1) ? inputVars[1] : "viewDir";
    std::string power = (inputVars.size() > 2) ? inputVars[2] : std::to_string(m_power);
    
    code << "    float " << outputVar << "_fresnel = pow(1.0 - max(0.0, dot(" << normal;
    code << ", " << viewDir << ")), " << power << ");\n";
    
    return code.str();
}

// ShaderGraph implementation
ShaderGraph::ShaderGraph()
    : m_nextNodeId(1), m_masterNodeId(-1), m_previewNodeId(-1), m_compiledShader(nullptr) {
    // Create master node
    m_masterNodeId = addNode(NodeType::MASTER_NODE);
}

ShaderGraph::~ShaderGraph() {
    if (m_compiledShader) {
        delete m_compiledShader;
    }
}

int ShaderGraph::addNode(NodeType type) {
    int nodeId = m_nextNodeId++;
    m_nodes.push_back(createNode(nodeId, type));
    return nodeId;
}

void ShaderGraph::removeNode(int nodeId) {
    // Remove connections
    disconnectAllFromNode(nodeId);
    
    // Remove node
    m_nodes.erase(
        std::remove_if(m_nodes.begin(), m_nodes.end(),
                      [nodeId](const std::unique_ptr<ShaderNode>& node) {
                          return node->getId() == nodeId;
                      }),
        m_nodes.end()
    );
}

ShaderNode* ShaderGraph::getNode(int nodeId) {
    for (auto& node : m_nodes) {
        if (node->getId() == nodeId) {
            return node.get();
        }
    }
    return nullptr;
}

bool ShaderGraph::connectNodes(int sourceNodeId, int sourcePinIndex,
                              int targetNodeId, int targetPinIndex) {
    ShaderNode* sourceNode = getNode(sourceNodeId);
    ShaderNode* targetNode = getNode(targetNodeId);
    
    if (!sourceNode || !targetNode) return false;
    
    // Validate connection
    NodePin* sourcePin = sourceNode->getOutput(sourcePinIndex);
    NodePin* targetPin = targetNode->getInput(targetPinIndex);
    
    if (!sourcePin || !targetPin) return false;
    if (sourcePin->type != targetPin->type) return false;  // Type mismatch
    
    // Remove existing connection to target pin
    disconnectNodes(targetNodeId, targetPinIndex);
    
    // Add new connection
    m_connections.emplace_back(sourceNodeId, sourcePinIndex, targetNodeId, targetPinIndex);
    
    return true;
}

void ShaderGraph::disconnectNodes(int targetNodeId, int targetPinIndex) {
    m_connections.erase(
        std::remove_if(m_connections.begin(), m_connections.end(),
                      [targetNodeId, targetPinIndex](const NodeConnection& conn) {
                          return conn.targetNodeId == targetNodeId &&
                                 conn.targetPinIndex == targetPinIndex;
                      }),
        m_connections.end()
    );
}

void ShaderGraph::disconnectAllFromNode(int nodeId) {
    m_connections.erase(
        std::remove_if(m_connections.begin(), m_connections.end(),
                      [nodeId](const NodeConnection& conn) {
                          return conn.sourceNodeId == nodeId || conn.targetNodeId == nodeId;
                      }),
        m_connections.end()
    );
}

bool ShaderGraph::compile() {
    // Validate graph
    std::string errorMsg;
    if (!validate(errorMsg)) {
        // Log error
        return false;
    }
    
    // Generate shader code
    std::ostringstream vertexShader, fragmentShader;
    
    // Vertex shader template
    vertexShader << "#version 330 core\n";
    vertexShader << "layout(location = 0) in vec3 position;\n";
    vertexShader << "layout(location = 1) in vec3 normal;\n";
    vertexShader << "layout(location = 2) in vec2 texCoord;\n";
    vertexShader << "\n";
    vertexShader << "out vec3 fragPosition;\n";
    vertexShader << "out vec3 fragNormal;\n";
    vertexShader << "out vec2 fragTexCoord;\n";
    vertexShader << "\n";
    vertexShader << "uniform mat4 model;\n";
    vertexShader << "uniform mat4 view;\n";
    vertexShader << "uniform mat4 projection;\n";
    vertexShader << "\n";
    vertexShader << "void main() {\n";
    vertexShader << "    fragPosition = vec3(model * vec4(position, 1.0));\n";
    vertexShader << "    fragNormal = mat3(transpose(inverse(model))) * normal;\n";
    vertexShader << "    fragTexCoord = texCoord;\n";
    vertexShader << "    gl_Position = projection * view * vec4(fragPosition, 1.0);\n";
    vertexShader << "}\n";
    
    // Fragment shader
    fragmentShader << "#version 330 core\n";
    fragmentShader << "\n";
    fragmentShader << "in vec3 fragPosition;\n";
    fragmentShader << "in vec3 fragNormal;\n";
    fragmentShader << "in vec2 fragTexCoord;\n";
    fragmentShader << "\n";
    fragmentShader << "out vec4 fragColor;\n";
    fragmentShader << "\n";
    fragmentShader << "uniform vec3 cameraPosition;\n";
    fragmentShader << "uniform float time;\n";
    fragmentShader << "\n";
    fragmentShader << "void main() {\n";
    
    // Generate code for each node in topological order
    std::vector<ShaderNode*> sortedNodes;
    topologicalSort(sortedNodes);
    
    std::unordered_map<int, std::string> nodeOutputVars;
    int tempVarCounter = 0;
    
    for (ShaderNode* node : sortedNodes) {
        std::string code = generateNodeCode(node, nodeOutputVars, tempVarCounter);
        fragmentShader << code;
    }
    
    fragmentShader << "}\n";
    
    m_vertexShaderCode = vertexShader.str();
    m_fragmentShaderCode = fragmentShader.str();
    
    // Compile actual shader
    if (m_compiledShader) {
        delete m_compiledShader;
    }
    // m_compiledShader = new Shader(m_vertexShaderCode, m_fragmentShaderCode);
    // TODO: Implement actual shader compilation
    
    return true;
}

std::string ShaderGraph::generateNodeCode(ShaderNode* node,
                                         std::unordered_map<int, std::string>& nodeOutputVars,
                                         int& tempVarCounter) {
    if (!node) return "";
    
    // Get input variable names from connected nodes
    std::vector<std::string> inputVars;
    for (size_t i = 0; i < node->getInputs().size(); ++i) {
        // Find connection to this input
        bool found = false;
        for (const auto& conn : m_connections) {
            if (conn.targetNodeId == node->getId() && conn.targetPinIndex == static_cast<int>(i)) {
                // Use output variable from source node
                auto it = nodeOutputVars.find(conn.sourceNodeId);
                if (it != nodeOutputVars.end()) {
                    inputVars.push_back(it->second + "_out" + std::to_string(conn.sourcePinIndex));
                }
                found = true;
                break;
            }
        }
        
        if (!found) {
            // Use default value
            const NodePin& pin = node->getInputs()[i];
            inputVars.push_back(getDefaultValue(pin.type));
        }
    }
    
    // Generate unique output variable name
    std::string outputVar = "node" + std::to_string(node->getId());
    nodeOutputVars[node->getId()] = outputVar;
    
    // Generate node code
    return node->generateCode(outputVar, inputVars);
}

std::string ShaderGraph::getDataTypeString(DataType type) const {
    switch (type) {
        case DataType::FLOAT: return "float";
        case DataType::VEC2: return "vec2";
        case DataType::VEC3: return "vec3";
        case DataType::VEC4: return "vec4";
        case DataType::MAT3: return "mat3";
        case DataType::MAT4: return "mat4";
        case DataType::SAMPLER_2D: return "sampler2D";
        case DataType::SAMPLER_CUBE: return "samplerCube";
        case DataType::BOOL: return "bool";
        case DataType::INT: return "int";
        default: return "float";
    }
}

std::string ShaderGraph::getDefaultValue(DataType type) const {
    switch (type) {
        case DataType::FLOAT: return "0.0";
        case DataType::VEC2: return "vec2(0.0)";
        case DataType::VEC3: return "vec3(0.0)";
        case DataType::VEC4: return "vec4(0.0)";
        case DataType::BOOL: return "false";
        case DataType::INT: return "0";
        default: return "0.0";
    }
}

void ShaderGraph::topologicalSort(std::vector<ShaderNode*>& sortedNodes) {
    // Simple topological sort using DFS
    std::unordered_map<int, bool> visited;
    std::unordered_map<int, bool> inStack;
    
    std::function<void(ShaderNode*)> dfs = [&](ShaderNode* node) {
        if (!node || visited[node->getId()]) return;
        
        visited[node->getId()] = true;
        inStack[node->getId()] = true;
        
        // Visit all nodes that feed into this one
        for (const auto& conn : m_connections) {
            if (conn.targetNodeId == node->getId()) {
                ShaderNode* sourceNode = getNode(conn.sourceNodeId);
                if (sourceNode && !inStack[sourceNode->getId()]) {
                    dfs(sourceNode);
                }
            }
        }
        
        inStack[node->getId()] = false;
        sortedNodes.push_back(node);
    };
    
    // Start from master node
    ShaderNode* masterNode = getNode(m_masterNodeId);
    if (masterNode) {
        dfs(masterNode);
    }
}

bool ShaderGraph::hasCircularDependency() const {
    // Check for cycles in the graph
    std::unordered_map<int, bool> visited;
    std::unordered_map<int, bool> inStack;
    
    std::function<bool(int)> hasCycle = [&](int nodeId) -> bool {
        if (inStack[nodeId]) return true;
        if (visited[nodeId]) return false;
        
        visited[nodeId] = true;
        inStack[nodeId] = true;
        
        for (const auto& conn : m_connections) {
            if (conn.sourceNodeId == nodeId) {
                if (hasCycle(conn.targetNodeId)) {
                    return true;
                }
            }
        }
        
        inStack[nodeId] = false;
        return false;
    };
    
    for (const auto& node : m_nodes) {
        if (hasCycle(node->getId())) {
            return true;
        }
    }
    
    return false;
}

bool ShaderGraph::validate(std::string& errorMessage) const {
    // Check for circular dependencies
    if (hasCircularDependency()) {
        errorMessage = "Graph contains circular dependencies";
        return false;
    }
    
    // Check if master node exists and has connections
    if (m_masterNodeId < 0) {
        errorMessage = "No master node found";
        return false;
    }
    
    return true;
}

bool ShaderGraph::saveToFile(const std::string& filename) const {
    std::ofstream file(filename);
    if (!file.is_open()) return false;
    
    // TODO: Implement JSON serialization
    // Serialize nodes, connections, and properties
    
    file.close();
    return true;
}

bool ShaderGraph::loadFromFile(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) return false;
    
    // TODO: Implement JSON deserialization
    
    file.close();
    return true;
}

std::unique_ptr<ShaderNode> ShaderGraph::createNode(int id, NodeType type) {
    switch (type) {
        case NodeType::MASTER_NODE:
            return std::make_unique<MasterNode>(id);
        case NodeType::SAMPLE_TEXTURE_2D:
            return std::make_unique<TextureSampleNode>(id);
        case NodeType::FRESNEL:
            return std::make_unique<FresnelNode>(id);
        case NodeType::PERLIN_NOISE:
        case NodeType::SIMPLEX_NOISE:
        case NodeType::VORONOI_NOISE:
            return std::make_unique<NoiseNode>(id, type);
        case NodeType::ADD:
        case NodeType::MULTIPLY:
        case NodeType::DOT:
        case NodeType::LERP:
            return std::make_unique<MathNode>(id, type);
        default:
            return std::make_unique<MathNode>(id, type);
    }
}

// ShaderGraphEditor implementation
ShaderGraphEditor::ShaderGraphEditor()
    : m_graph(nullptr), m_isDragging(false), m_dragNodeId(-1),
      m_isConnecting(false), m_viewX(0), m_viewY(0), m_zoom(1.0f) {
}

ShaderGraphEditor::~ShaderGraphEditor() {
}

void ShaderGraphEditor::update(float deltaTime) {
    // Update logic
}

void ShaderGraphEditor::render() {
    // Render graph visualization
    if (!m_graph) return;
    
    // Render connections
    for (const auto& conn : m_graph->getConnections()) {
        renderConnection(conn);
    }
    
    // Render nodes
    for (const auto& node : m_graph->getAllNodes()) {
        renderNode(node.get());
    }
}

void ShaderGraphEditor::selectNode(int nodeId) {
    if (std::find(m_selectedNodes.begin(), m_selectedNodes.end(), nodeId) == m_selectedNodes.end()) {
        m_selectedNodes.push_back(nodeId);
    }
}

void ShaderGraphEditor::deselectAll() {
    m_selectedNodes.clear();
}

bool ShaderGraphEditor::isNodeSelected(int nodeId) const {
    return std::find(m_selectedNodes.begin(), m_selectedNodes.end(), nodeId) != m_selectedNodes.end();
}

void ShaderGraphEditor::startDraggingNode(int nodeId, float mouseX, float mouseY) {
    m_isDragging = true;
    m_dragNodeId = nodeId;
    m_dragStartX = mouseX;
    m_dragStartY = mouseY;
}

void ShaderGraphEditor::dragNode(float mouseX, float mouseY) {
    if (!m_isDragging || !m_graph) return;
    
    ShaderNode* node = m_graph->getNode(m_dragNodeId);
    if (node) {
        float graphX, graphY;
        screenToGraph(mouseX, mouseY, graphX, graphY);
        node->setPosition(graphX, graphY);
    }
}

void ShaderGraphEditor::stopDragging() {
    m_isDragging = false;
    m_dragNodeId = -1;
}

void ShaderGraphEditor::startConnection(int nodeId, int pinIndex, bool isInput) {
    m_isConnecting = true;
    m_connectionSourceNode = nodeId;
    m_connectionSourcePin = pinIndex;
    m_connectionSourceIsInput = isInput;
}

void ShaderGraphEditor::updateConnection(float mouseX, float mouseY) {
    m_connectionEndX = mouseX;
    m_connectionEndY = mouseY;
}

void ShaderGraphEditor::endConnection(int nodeId, int pinIndex, bool isInput) {
    if (!m_isConnecting || !m_graph) return;
    
    // Connect nodes based on direction
    if (m_connectionSourceIsInput && !isInput) {
        // Source is input, target is output
        m_graph->connectNodes(nodeId, pinIndex, m_connectionSourceNode, m_connectionSourcePin);
    } else if (!m_connectionSourceIsInput && isInput) {
        // Source is output, target is input
        m_graph->connectNodes(m_connectionSourceNode, m_connectionSourcePin, nodeId, pinIndex);
    }
    
    cancelConnection();
}

void ShaderGraphEditor::cancelConnection() {
    m_isConnecting = false;
}

void ShaderGraphEditor::undo() {
    if (m_undoStack.empty()) return;
    
    EditorAction action = m_undoStack.back();
    m_undoStack.pop_back();
    
    // Apply inverse action
    // ...
    
    m_redoStack.push_back(action);
}

void ShaderGraphEditor::redo() {
    if (m_redoStack.empty()) return;
    
    EditorAction action = m_redoStack.back();
    m_redoStack.pop_back();
    
    // Apply action
    // ...
    
    m_undoStack.push_back(action);
}

bool ShaderGraphEditor::canUndo() const {
    return !m_undoStack.empty();
}

bool ShaderGraphEditor::canRedo() const {
    return !m_redoStack.empty();
}

void ShaderGraphEditor::screenToGraph(float screenX, float screenY, float& graphX, float& graphY) const {
    graphX = (screenX - m_viewX) / m_zoom;
    graphY = (screenY - m_viewY) / m_zoom;
}

void ShaderGraphEditor::graphToScreen(float graphX, float graphY, float& screenX, float& screenY) const {
    screenX = graphX * m_zoom + m_viewX;
    screenY = graphY * m_zoom + m_viewY;
}

void ShaderGraphEditor::pushAction(const EditorAction& action) {
    m_undoStack.push_back(action);
    m_redoStack.clear();
}

void ShaderGraphEditor::renderNode(ShaderNode* node) {
    // TODO: Implement node rendering
}

void ShaderGraphEditor::renderConnection(const NodeConnection& conn) {
    // TODO: Implement connection rendering
}

// MaterialGraph implementation
MaterialGraph::MaterialGraph() : m_shaderGraph(nullptr) {
}

MaterialGraph::~MaterialGraph() {
}

Material* MaterialGraph::compile() {
    if (!m_shaderGraph) return nullptr;
    
    if (!m_shaderGraph->compile()) {
        return nullptr;
    }
    
    // Create material from compiled shader
    // Material* material = new Material(m_shaderGraph->getCompiledShader());
    // TODO: Implement material creation
    
    return nullptr;
}

void MaterialGraph::loadPBRTemplate() {
    if (!m_shaderGraph) return;
    
    // Create PBR template nodes
    int albedoTex = m_shaderGraph->addNode(NodeType::SAMPLE_TEXTURE_2D);
    int normalTex = m_shaderGraph->addNode(NodeType::SAMPLE_TEXTURE_2D);
    int metallicRoughnessTex = m_shaderGraph->addNode(NodeType::SAMPLE_TEXTURE_2D);
    
    // Connect to master node
    int master = m_shaderGraph->getMasterNodeId();
    m_shaderGraph->connectNodes(albedoTex, 1, master, 0);  // Albedo RGB
    m_shaderGraph->connectNodes(normalTex, 1, master, 3);   // Normal
    m_shaderGraph->connectNodes(metallicRoughnessTex, 2, master, 1);  // Metallic
    m_shaderGraph->connectNodes(metallicRoughnessTex, 3, master, 2);  // Roughness
}

void MaterialGraph::loadUnlitTemplate() {
    if (!m_shaderGraph) return;
    
    // Simple unlit template
    int colorTex = m_shaderGraph->addNode(NodeType::SAMPLE_TEXTURE_2D);
    int master = m_shaderGraph->getMasterNodeId();
    m_shaderGraph->connectNodes(colorTex, 1, master, 0);  // Albedo
}

void MaterialGraph::loadTerrainTemplate() {
    if (!m_shaderGraph) return;
    
    // Terrain blending template with triplanar mapping
    // TODO: Implement terrain template
}

void MaterialGraph::exposeProperty(const std::string& name, int nodeId,
                                  const std::string& propertyName, DataType type) {
    ExposedProperty prop;
    prop.name = name;
    prop.nodeId = nodeId;
    prop.propertyName = propertyName;
    prop.type = type;
    m_properties.push_back(prop);
}

} // namespace Graphics
} // namespace JJM
