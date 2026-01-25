#ifndef MORPH_TARGET_SYSTEM_H
#define MORPH_TARGET_SYSTEM_H

#include <string>
#include <vector>
#include <unordered_map>

namespace JJM {
namespace Animation {

struct MorphTarget {
    std::string name;
    std::vector<float> positions;  // Vertex position deltas
    std::vector<float> normals;    // Normal deltas
    float weight;                  // 0-1
    
    MorphTarget() : weight(0.0f) {}
};

class MorphTargetController {
public:
    MorphTargetController();
    ~MorphTargetController();
    
    void addMorphTarget(const std::string& name, const MorphTarget& target);
    void setWeight(const std::string& name, float weight);
    float getWeight(const std::string& name) const;
    
    void update(float deltaTime);
    void applyToMesh();
    
    const std::vector<MorphTarget>& getMorphTargets() const { return m_targets; }
    
private:
    std::vector<MorphTarget> m_targets;
    std::unordered_map<std::string, int> m_targetIndices;
};

class BlendShapeSystem {
public:
    void registerController(MorphTargetController* controller);
    void unregisterController(MorphTargetController* controller);
    
    void update(float deltaTime);
    
private:
    std::vector<MorphTargetController*> m_controllers;
};

} // namespace Animation
} // namespace JJM

#endif
