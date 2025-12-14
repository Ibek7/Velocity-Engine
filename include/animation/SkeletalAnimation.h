#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include "math/Vector2D.h"

namespace JJM {
namespace Animation {

struct Matrix4 {
    float m[16];
    Matrix4() { for (int i = 0; i < 16; ++i) m[i] = (i % 5 == 0) ? 1.0f : 0.0f; }
    static Matrix4 translation(const Math::Vector2D& pos) {
        Matrix4 mat;
        mat.m[12] = pos.x; mat.m[13] = pos.y;
        return mat;
    }
    static Matrix4 scale(const Math::Vector2D& s) {
        Matrix4 mat;
        mat.m[0] = s.x; mat.m[5] = s.y;
        return mat;
    }
    Matrix4 operator*(const Matrix4& other) const {
        Matrix4 result;
        for (int i = 0; i < 4; ++i) {
            for (int j = 0; j < 4; ++j) {
                result.m[i * 4 + j] = 0;
                for (int k = 0; k < 4; ++k) {
                    result.m[i * 4 + j] += m[i * 4 + k] * other.m[k * 4 + j];
                }
            }
        }
        return result;
    }
};

struct Quaternion {
    float x, y, z, w;
    Quaternion() : x(0), y(0), z(0), w(1) {}
    Quaternion(float x, float y, float z, float w) : x(x), y(y), z(z), w(w) {}
    
    static Quaternion slerp(const Quaternion& a, const Quaternion& b, float t) {
        return Quaternion(
            a.x + (b.x - a.x) * t,
            a.y + (b.y - a.y) * t,
            a.z + (b.z - a.z) * t,
            a.w + (b.w - a.w) * t
        );
    }
    
    Matrix4 toMatrix() const {
        Matrix4 mat;
        mat.m[0] = 1 - 2*y*y - 2*z*z;
        mat.m[1] = 2*x*y - 2*w*z;
        mat.m[2] = 2*x*z + 2*w*y;
        mat.m[4] = 2*x*y + 2*w*z;
        mat.m[5] = 1 - 2*x*x - 2*z*z;
        mat.m[6] = 2*y*z - 2*w*x;
        mat.m[8] = 2*x*z - 2*w*y;
        mat.m[9] = 2*y*z + 2*w*x;
        mat.m[10] = 1 - 2*x*x - 2*y*y;
        return mat;
    }
};

struct Bone {
    std::string name;
    int parentIndex;
    Matrix4 offsetMatrix;
    Matrix4 localTransform;
    Matrix4 worldTransform;
    
    Bone() : name(""), parentIndex(-1) {}
    Bone(const std::string& name, int parent = -1) 
        : name(name), parentIndex(parent) {}
};

struct BoneKeyframe {
    double time;
    Math::Vector2D position;
    Quaternion rotation;
    Math::Vector2D scale;
    
    BoneKeyframe() : time(0.0), scale(1.0, 1.0) {}
};

struct BoneAnimation {
    std::string boneName;
    std::vector<BoneKeyframe> keyframes;
    
    BoneAnimation() : boneName("") {}
    explicit BoneAnimation(const std::string& name) : boneName(name) {}
    
    BoneKeyframe interpolate(double time) const;
    
    void addKeyframe(double time, const Math::Vector2D& pos,
                    const Quaternion& rot, const Math::Vector2D& scale);
};

class AnimationClip {
public:
    AnimationClip();
    explicit AnimationClip(const std::string& name);
    ~AnimationClip();
    
    void addBoneAnimation(const std::string& boneName);
    BoneAnimation* getBoneAnimation(const std::string& boneName);
    
    void setDuration(double duration) { this->duration = duration; }
    double getDuration() const { return duration; }
    
    void setName(const std::string& name) { this->name = name; }
    const std::string& getName() const { return name; }
    
    void setLooping(bool loop) { this->looping = loop; }
    bool isLooping() const { return looping; }
    
    const std::vector<BoneAnimation>& getBoneAnimations() const { 
        return boneAnimations; 
    }

private:
    std::string name;
    double duration;
    bool looping;
    std::vector<BoneAnimation> boneAnimations;
};

class Skeleton {
public:
    Skeleton();
    ~Skeleton();
    
    int addBone(const std::string& name, int parentIndex = -1);
    Bone* getBone(int index);
    Bone* getBone(const std::string& name);
    int getBoneIndex(const std::string& name) const;
    
    size_t getBoneCount() const { return bones.size(); }
    const std::vector<Bone>& getBones() const { return bones; }
    
