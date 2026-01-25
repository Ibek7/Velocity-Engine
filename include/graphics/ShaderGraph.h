#ifndef SHADER_GRAPH_H
#define SHADER_GRAPH_H

#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <functional>

namespace JJM {
namespace Graphics {

// Forward declarations
class Shader;
class Material;

// Shader graph node types
enum class NodeType {
    // Input nodes
    TIME,
    SCREEN_POSITION,
    WORLD_POSITION,
    NORMAL,
    TANGENT,
    UV,
    VERTEX_COLOR,
    CAMERA_POSITION,
    CAMERA_DIRECTION,
    
    // Texture nodes
    SAMPLE_TEXTURE_2D,
    SAMPLE_TEXTURE_CUBE,
    TEXTURE_COORDINATE,
    TRIPLANAR_MAPPING,
    
    // Math nodes
    ADD,
    SUBTRACT,
    MULTIPLY,
    DIVIDE,
    POWER,
    SQRT,
    ABS,
    CLAMP,
    LERP,
    SMOOTHSTEP,
    DOT,
    CROSS,
    NORMALIZE,
    LENGTH,
    DISTANCE,
    
    // Trigonometry
    SIN,
    COS,
    TAN,
    ASIN,
    ACOS,
    ATAN,
    ATAN2,
    
    // Logic nodes
    COMPARE,
    IF,
    SWITCH,
    BRANCH,
    
    // Color nodes
    RGB_TO_HSV,
    HSV_TO_RGB,
    COLOR_RAMP,
    CONTRAST,
    SATURATION,
    HUE_SHIFT,
    
    // PBR nodes
    FRESNEL,
    SPECULAR,
    METALLIC_ROUGHNESS,
    SUBSURFACE_SCATTERING,
    
    // Noise nodes
    PERLIN_NOISE,
    SIMPLEX_NOISE,
    VORONOI_NOISE,
    WHITE_NOISE,
    
    // Utility nodes
    SPLIT,
    COMBINE,
    REMAP,
    ONE_MINUS,
    NEGATE,
    
    // Output nodes
    MASTER_NODE,
    CUSTOM_OUTPUT
};

// Data types in shader graph
enum class DataType {
    FLOAT,
    VEC2,
    VEC3,
    VEC4,
    MAT3,
    MAT4,
    SAMPLER_2D,
    SAMPLER_CUBE,
    BOOL,
    INT
};

// Comparison operators for logic nodes
enum class CompareOp {
    EQUAL,
    NOT_EQUAL,
    LESS,
    LESS_OR_EQUAL,
    GREATER,
    GREATER_OR_EQUAL
};

// Pin connection point on a node
struct NodePin {
    std::string name;
    DataType type;
    bool isInput;
    int pinIndex;
    
    // Default value if not connected (for inputs)
    float defaultValue[4] = {0, 0, 0, 1};
    
    NodePin(const std::string& n, DataType t, bool input, int index)
        : name(n), type(t), isInput(input), pinIndex(index) {}
};

// Connection between two nodes
struct NodeConnection {
    int sourceNodeId;
    int sourcePinIndex;
    int targetNodeId;
    int targetPinIndex;
    
    NodeConnection(int srcNode, int srcPin, int tgtNode, int tgtPin)
        : sourceNodeId(srcNode), sourcePinIndex(srcPin),
          targetNodeId(tgtNode), targetPinIndex(tgtPin) {}
};

// Base shader graph node
class ShaderNode {
public:
    ShaderNode(int id, NodeType type);
    virtual ~ShaderNode() = default;
    
    int getId() const { return m_id; }
    NodeType getType() const { return m_type; }
    
    const std::vector<NodePin>& getInputs() const { return m_inputs; }
    const std::vector<NodePin>& getOutputs() const { return m_outputs; }
    
    NodePin* getInput(int index);
    NodePin* getOutput(int index);
    
    void setPosition(float x, float y) { m_posX = x; m_posY = y; }
    void getPosition(float& x, float& y) const { x = m_posX; y = m_posY; }
    
