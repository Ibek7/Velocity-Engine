#ifndef ADVANCED_ANIMATION_H
#define ADVANCED_ANIMATION_H

#include "../animation/Animation.h"
#include "../math/Vector2D.h"
#include "../math/Matrix3x3.h"
#include <vector>
#include <string>
#include <memory>
#include <unordered_map>

namespace JJM {
namespace Animation {

// =============================================================================
// Skeletal Animation System
// =============================================================================

struct Bone {
    std::string name;
    int id = -1;
    int parentId = -1;
    
    JJM::Math::Vector2D position;
    float rotation = 0.0f;
    JJM::Math::Vector2D scale = JJM::Math::Vector2D(1, 1);
    
    // Local and world transforms
    JJM::Math::Matrix3x3 localTransform;
    JJM::Math::Matrix3x3 worldTransform;
    JJM::Math::Matrix3x3 inverseBindPose;
    
    std::vector<int> childIds;
    
    Bone(const std::string& boneName = "", int boneId = -1, int parent = -1);
};

class Skeleton {
private:
    std::vector<Bone> bones;
    std::unordered_map<std::string, int> boneNameToId;
    int rootBoneId = -1;

public:
    Skeleton();
    
    int addBone(const std::string& name, int parentId = -1);
    Bone* getBone(int id);
    Bone* getBone(const std::string& name);
    const Bone* getBone(int id) const;
    const Bone* getBone(const std::string& name) const;
    
    int getBoneId(const std::string& name) const;
    int getBoneCount() const { return static_cast<int>(bones.size()); }
    int getRootBoneId() const { return rootBoneId; }
    
    void updateTransforms();
    void updateBoneTransform(int boneId);
    
    void reset();
};

// =============================================================================
// Keyframe Animation
// =============================================================================

struct PositionKey {
    float time;
    JJM::Math::Vector2D position;
    
    PositionKey(float t = 0, const JJM::Math::Vector2D& pos = JJM::Math::Vector2D(0, 0))
        : time(t), position(pos) {}
};

struct RotationKey {
    float time;
    float rotation;
    
    RotationKey(float t = 0, float rot = 0) : time(t), rotation(rot) {}
};

struct ScaleKey {
    float time;
    JJM::Math::Vector2D scale;
    
    ScaleKey(float t = 0, const JJM::Math::Vector2D& s = JJM::Math::Vector2D(1, 1))
        : time(t), scale(s) {}
};

class BoneAnimation {
private:
    std::string boneName;
    std::vector<PositionKey> positionKeys;
    std::vector<RotationKey> rotationKeys;
    std::vector<ScaleKey> scaleKeys;

public:
    explicit BoneAnimation(const std::string& name = "");
    
    void addPositionKey(float time, const JJM::Math::Vector2D& pos);
    void addRotationKey(float time, float rotation);
    void addScaleKey(float time, const JJM::Math::Vector2D& scale);
    
    JJM::Math::Vector2D getPosition(float time) const;
    float getRotation(float time) const;
    JJM::Math::Vector2D getScale(float time) const;
    
    std::string getBoneName() const { return boneName; }
    float getDuration() const;
};

class SkeletalAnimation {
private:
    std::string name;
    float duration;
    std::unordered_map<std::string, BoneAnimation> boneAnimations;

public:
    SkeletalAnimation(const std::string& animName = "", float dur = 0.0f);
    
    void addBoneAnimation(const BoneAnimation& boneAnim);
    BoneAnimation* getBoneAnimation(const std::string& boneName);
    
    std::string getName() const { return name; }
    float getDuration() const { return duration; }
    void setDuration(float dur) { duration = dur; }
};

// =============================================================================
// Animation Blending
// =============================================================================

enum class BlendMode {
    Override,      // Replace current animation
    Additive,      // Add to current animation
    Layered        // Blend with current animation
};

struct AnimationLayer {
    std::string animationName;
    float weight = 1.0f;
    float playbackSpeed = 1.0f;
    bool enabled = true;
    BlendMode blendMode = BlendMode::Override;
};

class AnimationBlender {
private:
    std::vector<AnimationLayer> layers;
    Skeleton* targetSkeleton;

public:
    explicit AnimationBlender(Skeleton* skeleton = nullptr);
    
    void setTargetSkeleton(Skeleton* skeleton) { targetSkeleton = skeleton; }
    
    int addLayer(const std::string& animName, float weight = 1.0f);
    void setLayerWeight(int layerId, float weight);
    void setLayerPlaybackSpeed(int layerId, float speed);
    void enableLayer(int layerId, bool enabled);
    void removeLayer(int layerId);
    
    void blend(const std::vector<SkeletalAnimation>& animations, float time);
    
