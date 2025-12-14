#include "animation/SkeletalAnimation.h"
#include <algorithm>
#include <cmath>

namespace JJM {
namespace Animation {

// BoneAnimation implementation
BoneKeyframe BoneAnimation::interpolate(double time) const {
    if (keyframes.empty()) {
        return BoneKeyframe();
    }
    
    if (keyframes.size() == 1 || time <= keyframes[0].time) {
        return keyframes[0];
    }
    
    if (time >= keyframes.back().time) {
        return keyframes.back();
    }
    
    // Find keyframes to interpolate between
    for (size_t i = 0; i < keyframes.size() - 1; ++i) {
        if (time >= keyframes[i].time && time <= keyframes[i + 1].time) {
            const auto& kf1 = keyframes[i];
            const auto& kf2 = keyframes[i + 1];
            
            float t = static_cast<float>((time - kf1.time) / (kf2.time - kf1.time));
            
            BoneKeyframe result;
            result.time = time;
            result.position = kf1.position + (kf2.position - kf1.position) * t;
            result.rotation = Quaternion::slerp(kf1.rotation, kf2.rotation, t);
            result.scale = kf1.scale + (kf2.scale - kf1.scale) * t;
            
            return result;
        }
    }
    
    return keyframes[0];
}

void BoneAnimation::addKeyframe(double time, const Math::Vector2D& pos,
                                const Quaternion& rot, const Math::Vector2D& scale) {
    BoneKeyframe kf;
    kf.time = time;
    kf.position = pos;
    kf.rotation = rot;
    kf.scale = scale;
    
    keyframes.push_back(kf);
    
    std::sort(keyframes.begin(), keyframes.end(),
              [](const BoneKeyframe& a, const BoneKeyframe& b) {
                  return a.time < b.time;
              });
}

// AnimationClip implementation
AnimationClip::AnimationClip()
    : name(""), duration(0.0), looping(true) {}

AnimationClip::AnimationClip(const std::string& name)
    : name(name), duration(0.0), looping(true) {}

AnimationClip::~AnimationClip() {}

void AnimationClip::addBoneAnimation(const std::string& boneName) {
    boneAnimations.emplace_back(boneName);
}

BoneAnimation* AnimationClip::getBoneAnimation(const std::string& boneName) {
    for (auto& anim : boneAnimations) {
        if (anim.boneName == boneName) {
            return &anim;
        }
    }
    return nullptr;
}

// Skeleton implementation
Skeleton::Skeleton() {}

Skeleton::~Skeleton() {}

int Skeleton::addBone(const std::string& name, int parentIndex) {
    int index = static_cast<int>(bones.size());
    bones.emplace_back(name, parentIndex);
    boneNameToIndex[name] = index;
    finalTransforms.push_back(Matrix4());
    return index;
}

Bone* Skeleton::getBone(int index) {
    if (index >= 0 && index < static_cast<int>(bones.size())) {
        return &bones[index];
    }
    return nullptr;
}

Bone* Skeleton::getBone(const std::string& name) {
    auto it = boneNameToIndex.find(name);
    if (it != boneNameToIndex.end()) {
        return &bones[it->second];
    }
    return nullptr;
}

int Skeleton::getBoneIndex(const std::string& name) const {
    auto it = boneNameToIndex.find(name);
    if (it != boneNameToIndex.end()) {
        return it->second;
    }
    return -1;
}

void Skeleton::updateBoneTransforms() {
    for (size_t i = 0; i < bones.size(); ++i) {
        if (bones[i].parentIndex == -1) {
            updateBoneHierarchy(static_cast<int>(i), Matrix4());
        }
    }
}

void Skeleton::updateBoneTransforms(const std::vector<Matrix4>& boneTransforms) {
    if (boneTransforms.size() != bones.size()) return;
    
    for (size_t i = 0; i < bones.size(); ++i) {
        bones[i].localTransform = boneTransforms[i];
    }
    
    updateBoneTransforms();
}

void Skeleton::updateBoneHierarchy(int boneIndex, const Matrix4& parentTransform) {
    Bone& bone = bones[boneIndex];
    bone.worldTransform = parentTransform * bone.localTransform;
    finalTransforms[boneIndex] = bone.worldTransform * bone.offsetMatrix;
    
    for (size_t i = 0; i < bones.size(); ++i) {
        if (bones[i].parentIndex == boneIndex) {
            updateBoneHierarchy(static_cast<int>(i), bone.worldTransform);
        }
    }
}

// SkeletalAnimator implementation
SkeletalAnimator::SkeletalAnimator()
    : currentClip(nullptr), currentTime(0.0), playbackSpeed(1.0f),
      playing(false), paused(false) {}

SkeletalAnimator::SkeletalAnimator(std::shared_ptr<Skeleton> skeleton)
    : skeleton(skeleton), currentClip(nullptr), currentTime(0.0),
      playbackSpeed(1.0f), playing(false), paused(false) {}

SkeletalAnimator::~SkeletalAnimator() {}

void SkeletalAnimator::setSkeleton(std::shared_ptr<Skeleton> skeleton) {
    this->skeleton = skeleton;
}

void SkeletalAnimator::addAnimationClip(std::shared_ptr<AnimationClip> clip) {
    if (clip) {
        animationClips[clip->getName()] = clip;
    }
}

void SkeletalAnimator::removeAnimationClip(const std::string& name) {
    animationClips.erase(name);
}

AnimationClip* SkeletalAnimator::getAnimationClip(const std::string& name) {
    auto it = animationClips.find(name);
    if (it != animationClips.end()) {
        return it->second.get();
    }
    return nullptr;
}

void SkeletalAnimator::play(const std::string& clipName, bool restart) {
    auto it = animationClips.find(clipName);
    if (it != animationClips.end()) {
        currentClipName = clipName;
        currentClip = it->second.get();
        if (restart || currentClipName != clipName) {
            currentTime = 0.0;
        }
        playing = true;
        paused = false;
    }
}

void SkeletalAnimator::stop() {
    playing = false;
    paused = false;
    currentTime = 0.0;
}

void SkeletalAnimator::pause() {
    paused = true;
}

void SkeletalAnimator::resume() {
    paused = false;
}

void SkeletalAnimator::setTime(double time) {
    currentTime = time;
}

void SkeletalAnimator::update(double deltaTime) {
    if (!playing || paused || !currentClip || !skeleton) {
        return;
    }
    
    currentTime += deltaTime * playbackSpeed;
    
    if (currentTime >= currentClip->getDuration()) {
        if (currentClip->isLooping()) {
            currentTime = std::fmod(currentTime, currentClip->getDuration());
        } else {
            currentTime = currentClip->getDuration();
            playing = false;
        }
    }
    
    applyAnimation();
}

void SkeletalAnimator::applyAnimation() {
    if (!currentClip || !skeleton) return;
    
    std::vector<Matrix4> boneTransforms(skeleton->getBoneCount());
    calculateBoneTransforms(boneTransforms);
    skeleton->updateBoneTransforms(boneTransforms);
}

void SkeletalAnimator::calculateBoneTransforms(std::vector<Matrix4>& transforms) {
    for (size_t i = 0; i < skeleton->getBoneCount(); ++i) {
        Bone* bone = skeleton->getBone(static_cast<int>(i));
        if (!bone) continue;
        
        BoneAnimation* boneAnim = currentClip->getBoneAnimation(bone->name);
        if (boneAnim) {
            BoneKeyframe kf = boneAnim->interpolate(currentTime);
            
            Matrix4 translation = Matrix4::translation(kf.position);
            Matrix4 rotation = kf.rotation.toMatrix();
            Matrix4 scale = Matrix4::scale(kf.scale);
            
            transforms[i] = translation * rotation * scale;
        } else {
            transforms[i] = bone->localTransform;
        }
    }
}

// AnimationBlendTree implementation
AnimationBlendTree::AnimationBlendTree() {}

AnimationBlendTree::~AnimationBlendTree() {}

void AnimationBlendTree::addAnimation(const std::string& name, std::shared_ptr<AnimationClip> clip) {
    BlendNode node;
    node.clip = clip;
    node.weight = 0.0f;
    node.time = 0.0;
    blendNodes[name] = node;
}

void AnimationBlendTree::setBlendWeight(const std::string& name, float weight) {
    auto it = blendNodes.find(name);
    if (it != blendNodes.end()) {
        it->second.weight = std::max(0.0f, std::min(1.0f, weight));
    }
}

void AnimationBlendTree::update(double deltaTime, Skeleton* skeleton) {
    if (!skeleton || blendNodes.empty()) return;
    
    normalizeWeights();
    
    std::vector<Matrix4> blendedTransforms(skeleton->getBoneCount());
    
    for (size_t i = 0; i < skeleton->getBoneCount(); ++i) {
        blendedTransforms[i] = Matrix4();
    }
    
    for (auto& pair : blendNodes) {
        BlendNode& node = pair.second;
        if (node.weight <= 0.0f || !node.clip) continue;
        
        node.time += deltaTime;
        if (node.time >= node.clip->getDuration()) {
            if (node.clip->isLooping()) {
                node.time = std::fmod(node.time, node.clip->getDuration());
            }
        }
    }
    
    skeleton->updateBoneTransforms(blendedTransforms);
}

void AnimationBlendTree::clear() {
    blendNodes.clear();
}

void AnimationBlendTree::normalizeWeights() {
    float totalWeight = 0.0f;
    for (const auto& pair : blendNodes) {
        totalWeight += pair.second.weight;
    }
    
    if (totalWeight > 0.0f) {
        for (auto& pair : blendNodes) {
            pair.second.weight /= totalWeight;
        }
    }
}

// InverseKinematics implementation
InverseKinematics::InverseKinematics()
    : maxIterations(10), tolerance(0.001f) {}

InverseKinematics::~InverseKinematics() {}

void InverseKinematics::solve(Skeleton* skeleton, int endEffectorIndex,
                              const Math::Vector2D& targetPosition, int iterations) {
    if (!skeleton) return;
    
    int actualIterations = (iterations > 0) ? iterations : maxIterations;
    
    for (int iter = 0; iter < actualIterations; ++iter) {
        Math::Vector2D currentPos = getEndEffectorPosition(skeleton, endEffectorIndex);
        float dx = targetPosition.x - currentPos.x;
        float dy = targetPosition.y - currentPos.y;
        float distance = std::sqrt(dx * dx + dy * dy);
        
        if (distance < tolerance) {
            break;
        }
        
        Bone* bone = skeleton->getBone(endEffectorIndex);
        int currentIndex = endEffectorIndex;
        
        while (currentIndex != -1 && bone) {
            rotateBoneTowards(skeleton, currentIndex, targetPosition);
            currentIndex = bone->parentIndex;
            if (currentIndex != -1) {
                bone = skeleton->getBone(currentIndex);
            }
        }
    }
}

void InverseKinematics::solveTwoBoneIK(Skeleton* skeleton, int rootIndex, 
                                       int midIndex, int endIndex,
                                       const Math::Vector2D& targetPosition) {
    if (!skeleton) return;
    
    // Two-bone IK (useful for legs, arms)
    // Implementation would involve geometric solution
    solve(skeleton, endIndex, targetPosition, maxIterations);
}

Math::Vector2D InverseKinematics::getEndEffectorPosition(Skeleton* skeleton, int boneIndex) {
    Bone* bone = skeleton->getBone(boneIndex);
    if (!bone) return Math::Vector2D(0, 0);
    
    return Math::Vector2D(
        bone->worldTransform.m[12],
        bone->worldTransform.m[13]
    );
}

void InverseKinematics::rotateBoneTowards(Skeleton* skeleton, int boneIndex,
                                         const Math::Vector2D& target) {
    // Simplified rotation towards target
    // Full implementation would involve quaternion rotations
}

// SkeletalMesh implementation
SkeletalMesh::SkeletalMesh() {}

SkeletalMesh::~SkeletalMesh() {}

void SkeletalMesh::setSkeleton(std::shared_ptr<Skeleton> skeleton) {
    this->skeleton = skeleton;
}

void SkeletalMesh::setAnimator(std::shared_ptr<SkeletalAnimator> animator) {
    this->animator = animator;
}

void SkeletalMesh::update(double deltaTime) {
    if (animator) {
        animator->update(deltaTime);
    }
    
    applyBoneTransforms();
}

void SkeletalMesh::render() {
    // Rendering would be done by the graphics system
}

void SkeletalMesh::addBoneWeight(int vertexIndex, int boneIndex, float weight) {
    if (vertexIndex >= static_cast<int>(boneWeights.size())) {
        boneWeights.resize(vertexIndex + 1);
    }
    
    VertexWeight vw;
    vw.boneIndex = boneIndex;
    vw.weight = weight;
    boneWeights[vertexIndex].push_back(vw);
}

void SkeletalMesh::applyBoneTransforms() {
    if (!skeleton) return;
    
    // Apply bone transforms to vertices
    // This would involve GPU skinning in a real implementation
}

} // namespace Animation
} // namespace JJM