    // Generate GLSL code for this node
    virtual std::string generateCode(const std::string& outputVar,
                                    const std::vector<std::string>& inputVars) const = 0;
    
    // Node-specific properties
    virtual void setProperty(const std::string& name, const void* value) {}
    virtual void getProperty(const std::string& name, void* value) const {}
    
protected:
    int m_id;
    NodeType m_type;
    std::vector<NodePin> m_inputs;
    std::vector<NodePin> m_outputs;
    float m_posX, m_posY;
    
    void addInput(const std::string& name, DataType type);
    void addOutput(const std::string& name, DataType type);
};

// Specific node implementations
class MasterNode : public ShaderNode {
public:
    MasterNode(int id);
    std::string generateCode(const std::string& outputVar,
                           const std::vector<std::string>& inputVars) const override;
    
    // Master node has: albedo, metallic, roughness, normal, emission, AO, alpha
};

class TextureSampleNode : public ShaderNode {
public:
    TextureSampleNode(int id);
    std::string generateCode(const std::string& outputVar,
                           const std::vector<std::string>& inputVars) const override;
    
    void setTextureName(const std::string& name) { m_textureName = name; }
    std::string getTextureName() const { return m_textureName; }
    
private:
    std::string m_textureName;
};

class MathNode : public ShaderNode {
public:
    MathNode(int id, NodeType mathOp);
    std::string generateCode(const std::string& outputVar,
                           const std::vector<std::string>& inputVars) const override;
};

class NoiseNode : public ShaderNode {
public:
    NoiseNode(int id, NodeType noiseType);
    std::string generateCode(const std::string& outputVar,
                           const std::vector<std::string>& inputVars) const override;
    
    void setScale(float scale) { m_scale = scale; }
    void setOctaves(int octaves) { m_octaves = octaves; }
    
private:
    float m_scale = 1.0f;
    int m_octaves = 4;
};

class FresnelNode : public ShaderNode {
public:
    FresnelNode(int id);
    std::string generateCode(const std::string& outputVar,
                           const std::vector<std::string>& inputVars) const override;
    
    void setPower(float power) { m_power = power; }
    
private:
    float m_power = 5.0f;
};

// Shader graph container
class ShaderGraph {
public:
    ShaderGraph();
    ~ShaderGraph();
    
    // Node management
    int addNode(NodeType type);
    void removeNode(int nodeId);
    ShaderNode* getNode(int nodeId);
    const std::vector<std::unique_ptr<ShaderNode>>& getAllNodes() const { return m_nodes; }
    
    // Connection management
    bool connectNodes(int sourceNodeId, int sourcePinIndex,
                     int targetNodeId, int targetPinIndex);
    void disconnectNodes(int targetNodeId, int targetPinIndex);
    void disconnectAllFromNode(int nodeId);
    const std::vector<NodeConnection>& getConnections() const { return m_connections; }
    
    // Master node
    int getMasterNodeId() const { return m_masterNodeId; }
    
    // Code generation
    bool compile();
    std::string getVertexShader() const { return m_vertexShaderCode; }
    std::string getFragmentShader() const { return m_fragmentShaderCode; }
    Shader* getCompiledShader() { return m_compiledShader; }
    
    // Validation
    bool validate(std::string& errorMessage) const;
    
    // Serialization
    bool saveToFile(const std::string& filename) const;
    bool loadFromFile(const std::string& filename);
    
    // Preview
    void setPreviewNode(int nodeId) { m_previewNodeId = nodeId; }
    int getPreviewNode() const { return m_previewNodeId; }
    
    // Properties
    void setName(const std::string& name) { m_name = name; }
    std::string getName() const { return m_name; }
    
private:
    std::vector<std::unique_ptr<ShaderNode>> m_nodes;
    std::vector<NodeConnection> m_connections;
    int m_nextNodeId;
    int m_masterNodeId;
    int m_previewNodeId;
    
    std::string m_name;
    std::string m_vertexShaderCode;
    std::string m_fragmentShaderCode;
    Shader* m_compiledShader;
    