    int getLayerCount() const { return static_cast<int>(layers.size()); }
};

// =============================================================================
// Inverse Kinematics (IK)
// =============================================================================

class IKSolver {
public:
    virtual ~IKSolver() = default;
    virtual void solve(Skeleton& skeleton) = 0;
};

// Two-bone IK solver (for limbs like arms, legs)
class TwoBoneIK : public IKSolver {
private:
    int rootBoneId;
    int midBoneId;
    int endBoneId;
    JJM::Math::Vector2D targetPosition;
    float maxIterations = 10;
    float tolerance = 0.01f;

public:
    TwoBoneIK(int root, int mid, int end);
    
    void setTarget(const JJM::Math::Vector2D& target) { targetPosition = target; }
    void setMaxIterations(int iterations) { maxIterations = static_cast<float>(iterations); }
    void setTolerance(float tol) { tolerance = tol; }
    
    void solve(Skeleton& skeleton) override;

private:
    void solveTwoBone(Bone* root, Bone* mid, Bone* end);
};

// FABRIK (Forward And Backward Reaching Inverse Kinematics)
class FABRIKSolver : public IKSolver {
private:
    std::vector<int> chain;
    JJM::Math::Vector2D targetPosition;
    int maxIterations = 10;
    float tolerance = 0.01f;

public:
    explicit FABRIKSolver(const std::vector<int>& boneChain);
    
    void setTarget(const JJM::Math::Vector2D& target) { targetPosition = target; }
    void setMaxIterations(int iterations) { maxIterations = iterations; }
    void setTolerance(float tol) { tolerance = tol; }
    
    void solve(Skeleton& skeleton) override;

private:
    void forwardReach(std::vector<JJM::Math::Vector2D>& positions);
    void backwardReach(std::vector<JJM::Math::Vector2D>& positions, 
                      const JJM::Math::Vector2D& origin);
};

// Look-at IK for head/eyes
class LookAtIK : public IKSolver {
private:
    int boneId;
    JJM::Math::Vector2D targetPosition;
    float weight = 1.0f;

public:
    explicit LookAtIK(int bone);
    
    void setTarget(const JJM::Math::Vector2D& target) { targetPosition = target; }
    void setWeight(float w) { weight = w; }
    
    void solve(Skeleton& skeleton) override;
};

// =============================================================================
// Animation Curves
// =============================================================================

enum class CurveInterpolation {
    Linear,
    Bezier,
    Hermite,
    Step
};

template<typename T>
struct CurveKey {
    float time;
    T value;
    T inTangent;
    T outTangent;
    CurveInterpolation interpolation = CurveInterpolation::Linear;
    
    CurveKey(float t = 0, const T& val = T())
        : time(t), value(val), inTangent(T()), outTangent(T()) {}
};

template<typename T>
class AnimationCurve {
private:
    std::vector<CurveKey<T>> keys;
    bool looping = false;

public:
    AnimationCurve() = default;
    
    void addKey(float time, const T& value, CurveInterpolation interp = CurveInterpolation::Linear);
    void addKey(const CurveKey<T>& key);
    
    T evaluate(float time) const;
    
    void setLooping(bool loop) { looping = loop; }
    bool isLooping() const { return looping; }
    
    int getKeyCount() const { return static_cast<int>(keys.size()); }
    float getDuration() const;
    
    void clear() { keys.clear(); }

private:
    T interpolate(const CurveKey<T>& k1, const CurveKey<T>& k2, float t) const;
    T bezierInterpolate(const T& p0, const T& p1, const T& p2, const T& p3, float t) const;
    T hermiteInterpolate(const T& p0, const T& p1, const T& m0, const T& m1, float t) const;
};

// =============================================================================
// Procedural Animation
// =============================================================================

class ProceduralAnimation {
public:
    virtual ~ProceduralAnimation() = default;
    virtual void update(Skeleton& skeleton, float deltaTime) = 0;
};

// Breathing animation
class BreathingAnimation : public ProceduralAnimation {
private:
    std::vector<int> affectedBones;
    float frequency = 0.3f;
    float amplitude = 0.02f;
    float phase = 0.0f;

public:
    explicit BreathingAnimation(const std::vector<int>& bones);
    
    void setFrequency(float freq) { frequency = freq; }
    void setAmplitude(float amp) { amplitude = amp; }
    
    void update(Skeleton& skeleton, float deltaTime) override;
};

// Idle sway animation
class IdleSwayAnimation : public ProceduralAnimation {
private:
    int rootBoneId;
    float swayAmount = 0.1f;
    float swaySpeed = 0.5f;
    float time = 0.0f;

public:
    explicit IdleSwayAnimation(int rootBone);
    
