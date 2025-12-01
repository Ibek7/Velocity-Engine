#include "../../include/animation/AdvancedAnimation.h"
#include <algorithm>
#include <cmath>
#include <numeric>

namespace JJM {
namespace Animation {

// Helper function to create TRS matrix
static JJM::Math::Matrix3x3 createTRS(const JJM::Math::Vector2D& pos, 
                                      float rotation, 
                                      const JJM::Math::Vector2D& scale) {
    JJM::Math::Matrix3x3 translation = JJM::Math::Matrix3x3::Translation(pos);
    JJM::Math::Matrix3x3 rot = JJM::Math::Matrix3x3::Rotation(rotation);
    JJM::Math::Matrix3x3 scaleMatrix = JJM::Math::Matrix3x3::Scale(scale.x, scale.y);
    return translation * rot * scaleMatrix;
}

// =============================================================================
// Bone Implementation
// =============================================================================

Bone::Bone(const std::string& boneName, int boneId, int parent)
    : name(boneName), id(boneId), parentId(parent) {
    localTransform = JJM::Math::Matrix3x3::Identity();
    worldTransform = JJM::Math::Matrix3x3::Identity();
    inverseBindPose = JJM::Math::Matrix3x3::Identity();
}

// =============================================================================
// Skeleton Implementation
// =============================================================================

Skeleton::Skeleton() {}

int Skeleton::addBone(const std::string& name, int parentId) {
    int id = static_cast<int>(bones.size());
    bones.emplace_back(name, id, parentId);
    boneNameToId[name] = id;
    
    if (parentId >= 0 && parentId < static_cast<int>(bones.size())) {
        bones[parentId].childIds.push_back(id);
    } else {
        rootBoneId = id;
    }
    
    return id;
}

Bone* Skeleton::getBone(int id) {
    if (id >= 0 && id < static_cast<int>(bones.size())) {
        return &bones[id];
    }
    return nullptr;
}

Bone* Skeleton::getBone(const std::string& name) {
    auto it = boneNameToId.find(name);
    if (it != boneNameToId.end()) {
        return &bones[it->second];
    }
    return nullptr;
}

const Bone* Skeleton::getBone(int id) const {
    if (id >= 0 && id < static_cast<int>(bones.size())) {
        return &bones[id];
    }
    return nullptr;
}

const Bone* Skeleton::getBone(const std::string& name) const {
    auto it = boneNameToId.find(name);
    if (it != boneNameToId.end()) {
        return &bones[it->second];
    }
    return nullptr;
}

int Skeleton::getBoneId(const std::string& name) const {
    auto it = boneNameToId.find(name);
    return (it != boneNameToId.end()) ? it->second : -1;
}

void Skeleton::updateTransforms() {
    if (rootBoneId >= 0) {
        updateBoneTransform(rootBoneId);
    }
}

void Skeleton::updateBoneTransform(int boneId) {
    Bone* bone = getBone(boneId);
    if (!bone) return;
    
    // Update local transform from position, rotation, scale
    bone->localTransform = createTRS(bone->position, bone->rotation, bone->scale);
    
    // Calculate world transform
    if (bone->parentId >= 0) {
        Bone* parent = getBone(bone->parentId);
        bone->worldTransform = parent->worldTransform * bone->localTransform;
    } else {
        bone->worldTransform = bone->localTransform;
    }
    
    // Update children
    for (int childId : bone->childIds) {
        updateBoneTransform(childId);
    }
}

void Skeleton::reset() {
    for (auto& bone : bones) {
        bone.position = JJM::Math::Vector2D(0, 0);
        bone.rotation = 0.0f;
        bone.scale = JJM::Math::Vector2D(1, 1);
    }
    updateTransforms();
}

// =============================================================================
// BoneAnimation Implementation
// =============================================================================

BoneAnimation::BoneAnimation(const std::string& name) : boneName(name) {}

void BoneAnimation::addPositionKey(float time, const JJM::Math::Vector2D& pos) {
    positionKeys.emplace_back(time, pos);
    std::sort(positionKeys.begin(), positionKeys.end(), 
             [](const PositionKey& a, const PositionKey& b) { return a.time < b.time; });
}

void BoneAnimation::addRotationKey(float time, float rotation) {
    rotationKeys.emplace_back(time, rotation);
    std::sort(rotationKeys.begin(), rotationKeys.end(),
             [](const RotationKey& a, const RotationKey& b) { return a.time < b.time; });
}

void BoneAnimation::addScaleKey(float time, const JJM::Math::Vector2D& scale) {
    scaleKeys.emplace_back(time, scale);
    std::sort(scaleKeys.begin(), scaleKeys.end(),
             [](const ScaleKey& a, const ScaleKey& b) { return a.time < b.time; });
}

JJM::Math::Vector2D BoneAnimation::getPosition(float time) const {
    if (positionKeys.empty()) return JJM::Math::Vector2D(0, 0);
    if (positionKeys.size() == 1 || time <= positionKeys.front().time) {
        return positionKeys.front().position;
    }
    if (time >= positionKeys.back().time) {
        return positionKeys.back().position;
    }
    
    for (size_t i = 0; i < positionKeys.size() - 1; i++) {
        if (time >= positionKeys[i].time && time < positionKeys[i + 1].time) {
            float t = (time - positionKeys[i].time) / 
                     (positionKeys[i + 1].time - positionKeys[i].time);
            return positionKeys[i].position.lerp(positionKeys[i + 1].position, t);
        }
    }
    
    return positionKeys.back().position;
}

float BoneAnimation::getRotation(float time) const {
    if (rotationKeys.empty()) return 0.0f;
    if (rotationKeys.size() == 1 || time <= rotationKeys.front().time) {
        return rotationKeys.front().rotation;
    }
    if (time >= rotationKeys.back().time) {
        return rotationKeys.back().rotation;
    }
    
    for (size_t i = 0; i < rotationKeys.size() - 1; i++) {
        if (time >= rotationKeys[i].time && time < rotationKeys[i + 1].time) {
            float t = (time - rotationKeys[i].time) / 
                     (rotationKeys[i + 1].time - rotationKeys[i].time);
            return rotationKeys[i].rotation + 
                   (rotationKeys[i + 1].rotation - rotationKeys[i].rotation) * t;
        }
    }
    
    return rotationKeys.back().rotation;
}

JJM::Math::Vector2D BoneAnimation::getScale(float time) const {
    if (scaleKeys.empty()) return JJM::Math::Vector2D(1, 1);
    if (scaleKeys.size() == 1 || time <= scaleKeys.front().time) {
        return scaleKeys.front().scale;
    }
    if (time >= scaleKeys.back().time) {
        return scaleKeys.back().scale;
    }
    
    for (size_t i = 0; i < scaleKeys.size() - 1; i++) {
        if (time >= scaleKeys[i].time && time < scaleKeys[i + 1].time) {
            float t = (time - scaleKeys[i].time) / 
                     (scaleKeys[i + 1].time - scaleKeys[i].time);
            return scaleKeys[i].scale.lerp(scaleKeys[i + 1].scale, t);
        }
    }
    
    return scaleKeys.back().scale;
}

float BoneAnimation::getDuration() const {
    float maxTime = 0.0f;
    
    if (!positionKeys.empty()) {
        maxTime = std::max(maxTime, positionKeys.back().time);
    }
    if (!rotationKeys.empty()) {
        maxTime = std::max(maxTime, rotationKeys.back().time);
    }
    if (!scaleKeys.empty()) {
        maxTime = std::max(maxTime, scaleKeys.back().time);
    }
    
    return maxTime;
}

// =============================================================================
// SkeletalAnimation Implementation
// =============================================================================

SkeletalAnimation::SkeletalAnimation(const std::string& animName, float dur)
    : name(animName), duration(dur) {}

void SkeletalAnimation::addBoneAnimation(const BoneAnimation& boneAnim) {
    boneAnimations[boneAnim.getBoneName()] = boneAnim;
    
    float boneAnimDuration = boneAnim.getDuration();
    if (boneAnimDuration > duration) {
        duration = boneAnimDuration;
    }
}

BoneAnimation* SkeletalAnimation::getBoneAnimation(const std::string& boneName) {
    auto it = boneAnimations.find(boneName);
    return (it != boneAnimations.end()) ? &it->second : nullptr;
}

// =============================================================================
// AnimationBlender Implementation
// =============================================================================

AnimationBlender::AnimationBlender(Skeleton* skeleton) : targetSkeleton(skeleton) {}

int AnimationBlender::addLayer(const std::string& animName, float weight) {
    AnimationLayer layer;
    layer.animationName = animName;
    layer.weight = weight;
    
    layers.push_back(layer);
    return static_cast<int>(layers.size() - 1);
}

void AnimationBlender::setLayerWeight(int layerId, float weight) {
    if (layerId >= 0 && layerId < static_cast<int>(layers.size())) {
        layers[layerId].weight = weight;
    }
}

void AnimationBlender::setLayerPlaybackSpeed(int layerId, float speed) {
    if (layerId >= 0 && layerId < static_cast<int>(layers.size())) {
        layers[layerId].playbackSpeed = speed;
    }
}

void AnimationBlender::enableLayer(int layerId, bool enabled) {
    if (layerId >= 0 && layerId < static_cast<int>(layers.size())) {
        layers[layerId].enabled = enabled;
    }
}

void AnimationBlender::removeLayer(int layerId) {
    if (layerId >= 0 && layerId < static_cast<int>(layers.size())) {
        layers.erase(layers.begin() + layerId);
    }
}

void AnimationBlender::blend(const std::vector<SkeletalAnimation>& animations, float time) {
    // Simplified blending - full implementation would blend bone transforms
    // from multiple animations based on layer weights
}

// =============================================================================
// TwoBoneIK Implementation
// =============================================================================

TwoBoneIK::TwoBoneIK(int root, int mid, int end)
    : rootBoneId(root), midBoneId(mid), endBoneId(end) {}

void TwoBoneIK::solve(Skeleton& skeleton) {
    Bone* root = skeleton.getBone(rootBoneId);
    Bone* mid = skeleton.getBone(midBoneId);
    Bone* end = skeleton.getBone(endBoneId);
    
    if (!root || !mid || !end) return;
    
    solveTwoBone(root, mid, end);
}

void TwoBoneIK::solveTwoBone(Bone* root, Bone* mid, Bone* end) {
    JJM::Math::Vector2D rootPos = root->position;
    JJM::Math::Vector2D midPos = mid->position;
    JJM::Math::Vector2D endPos = end->position;
    
    float upperLength = (midPos - rootPos).magnitude();
    float lowerLength = (endPos - midPos).magnitude();
    float targetDist = (targetPosition - rootPos).magnitude();
    
    // Check if target is reachable
    float maxReach = upperLength + lowerLength;
    if (targetDist > maxReach) {
        targetDist = maxReach;
    }
    
    // Law of cosines to find angles
    float cosAngle = (upperLength * upperLength + lowerLength * lowerLength - 
                     targetDist * targetDist) / (2 * upperLength * lowerLength);
    cosAngle = std::max(-1.0f, std::min(1.0f, cosAngle));
    
    float angle = std::acos(cosAngle);
    
    // Update mid bone rotation
    mid->rotation = 3.14159f - angle;
    
    // Update root bone rotation to point toward target
    JJM::Math::Vector2D toTarget = targetPosition - rootPos;
    root->rotation = std::atan2(toTarget.y, toTarget.x);
}

// =============================================================================
// FABRIKSolver Implementation
// =============================================================================

FABRIKSolver::FABRIKSolver(const std::vector<int>& boneChain) : chain(boneChain) {}

void FABRIKSolver::solve(Skeleton& skeleton) {
    if (chain.empty()) return;
    
    std::vector<JJM::Math::Vector2D> positions;
    for (int boneId : chain) {
        Bone* bone = skeleton.getBone(boneId);
        if (bone) {
            positions.push_back(bone->position);
        }
    }
    
    if (positions.empty()) return;
    
    JJM::Math::Vector2D origin = positions.front();
    
    for (int iter = 0; iter < maxIterations; iter++) {
        forwardReach(positions);
        backwardReach(positions, origin);
        
        // Check convergence
        float dist = (positions.back() - targetPosition).magnitude();
        if (dist < tolerance) break;
    }
    
    // Apply solved positions back to skeleton
    for (size_t i = 0; i < chain.size() && i < positions.size(); i++) {
        Bone* bone = skeleton.getBone(chain[i]);
        if (bone) {
            bone->position = positions[i];
        }
    }
}

void FABRIKSolver::forwardReach(std::vector<JJM::Math::Vector2D>& positions) {
    positions.back() = targetPosition;
    
    for (int i = static_cast<int>(positions.size()) - 2; i >= 0; i--) {
        JJM::Math::Vector2D dir = positions[i] - positions[i + 1];
        dir.normalize();
        float length = (positions[i] - positions[i + 1]).magnitude();
        positions[i] = positions[i + 1] + dir * length;
    }
}

void FABRIKSolver::backwardReach(std::vector<JJM::Math::Vector2D>& positions,
                                 const JJM::Math::Vector2D& origin) {
    positions.front() = origin;
    
    for (size_t i = 0; i < positions.size() - 1; i++) {
        JJM::Math::Vector2D dir = positions[i + 1] - positions[i];
        dir.normalize();
        float length = (positions[i + 1] - positions[i]).magnitude();
        positions[i + 1] = positions[i] + dir * length;
    }
}

// =============================================================================
// LookAtIK Implementation
// =============================================================================

LookAtIK::LookAtIK(int bone) : boneId(bone) {}

void LookAtIK::solve(Skeleton& skeleton) {
    Bone* bone = skeleton.getBone(boneId);
    if (!bone) return;
    
    JJM::Math::Vector2D direction = targetPosition - bone->position;
    float targetRotation = std::atan2(direction.y, direction.x);
    
    // Blend rotation based on weight
    bone->rotation = bone->rotation + (targetRotation - bone->rotation) * weight;
}

// =============================================================================
// Procedural Animations
// =============================================================================

BreathingAnimation::BreathingAnimation(const std::vector<int>& bones)
    : affectedBones(bones) {}

void BreathingAnimation::update(Skeleton& skeleton, float deltaTime) {
    phase += deltaTime * frequency * 2.0f * 3.14159f;
    
    float breathingScale = 1.0f + std::sin(phase) * amplitude;
    
    for (int boneId : affectedBones) {
        Bone* bone = skeleton.getBone(boneId);
        if (bone) {
            bone->scale.y = breathingScale;
        }
    }
}

IdleSwayAnimation::IdleSwayAnimation(int rootBone) : rootBoneId(rootBone) {}

void IdleSwayAnimation::update(Skeleton& skeleton, float deltaTime) {
    time += deltaTime * swaySpeed;
    
    Bone* root = skeleton.getBone(rootBoneId);
    if (root) {
        float sway = std::sin(time) * swayAmount;
        root->rotation = sway;
    }
}

// =============================================================================
// AnimationStateMachine Implementation
// =============================================================================

AnimationStateMachine::AnimationStateMachine(Skeleton* skeleton)
    : targetSkeleton(skeleton) {}

void AnimationStateMachine::addState(const std::string& name, 
                                    const SkeletalAnimation& animation) {
    states[name] = animation;
}

void AnimationStateMachine::addTransition(const AnimationTransition& transition) {
    transitions.push_back(transition);
}

void AnimationStateMachine::addTransition(const std::string& from, const std::string& to,
                                         float blendDuration, std::function<bool()> condition) {
    transitions.emplace_back(from, to, blendDuration, condition);
}

void AnimationStateMachine::setState(const std::string& state) {
    if (states.find(state) != states.end()) {
        previousState = currentState;
        currentState = state;
        currentTime = 0.0f;
    }
}

void AnimationStateMachine::update(float deltaTime) {
    // Check transitions
    for (const auto& transition : transitions) {
        if (transition.fromState == currentState) {
            if (!transition.condition || transition.condition()) {
                previousState = currentState;
                currentState = transition.toState;
                currentTime = 0.0f;
                inTransition = true;
                transitionTime = 0.0f;
                transitionDuration = transition.blendDuration;
                break;
            }
        }
    }
    
    // Update transition
    if (inTransition) {
        transitionTime += deltaTime;
        if (transitionTime >= transitionDuration) {
            inTransition = false;
        }
    }
    
    currentTime += deltaTime;
}

// =============================================================================
// AnimationRetargeting Implementation
// =============================================================================

void AnimationRetargeting::addBoneMapping(const std::string& source, const std::string& target,
                                         float scaleComp) {
    BoneMapping mapping;
    mapping.sourceBoneName = source;
    mapping.targetBoneName = target;
    mapping.scaleCompensation = scaleComp;
    boneMappings.push_back(mapping);
}

SkeletalAnimation AnimationRetargeting::retargetAnimation(
    const SkeletalAnimation& sourceAnim,
    const Skeleton& sourceSkeleton,
    const Skeleton& targetSkeleton) const {
    
    SkeletalAnimation retargeted(sourceAnim.getName(), sourceAnim.getDuration());
    
    // Simplified retargeting - copy animations with bone name mapping
    for (const auto& mapping : boneMappings) {
        // Implementation would copy and scale keyframes based on skeleton differences
    }
    
    return retargeted;
}

// =============================================================================
// AnimationTimeline Implementation
// =============================================================================

void AnimationTimeline::addEvent(float time, const std::string& eventName,
                                std::function<void()> callback) {
    events.emplace_back(time, eventName, callback);
    eventTriggered.push_back(false);
    
    // Sort events by time
    std::vector<size_t> indices(events.size());
    std::iota(indices.begin(), indices.end(), 0);
    std::sort(indices.begin(), indices.end(),
             [this](size_t a, size_t b) { return events[a].time < events[b].time; });
    
    std::vector<TimelineEvent> sortedEvents;
    std::vector<bool> sortedTriggered;
    for (size_t idx : indices) {
        sortedEvents.push_back(events[idx]);
        sortedTriggered.push_back(eventTriggered[idx]);
    }
    
    events = sortedEvents;
    eventTriggered = sortedTriggered;
}

void AnimationTimeline::update(float deltaTime) {
    currentTime += deltaTime;
    
    for (size_t i = 0; i < events.size(); i++) {
        if (!eventTriggered[i] && currentTime >= events[i].time) {
            if (events[i].callback) {
                events[i].callback();
            }
            eventTriggered[i] = true;
        }
    }
}

void AnimationTimeline::reset() {
    currentTime = 0.0f;
    std::fill(eventTriggered.begin(), eventTriggered.end(), false);
}

void AnimationTimeline::clear() {
    events.clear();
    eventTriggered.clear();
    currentTime = 0.0f;
}

// =============================================================================
// AdvancedAnimator Implementation
// =============================================================================

AdvancedAnimator::AdvancedAnimator() {}

void AdvancedAnimator::addAnimation(const SkeletalAnimation& animation) {
    animations[animation.getName()] = animation;
}

void AdvancedAnimator::playAnimation(const std::string& name) {
    if (stateMachine) {
        stateMachine->setState(name);
    }
}

void AdvancedAnimator::enableStateMachine() {
    if (!stateMachine) {
        stateMachine = std::make_unique<AnimationStateMachine>(&skeleton);
    }
}

void AdvancedAnimator::enableBlending() {
    if (!blender) {
        blender = std::make_unique<AnimationBlender>(&skeleton);
    }
}

void AdvancedAnimator::addIKSolver(std::unique_ptr<IKSolver> solver) {
    ikSolvers.push_back(std::move(solver));
}

void AdvancedAnimator::addProceduralAnimation(std::unique_ptr<ProceduralAnimation> anim) {
    proceduralAnims.push_back(std::move(anim));
}

void AdvancedAnimator::update(float deltaTime) {
    // Update state machine
    if (stateMachine) {
        stateMachine->update(deltaTime);
    }
    
    // Update procedural animations
    if (proceduralEnabled) {
        for (auto& anim : proceduralAnims) {
            anim->update(skeleton, deltaTime);
        }
    }
    
    // Apply IK
    if (ikEnabled) {
        for (auto& solver : ikSolvers) {
            solver->solve(skeleton);
        }
    }
    
    // Update timeline
    timeline.update(deltaTime);
    
    // Update skeleton transforms
    skeleton.updateTransforms();
}

} // namespace Animation
} // namespace JJM
