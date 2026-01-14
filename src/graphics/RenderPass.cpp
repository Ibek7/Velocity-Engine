#include "graphics/RenderPass.h"
#include <algorithm>
#include <iostream>

namespace JJM {
namespace Graphics {

// RenderPass Implementation
RenderPass::RenderPass(const std::string& name)
    : m_name(name)
    , m_enabled(true)
    , m_framebuffer(0) {
}

RenderPass::~RenderPass() {
}

// RenderPassManager Implementation
RenderPassManager::RenderPassManager()
    : m_totalTime(0.0f) {
}

RenderPassManager::~RenderPassManager() {
    m_passes.clear();
}

void RenderPassManager::addPass(std::unique_ptr<RenderPass> pass) {
    if (pass) {
        std::string name = pass->getName();
        m_passes.push_back(std::move(pass));
        m_passOrder.push_back(name);
    }
}

void RenderPassManager::removePass(const std::string& name) {
    // Remove from execution order
    auto orderIt = std::find(m_passOrder.begin(), m_passOrder.end(), name);
    if (orderIt != m_passOrder.end()) {
        m_passOrder.erase(orderIt);
    }
    
    // Remove the pass itself
    auto passIt = std::find_if(m_passes.begin(), m_passes.end(),
        [&name](const std::unique_ptr<RenderPass>& pass) {
            return pass->getName() == name;
        });
    
    if (passIt != m_passes.end()) {
        m_passes.erase(passIt);
    }
}

RenderPass* RenderPassManager::getPass(const std::string& name) {
    auto it = std::find_if(m_passes.begin(), m_passes.end(),
        [&name](const std::unique_ptr<RenderPass>& pass) {
            return pass->getName() == name;
        });
    
    return (it != m_passes.end()) ? it->get() : nullptr;
}

void RenderPassManager::executeAll() {
    m_totalTime = 0.0f;
    
    for (const auto& passName : m_passOrder) {
        RenderPass* pass = getPass(passName);
        if (pass && pass->isEnabled()) {
            // TODO: Time the execution
            pass->execute();
        }
    }
}

void RenderPassManager::executePass(const std::string& name) {
    RenderPass* pass = getPass(name);
    if (pass && pass->isEnabled()) {
        pass->execute();
    }
}

void RenderPassManager::setPassOrder(const std::vector<std::string>& order) {
    // Validate that all passes in order exist
    for (const auto& name : order) {
        if (!getPass(name)) {
            std::cerr << "Warning: Pass '" << name << "' not found in render pass manager" << std::endl;
        }
    }
    
    m_passOrder = order;
}

// LambdaRenderPass Implementation
LambdaRenderPass::LambdaRenderPass(const std::string& name, ExecuteFunc func)
    : RenderPass(name)
    , m_executeFunc(func) {
}

void LambdaRenderPass::execute() {
    if (m_executeFunc) {
        m_executeFunc();
    }
}

} // namespace Graphics
} // namespace JJM