    void setSwayAmount(float amount) { swayAmount = amount; }
    void setSwaySpeed(float speed) { swaySpeed = speed; }
    
    void update(Skeleton& skeleton, float deltaTime) override;
};

// =============================================================================
// Animation State Machine
// =============================================================================

struct AnimationTransition {
    std::string fromState;
    std::string toState;
    float blendDuration = 0.3f;
    std::function<bool()> condition;
    
    AnimationTransition(const std::string& from, const std::string& to, 
                       float blend = 0.3f, std::function<bool()> cond = nullptr)
        : fromState(from), toState(to), blendDuration(blend), condition(cond) {}
};

class AnimationStateMachine {
private:
    std::unordered_map<std::string, SkeletalAnimation> states;
    std::vector<AnimationTransition> transitions;
    
    std::string currentState;
    std::string previousState;
    float currentTime = 0.0f;
    float transitionTime = 0.0f;
    float transitionDuration = 0.0f;
    bool inTransition = false;
    
    Skeleton* targetSkeleton;

public:
    explicit AnimationStateMachine(Skeleton* skeleton = nullptr);
    
    void addState(const std::string& name, const SkeletalAnimation& animation);
    void addTransition(const AnimationTransition& transition);
    void addTransition(const std::string& from, const std::string& to, 
                      float blendDuration = 0.3f, std::function<bool()> condition = nullptr);
    
    void setState(const std::string& state);
    void update(float deltaTime);
    
    std::string getCurrentState() const { return currentState; }
    bool isInTransition() const { return inTransition; }
    
    void setTargetSkeleton(Skeleton* skeleton) { targetSkeleton = skeleton; }
};

// =============================================================================
// Animation Retargeting
// =============================================================================

struct BoneMapping {
    std::string sourceBoneName;
    std::string targetBoneName;
    float scaleCompensation = 1.0f;
};

class AnimationRetargeting {
private:
    std::vector<BoneMapping> boneMappings;

public:
    AnimationRetargeting() = default;
    
    void addBoneMapping(const std::string& source, const std::string& target, 
                       float scaleComp = 1.0f);
    
    SkeletalAnimation retargetAnimation(const SkeletalAnimation& sourceAnim, 
                                        const Skeleton& sourceSkeleton,
                                        const Skeleton& targetSkeleton) const;
    
    void clearMappings() { boneMappings.clear(); }
};

// =============================================================================
// Animation Timeline
// =============================================================================

struct TimelineEvent {
    float time;
    std::string eventName;
    std::function<void()> callback;
    
    TimelineEvent(float t = 0, const std::string& name = "", 
                 std::function<void()> cb = nullptr)
        : time(t), eventName(name), callback(cb) {}
};

class AnimationTimeline {
private:
    std::vector<TimelineEvent> events;
    std::vector<bool> eventTriggered;
    float currentTime = 0.0f;

public:
    AnimationTimeline() = default;
    
    void addEvent(float time, const std::string& eventName, 
                 std::function<void()> callback = nullptr);
    void update(float deltaTime);
    void reset();
    
    void setTime(float time) { currentTime = time; }
    float getTime() const { return currentTime; }
    
    void clear();
};

// =============================================================================
// Advanced Animator
// =============================================================================

class AdvancedAnimator {
private:
    Skeleton skeleton;
    std::unordered_map<std::string, SkeletalAnimation> animations;
    std::unique_ptr<AnimationStateMachine> stateMachine;
    std::unique_ptr<AnimationBlender> blender;
    std::vector<std::unique_ptr<IKSolver>> ikSolvers;
    std::vector<std::unique_ptr<ProceduralAnimation>> proceduralAnims;
    AnimationTimeline timeline;
    
    bool ikEnabled = true;
    bool proceduralEnabled = true;

public:
    AdvancedAnimator();
    
    Skeleton& getSkeleton() { return skeleton; }
    const Skeleton& getSkeleton() const { return skeleton; }
    
    void addAnimation(const SkeletalAnimation& animation);
    void playAnimation(const std::string& name);
    
    void enableStateMachine();
    AnimationStateMachine* getStateMachine() { return stateMachine.get(); }
    
    void enableBlending();
    AnimationBlender* getBlender() { return blender.get(); }
    
    void addIKSolver(std::unique_ptr<IKSolver> solver);
    void addProceduralAnimation(std::unique_ptr<ProceduralAnimation> anim);
    
    void setIKEnabled(bool enabled) { ikEnabled = enabled; }
    void setProceduralEnabled(bool enabled) { proceduralEnabled = enabled; }
    
    AnimationTimeline& getTimeline() { return timeline; }
    
    void update(float deltaTime);
};

} // namespace Animation
} // namespace JJM

#endif // ADVANCED_ANIMATION_H
