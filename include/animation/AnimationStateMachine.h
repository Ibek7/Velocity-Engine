#ifndef JJM_ANIMATION_STATE_MACHINE_H
#define JJM_ANIMATION_STATE_MACHINE_H

#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <functional>

namespace JJM {
namespace Animation {

class AnimationStateMachine;
class AnimationState;

/**
 * @brief Animation transition condition
 */
class TransitionCondition {
public:
    virtual ~TransitionCondition() {}
    virtual bool evaluate() const = 0;
};

/**
 * @brief Float parameter condition
 */
class FloatCondition : public TransitionCondition {
public:
    enum class Operator { Less, Greater, Equal, NotEqual };
    
    FloatCondition(const std::string& paramName, Operator op, float value);
    bool evaluate() const override;

private:
    std::string paramName;
    Operator op;
    float value;
};

/**
 * @brief Bool parameter condition
 */
class BoolCondition : public TransitionCondition {
public:
    BoolCondition(const std::string& paramName, bool value);
    bool evaluate() const override;

private:
    std::string paramName;
    bool value;
};

/**
 * @brief Trigger condition
 */
class TriggerCondition : public TransitionCondition {
public:
    TriggerCondition(const std::string& triggerName);
    bool evaluate() const override;

private:
    std::string triggerName;
};

/**
 * @brief Animation state transition
 */
class AnimationTransition {
public:
    AnimationTransition(AnimationState* from, AnimationState* to);
    ~AnimationTransition();

    void addCondition(std::shared_ptr<TransitionCondition> condition);
    bool canTransition() const;
    
    void setDuration(float duration);
    float getDuration() const;
    
    void setOffset(float offset);
    float getOffset() const;
    
    AnimationState* getFromState() const;
    AnimationState* getToState() const;

private:
    AnimationState* fromState;
    AnimationState* toState;
    std::vector<std::shared_ptr<TransitionCondition>> conditions;
    float duration;
    float offset;
};

/**
 * @brief Animation state
 */
class AnimationState {
public:
    AnimationState(const std::string& name);
    ~AnimationState();

    void update(float deltaTime);
    
    void setAnimation(const std::string& animationName);
    std::string getAnimation() const;
    
    void setSpeed(float speed);
    float getSpeed() const;
    
    void setLoop(bool loop);
    bool isLoop() const;
    
    void addTransition(std::shared_ptr<AnimationTransition> transition);
    std::vector<std::shared_ptr<AnimationTransition>> getTransitions() const;
    
    std::string getName() const;
    
    void onEnter();
    void onExit();
    void onUpdate(float deltaTime);

private:
    std::string name;
    std::string animationName;
    float speed;
    bool loop;
    std::vector<std::shared_ptr<AnimationTransition>> transitions;
};

/**
 * @brief Animation parameter types
 */
enum class ParameterType {
    Float,
    Int,
    Bool,
    Trigger
};

/**
 * @brief Animation parameter
 */
struct AnimationParameter {
    std::string name;
    ParameterType type;
    float floatValue;
    int intValue;
    bool boolValue;
    bool triggered;
};

/**
 * @brief Animation layer for blending
 */
class AnimationLayer {
public:
    AnimationLayer(const std::string& name);
    ~AnimationLayer();

    void setWeight(float weight);
    float getWeight() const;
    
    void setBlendMode(int mode);
    int getBlendMode() const;
    
    std::string getName() const;

private:
    std::string name;
    float weight;
    int blendMode;
};

/**
 * @brief Animation state machine
 */
class AnimationStateMachine {
public:
    AnimationStateMachine();
    ~AnimationStateMachine();

    void update(float deltaTime);
    
    std::shared_ptr<AnimationState> createState(const std::string& name);
    void removeState(const std::string& name);
    std::shared_ptr<AnimationState> getState(const std::string& name);
    
    void setDefaultState(const std::string& name);
    void setState(const std::string& name);
    AnimationState* getCurrentState() const;
    
    std::shared_ptr<AnimationTransition> createTransition(
        const std::string& from, const std::string& to);
    
    void setParameter(const std::string& name, float value);
    void setParameter(const std::string& name, int value);
    void setParameter(const std::string& name, bool value);
    void setTrigger(const std::string& name);
    void resetTrigger(const std::string& name);
    
    float getFloatParameter(const std::string& name) const;
    int getIntParameter(const std::string& name) const;
    bool getBoolParameter(const std::string& name) const;
    bool isTriggerSet(const std::string& name) const;
    
    void addLayer(std::shared_ptr<AnimationLayer> layer);
    void removeLayer(const std::string& name);
    std::shared_ptr<AnimationLayer> getLayer(const std::string& name);

private:
    std::unordered_map<std::string, std::shared_ptr<AnimationState>> states;
    std::unordered_map<std::string, AnimationParameter> parameters;
    std::unordered_map<std::string, std::shared_ptr<AnimationLayer>> layers;
    AnimationState* currentState;
    AnimationState* defaultState;
    
    void checkTransitions();
};

/**
 * @brief Animation blend tree node
 */
class BlendTreeNode {
public:
    virtual ~BlendTreeNode() {}
    virtual void update(float deltaTime) = 0;
    virtual float getBlendWeight() const = 0;
};

/**
 * @brief 1D blend space
 */
class BlendSpace1D : public BlendTreeNode {
public:
    BlendSpace1D(const std::string& parameterName);
    ~BlendSpace1D();

    void update(float deltaTime) override;
    float getBlendWeight() const override;
    
    void addAnimation(const std::string& name, float position);
    void removeAnimation(const std::string& name);

private:
    std::string parameterName;
    struct BlendPoint {
        std::string animation;
        float position;
        float weight;
    };
    std::vector<BlendPoint> points;
};

/**
 * @brief 2D blend space
 */
class BlendSpace2D : public BlendTreeNode {
public:
    BlendSpace2D(const std::string& paramX, const std::string& paramY);
    ~BlendSpace2D();

    void update(float deltaTime) override;
    float getBlendWeight() const override;
    
    void addAnimation(const std::string& name, float x, float y);
    void removeAnimation(const std::string& name);

private:
    std::string parameterX;
    std::string parameterY;
    struct BlendPoint2D {
        std::string animation;
        float x, y;
        float weight;
    };
    std::vector<BlendPoint2D> points;
};

/**
 * @brief Animation state machine builder
 */
class StateMachineBuilder {
public:
    StateMachineBuilder();
    ~StateMachineBuilder();

    StateMachineBuilder& addState(const std::string& name, const std::string& animation);
    StateMachineBuilder& addTransition(const std::string& from, const std::string& to);
    StateMachineBuilder& addCondition(const std::string& param, float value);
    StateMachineBuilder& setDefaultState(const std::string& name);
    
    std::shared_ptr<AnimationStateMachine> build();

private:
    std::shared_ptr<AnimationStateMachine> stateMachine;
};

} // namespace Animation
} // namespace JJM

#endif // JJM_ANIMATION_STATE_MACHINE_H