    // Code generation helpers
    std::string generateNodeCode(ShaderNode* node,
                                std::unordered_map<int, std::string>& nodeOutputVars,
                                int& tempVarCounter);
    std::string getDataTypeString(DataType type) const;
    std::string getDefaultValue(DataType type) const;
    void topologicalSort(std::vector<ShaderNode*>& sortedNodes);
    bool hasCircularDependency() const;
    
    // Node factory
    std::unique_ptr<ShaderNode> createNode(int id, NodeType type);
};

// Shader graph editor (for runtime or in-editor use)
class ShaderGraphEditor {
public:
    ShaderGraphEditor();
    ~ShaderGraphEditor();
    
    void setGraph(ShaderGraph* graph) { m_graph = graph; }
    ShaderGraph* getGraph() { return m_graph; }
    
    // Editor operations
    void update(float deltaTime);
    void render();
    
    // Selection
    void selectNode(int nodeId);
    void deselectAll();
    bool isNodeSelected(int nodeId) const;
    
    // Dragging
    void startDraggingNode(int nodeId, float mouseX, float mouseY);
    void dragNode(float mouseX, float mouseY);
    void stopDragging();
    
    // Connecting
    void startConnection(int nodeId, int pinIndex, bool isInput);
    void updateConnection(float mouseX, float mouseY);
    void endConnection(int nodeId, int pinIndex, bool isInput);
    void cancelConnection();
    
    // Undo/Redo
    void undo();
    void redo();
    bool canUndo() const;
    bool canRedo() const;
    
    // Viewport
    void setViewportPosition(float x, float y) { m_viewX = x; m_viewY = y; }
    void setViewportZoom(float zoom) { m_zoom = zoom; }
    void screenToGraph(float screenX, float screenY, float& graphX, float& graphY) const;
    void graphToScreen(float graphX, float graphY, float& screenX, float& screenY) const;
    
private:
    ShaderGraph* m_graph;
    
    // Selection state
    std::vector<int> m_selectedNodes;
    
    // Dragging state
    bool m_isDragging;
    int m_dragNodeId;
    float m_dragStartX, m_dragStartY;
    float m_dragOffsetX, m_dragOffsetY;
    
    // Connection state
    bool m_isConnecting;
    int m_connectionSourceNode;
    int m_connectionSourcePin;
    bool m_connectionSourceIsInput;
    float m_connectionEndX, m_connectionEndY;
    
    // Viewport
    float m_viewX, m_viewY;
    float m_zoom;
    
    // Undo/Redo
    struct EditorAction {
        enum Type { ADD_NODE, REMOVE_NODE, CONNECT, DISCONNECT, MOVE_NODE } type;
        // Action data...
    };
    std::vector<EditorAction> m_undoStack;
    std::vector<EditorAction> m_redoStack;
    
    void pushAction(const EditorAction& action);
    void renderNode(ShaderNode* node);
    void renderConnection(const NodeConnection& conn);
};

// Material graph (high-level wrapper around shader graph)
class MaterialGraph {
public:
    MaterialGraph();
    ~MaterialGraph();
    
    void setShaderGraph(ShaderGraph* graph) { m_shaderGraph = graph; }
    ShaderGraph* getShaderGraph() { return m_shaderGraph; }
    
    // Compile and create material
    Material* compile();
    
    // Presets
    void loadPBRTemplate();
    void loadUnlitTemplate();
    void loadTerrainTemplate();
    
    // Properties exposed to material inspector
    struct ExposedProperty {
        std::string name;
        DataType type;
        int nodeId;
        std::string propertyName;
    };
    
    void exposeProperty(const std::string& name, int nodeId,
                       const std::string& propertyName, DataType type);
    const std::vector<ExposedProperty>& getExposedProperties() const { return m_properties; }
    
private:
    ShaderGraph* m_shaderGraph;
    std::vector<ExposedProperty> m_properties;
};

} // namespace Graphics
} // namespace JJM

#endif // SHADER_GRAPH_H
