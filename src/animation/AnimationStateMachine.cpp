#include "animation/AnimationStateMachine.h"
#include <algorithm>

namespace JJM {
namespace Animation {

// FloatCondition implementation
FloatCondition::FloatCondition(const std::string& paramName, Operator op, float value)
    : paramName(paramName), op(op), value(value) {
}

bool FloatCondition::evaluate() const {
    // Stub - would check actual parameter value
    return true;
}

// BoolCondition implementation
BoolCondition::BoolCondition(const std::string& paramName, bool value)
    : paramName(paramName), value(value) {
}

bool BoolCondition::evaluate() const {
    // Stub - would check actual parameter value
    return true;
}

// TriggerCondition implementation
TriggerCondition::TriggerCondition(const std::string& triggerName)
    : triggerName(triggerName) {
}

bool TriggerCondition::evaluate() const {
    // Stub - would check if trigger is set
    return true;
}

// AnimationTransition implementation
AnimationTransition::AnimationTransition(AnimationState* from, AnimationState* to)
    : fromState(from), toState(to), duration(0.2f), offset(0.0f) {
}

AnimationTransition::~AnimationTransition() {
}

void AnimationTransition::addCondition(std::shared_ptr<TransitionCondition> condition) {
    conditions.push_back(condition);
}

bool AnimationTransition::canTransition() const {
    if (conditions.empty()) return true;
    
    for (const auto& condition : conditions) {
        if (!condition->evaluate()) {
            return false;
        }
    }
    return true;
}

void AnimationTransition::setDuration(float duration) {
    this->duration = duration;
}

float AnimationTransition::getDuration() const {
    return duration;
}

void AnimationTransition::setOffset(float offset) {
    this->offset = offset;
}

float AnimationTransition::getOffset() const {
    return offset;
}

AnimationState* AnimationTransition::getFromState() const {
    return fromState;
}

AnimationState* AnimationTransition::getToState() const {
    return toState;
}

// AnimationState implementation
AnimationState::AnimationState(const std::string& name)
    : name(name), speed(1.0f), loop(true) {
}

AnimationState::~AnimationState() {
}

void AnimationState::update(float deltaTime) {
    onUpdate(deltaTime);
}

void AnimationState::setAnimation(const std::string& animationName) {
    this->animationName = animationName;
}

std::string AnimationState::getAnimation() const {
    return animationName;
}

void AnimationState::setSpeed(float speed) {
    this->speed = speed;
}

float AnimationState::getSpeed() const {
    return speed;
}

void AnimationState::setLoop(bool loop) {
    this->loop = loop;
}

bool AnimationState::isLoop() const {
    return loop;
}

void AnimationState::addTransition(std::shared_ptr<AnimationTransition> transition) {
    transitions.push_back(transition);
}

std::vector<std::shared_ptr<AnimationTransition>> AnimationState::getTransitions() const {
    return transitions;
}

std::string AnimationState::getName() const {
    return name;
}

void AnimationState::onEnter() {
    // Stub - would be called when entering state
}

void AnimationState::onExit() {
    // Stub - would be called when exiting state
}

void AnimationState::onUpdate(float deltaTime) {
    // Stub - would update animation
    (void)deltaTime;
}

// AnimationLayer implementation
AnimationLayer::AnimationLayer(const std::string& name)
    : name(name), weight(1.0f), blendMode(0) {
}

AnimationLayer::~AnimationLayer() {
}

void AnimationLayer::setWeight(float weight) {
    this->weight = weight;
}

float AnimationLayer::getWeight() const {
    return weight;
}

void AnimationLayer::setBlendMode(int mode) {
    blendMode = mode;
}

int AnimationLayer::getBlendMode() const {
    return blendMode;
}

std::string AnimationLayer::getName() const {
    return name;
}

// AnimationStateMachine implementation
AnimationStateMachine::AnimationStateMachine()
    : currentState(nullptr), defaultState(nullptr) {
}

AnimationStateMachine::~AnimationStateMachine() {
}

void AnimationStateMachine::update(float deltaTime) {
    if (!currentState) return;
    
    checkTransitions();
    currentState->update(deltaTime);
}

std::shared_ptr<AnimationState> AnimationStateMachine::createState(const std::string& name) {
    auto state = std::make_shared<AnimationState>(name);
    states[name] = state;
    return state;
}

void AnimationStateMachine::removeState(const std::string& name) {
    states.erase(name);
}

std::shared_ptr<AnimationState> AnimationStateMachine::getState(const std::string& name) {
    auto it = states.find(name);
    return it != states.end() ? it->second : nullptr;
}

void AnimationStateMachine::setDefaultState(const std::string& name) {
    auto state = getState(name);
    if (state) {
        defaultState = state.get();
        if (!currentState) {
            currentState = defaultState;
        }
    }
}

void AnimationStateMachine::setState(const std::string& name) {
    auto state = getState(name);
    if (state) {
        if (currentState) {
            currentState->onExit();
        }
        currentState = state.get();
        currentState->onEnter();
    }
}

AnimationState* AnimationStateMachine::getCurrentState() const {
    return currentState;
}

std::shared_ptr<AnimationTransition> AnimationStateMachine::createTransition(
    const std::string& from, const std::string& to) {
    
    auto fromState = getState(from);
    auto toState = getState(to);
    
    if (fromState && toState) {
        auto transition = std::make_shared<AnimationTransition>(
            fromState.get(), toState.get());
        fromState->addTransition(transition);
        return transition;
    }
    
    return nullptr;
}

void AnimationStateMachine::setParameter(const std::string& name, float value) {
    parameters[name].type = ParameterType::Float;
    parameters[name].floatValue = value;
}

void AnimationStateMachine::setParameter(const std::string& name, int value) {
    parameters[name].type = ParameterType::Int;
    parameters[name].intValue = value;
}

void AnimationStateMachine::setParameter(const std::string& name, bool value) {
    parameters[name].type = ParameterType::Bool;
    parameters[name].boolValue = value;
}

void AnimationStateMachine::setTrigger(const std::string& name) {
    parameters[name].type = ParameterType::Trigger;
    parameters[name].triggered = true;
}

void AnimationStateMachine::resetTrigger(const std::string& name) {
    auto it = parameters.find(name);
    if (it != parameters.end() && it->second.type == ParameterType::Trigger) {
        it->second.triggered = false;
    }
}

float AnimationStateMachine::getFloatParameter(const std::string& name) const {
    auto it = parameters.find(name);
    return it != parameters.end() ? it->second.floatValue : 0.0f;
}

int AnimationStateMachine::getIntParameter(const std::string& name) const {
    auto it = parameters.find(name);
    return it != parameters.end() ? it->second.intValue : 0;
}

bool AnimationStateMachine::getBoolParameter(const std::string& name) const {
    auto it = parameters.find(name);
    return it != parameters.end() ? it->second.boolValue : false;
}

bool AnimationStateMachine::isTriggerSet(const std::string& name) const {
    auto it = parameters.find(name);
    return it != parameters.end() ? it->second.triggered : false;
}

void AnimationStateMachine::addLayer(std::shared_ptr<AnimationLayer> layer) {
    layers[layer->getName()] = layer;
}

void AnimationStateMachine::removeLayer(const std::string& name) {
    layers.erase(name);
}

std::shared_ptr<AnimationLayer> AnimationStateMachine::getLayer(const std::string& name) {
    auto it = layers.find(name);
    return it != layers.end() ? it->second : nullptr;
}

void AnimationStateMachine::checkTransitions() {
    if (!currentState) return;
    
    auto transitions = currentState->getTransitions();
    for (const auto& transition : transitions) {
        if (transition->canTransition()) {
            setState(transition->getToState()->getName());
            break;
        }
    }
}

// BlendSpace1D implementation
BlendSpace1D::BlendSpace1D(const std::string& parameterName)
    : parameterName(parameterName) {
}

BlendSpace1D::~BlendSpace1D() {
}

void BlendSpace1D::update(float deltaTime) {
    (void)deltaTime;
    // Stub - would update blend weights
}

float BlendSpace1D::getBlendWeight() const {
    return 1.0f;
}

void BlendSpace1D::addAnimation(const std::string& name, float position) {
    points.push_back({name, position, 0.0f});
}

void BlendSpace1D::removeAnimation(const std::string& name) {
    points.erase(
        std::remove_if(points.begin(), points.end(),
            [&name](const BlendPoint& p) { return p.animation == name; }),
        points.end()
    );
}

// BlendSpace2D implementation
BlendSpace2D::BlendSpace2D(const std::string& paramX, const std::string& paramY)
    : parameterX(paramX), parameterY(paramY) {
}

BlendSpace2D::~BlendSpace2D() {
}

void BlendSpace2D::update(float deltaTime) {
    (void)deltaTime;
    // Stub - would update blend weights
}

float BlendSpace2D::getBlendWeight() const {
    return 1.0f;
}

void BlendSpace2D::addAnimation(const std::string& name, float x, float y) {
    points.push_back({name, x, y, 0.0f});
}

void BlendSpace2D::removeAnimation(const std::string& name) {
    points.erase(
        std::remove_if(points.begin(), points.end(),
            [&name](const BlendPoint2D& p) { return p.animation == name; }),
        points.end()
    );
}

// StateMachineBuilder implementation
StateMachineBuilder::StateMachineBuilder() {
    stateMachine = std::make_shared<AnimationStateMachine>();
}

StateMachineBuilder::~StateMachineBuilder() {
}

StateMachineBuilder& StateMachineBuilder::addState(const std::string& name,
                                                   const std::string& animation) {
    auto state = stateMachine->createState(name);
    state->setAnimation(animation);
    return *this;
}

StateMachineBuilder& StateMachineBuilder::addTransition(const std::string& from,
                                                        const std::string& to) {
    stateMachine->createTransition(from, to);
    return *this;
}

StateMachineBuilder& StateMachineBuilder::addCondition(const std::string& param, float value) {
    // Stub - would add condition to last transition
    (void)param;
    (void)value;
    return *this;
}

StateMachineBuilder& StateMachineBuilder::setDefaultState(const std::string& name) {
    stateMachine->setDefaultState(name);
    return *this;
}

std::shared_ptr<AnimationStateMachine> StateMachineBuilder::build() {
    return stateMachine;
}

} // namespace Animation
} // namespace JJM