    void updateBoneTransforms();
    void updateBoneTransforms(const std::vector<Matrix4>& boneTransforms);
    
    const std::vector<Matrix4>& getFinalTransforms() const {
        return finalTransforms;
    }

private:
    std::vector<Bone> bones;
    std::unordered_map<std::string, int> boneNameToIndex;
    std::vector<Matrix4> finalTransforms;
    
    void updateBoneHierarchy(int boneIndex, const Matrix4& parentTransform);
};

class SkeletalAnimator {
public:
    SkeletalAnimator();
    explicit SkeletalAnimator(std::shared_ptr<Skeleton> skeleton);
    ~SkeletalAnimator();
    
    void setSkeleton(std::shared_ptr<Skeleton> skeleton);
    std::shared_ptr<Skeleton> getSkeleton() const { return skeleton; }
    
    void addAnimationClip(std::shared_ptr<AnimationClip> clip);
    void removeAnimationClip(const std::string& name);
    AnimationClip* getAnimationClip(const std::string& name);
    
    void play(const std::string& clipName, bool restart = false);
    void stop();
    void pause();
    void resume();
    
    void setSpeed(float speed) { this->playbackSpeed = speed; }
    float getSpeed() const { return playbackSpeed; }
    
    void setTime(double time);
    double getTime() const { return currentTime; }
    
    bool isPlaying() const { return playing; }
    bool isPaused() const { return paused; }
    
    void update(double deltaTime);
    
    const std::string& getCurrentClipName() const { return currentClipName; }

private:
    std::shared_ptr<Skeleton> skeleton;
    std::unordered_map<std::string, std::shared_ptr<AnimationClip>> animationClips;
    
    std::string currentClipName;
    AnimationClip* currentClip;
    double currentTime;
    float playbackSpeed;
    bool playing;
    bool paused;
    
    void applyAnimation();
    void calculateBoneTransforms(std::vector<Matrix4>& transforms);
};

class AnimationBlendTree {
public:
    AnimationBlendTree();
    ~AnimationBlendTree();
    
    void addAnimation(const std::string& name, std::shared_ptr<AnimationClip> clip);
    void setBlendWeight(const std::string& name, float weight);
    
    void update(double deltaTime, Skeleton* skeleton);
    
    void clear();

private:
    struct BlendNode {
        std::shared_ptr<AnimationClip> clip;
        float weight;
        double time;
        
        BlendNode() : weight(0.0f), time(0.0) {}
    };
    
    std::unordered_map<std::string, BlendNode> blendNodes;
    
    void normalizeWeights();
};

class InverseKinematics {
public:
    InverseKinematics();
    ~InverseKinematics();
    
    void solve(Skeleton* skeleton, int endEffectorIndex, 
               const Math::Vector2D& targetPosition, int iterations = 10);
    
    void solveTwoBoneIK(Skeleton* skeleton, int rootIndex, int midIndex, 
                       int endIndex, const Math::Vector2D& targetPosition);
    
    void setMaxIterations(int iterations) { maxIterations = iterations; }
    int getMaxIterations() const { return maxIterations; }
    
    void setTolerance(float tolerance) { this->tolerance = tolerance; }
    float getTolerance() const { return tolerance; }

private:
    int maxIterations;
    float tolerance;
    
    Math::Vector2D getEndEffectorPosition(Skeleton* skeleton, int boneIndex);
    void rotateBoneTowards(Skeleton* skeleton, int boneIndex, 
                          const Math::Vector2D& target);
};

class SkeletalMesh {
public:
    SkeletalMesh();
    ~SkeletalMesh();
    
    void setSkeleton(std::shared_ptr<Skeleton> skeleton);
    std::shared_ptr<Skeleton> getSkeleton() const { return skeleton; }
    
    void setAnimator(std::shared_ptr<SkeletalAnimator> animator);
    std::shared_ptr<SkeletalAnimator> getAnimator() const { return animator; }
    
    void update(double deltaTime);
    void render();
    
    void addBoneWeight(int vertexIndex, int boneIndex, float weight);
    
    struct VertexWeight {
        int boneIndex;
        float weight;
    };
    
    const std::vector<std::vector<VertexWeight>>& getBoneWeights() const {
        return boneWeights;
    }

private:
    std::shared_ptr<Skeleton> skeleton;
    std::shared_ptr<SkeletalAnimator> animator;
    std::vector<std::vector<VertexWeight>> boneWeights;
    
    void applyBoneTransforms();
};

} // namespace Animation
} // namespace JJM
