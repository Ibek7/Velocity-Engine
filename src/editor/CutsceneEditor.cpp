#include "editor/CutsceneEditor.h"
#include <algorithm>
#include <fstream>
#include <unordered_map>

namespace Engine {

// Cutscene implementation
Cutscene::Cutscene(const std::string& name)
    : m_name(name)
    , m_currentTime(0.0f)
    , m_playing(false)
    , m_paused(false)
    , m_currentActionIndex(0)
{
}

Cutscene::~Cutscene() {
}

void Cutscene::addAction(const CutsceneAction& action) {
    m_actions.push_back(action);
    
    // Sort by timestamp
    std::sort(m_actions.begin(), m_actions.end(),
        [](const CutsceneAction& a, const CutsceneAction& b) {
            return a.timestamp < b.timestamp;
        });
}

void Cutscene::removeAction(int index) {
    if (index >= 0 && index < static_cast<int>(m_actions.size())) {
        m_actions.erase(m_actions.begin() + index);
    }
}

void Cutscene::clearActions() {
    m_actions.clear();
}

void Cutscene::play() {
    m_playing = true;
    m_paused = false;
    m_currentTime = 0.0f;
    m_currentActionIndex = 0;
}

void Cutscene::pause() {
    m_paused = true;
}

void Cutscene::stop() {
    m_playing = false;
    m_paused = false;
    m_currentTime = 0.0f;
    m_currentActionIndex = 0;
}

void Cutscene::update(float deltaTime) {
    if (!m_playing || m_paused) {
        return;
    }
    
    m_currentTime += deltaTime;
    
    // Execute actions at current time
    while (m_currentActionIndex < static_cast<int>(m_actions.size())) {
        const auto& action = m_actions[m_currentActionIndex];
        
        if (action.timestamp <= m_currentTime) {
            executeAction(action);
            m_currentActionIndex++;
        } else {
            break;
        }
    }
    
    // Stop if we've reached the end
    if (m_currentActionIndex >= static_cast<int>(m_actions.size())) {
        stop();
    }
}

void Cutscene::seek(float time) {
    m_currentTime = time;
    
    // Find the appropriate action index
    m_currentActionIndex = 0;
    for (int i = 0; i < static_cast<int>(m_actions.size()); ++i) {
        if (m_actions[i].timestamp <= time) {
            m_currentActionIndex = i + 1;
        } else {
            break;
        }
    }
}

float Cutscene::getDuration() const {
    if (m_actions.empty()) {
        return 0.0f;
    }
    
    const auto& lastAction = m_actions.back();
    return lastAction.timestamp + lastAction.duration;
}

const CutsceneAction* Cutscene::getAction(int index) const {
    if (index >= 0 && index < static_cast<int>(m_actions.size())) {
        return &m_actions[index];
    }
    return nullptr;
}

void Cutscene::executeAction(const CutsceneAction& action) {
    switch (action.type) {
        case CutsceneActionType::CameraMove:
            // TODO: Move camera to position
            break;
            
        case CutsceneActionType::CameraLookAt:
            // TODO: Point camera at target
            break;
            
        case CutsceneActionType::ShowDialog:
            // TODO: Display dialog
            break;
            
        case CutsceneActionType::PlayAnimation:
            // TODO: Play animation on target
            break;
            
        case CutsceneActionType::PlaySound:
            // TODO: Play sound effect
            break;
            
        case CutsceneActionType::SpawnObject:
            // TODO: Spawn object at position
            break;
            
        case CutsceneActionType::TriggerEvent:
            // TODO: Trigger game event
            break;
            
        case CutsceneActionType::Wait:
            // Passive action, just wait
            break;
    }
}

// CutsceneEditor implementation
CutsceneEditor::CutsceneEditor()
    : m_currentCutscene(nullptr)
{
}

CutsceneEditor& CutsceneEditor::getInstance() {
    static CutsceneEditor instance;
    return instance;
}

Cutscene* CutsceneEditor::createCutscene(const std::string& name) {
    auto cutscene = std::make_unique<Cutscene>(name);
    Cutscene* ptr = cutscene.get();
    m_cutscenes[name] = std::move(cutscene);
    return ptr;
}

void CutsceneEditor::deleteCutscene(const std::string& name) {
    if (m_currentCutscene && m_currentCutscene->getName() == name) {
        m_currentCutscene = nullptr;
    }
    m_cutscenes.erase(name);
}

Cutscene* CutsceneEditor::getCutscene(const std::string& name) {
    auto it = m_cutscenes.find(name);
    if (it != m_cutscenes.end()) {
        return it->second.get();
    }
    return nullptr;
}

void CutsceneEditor::playCutscene(const std::string& name) {
    Cutscene* cutscene = getCutscene(name);
    if (cutscene) {
        m_currentCutscene = cutscene;
        cutscene->play();
    }
}

void CutsceneEditor::stopCurrentCutscene() {
    if (m_currentCutscene) {
        m_currentCutscene->stop();
        m_currentCutscene = nullptr;
    }
}

void CutsceneEditor::update(float deltaTime) {
    if (m_currentCutscene) {
        m_currentCutscene->update(deltaTime);
        
        if (!m_currentCutscene->isPlaying()) {
            m_currentCutscene = nullptr;
        }
    }
}

bool CutsceneEditor::saveCutscene(const std::string& name, const std::string& filePath) {
    Cutscene* cutscene = getCutscene(name);
    if (!cutscene) {
        return false;
    }
    
    std::ofstream file(filePath);
    if (!file.is_open()) {
        return false;
    }
    
    // Simple text format
    file << cutscene->getName() << "\n";
    file << cutscene->getActionCount() << "\n";
    
    for (int i = 0; i < cutscene->getActionCount(); ++i) {
        const CutsceneAction* action = cutscene->getAction(i);
        file << static_cast<int>(action->type) << " ";
        file << action->timestamp << " ";
        file << action->duration << " ";
        file << action->targetId << " ";
        for (int j = 0; j < 6; ++j) {
            file << action->parameters[j] << " ";
        }
        file << action->stringParam << "\n";
    }
    
    file.close();
    return true;
}

bool CutsceneEditor::loadCutscene(const std::string& filePath) {
    std::ifstream file(filePath);
    if (!file.is_open()) {
        return false;
    }
    
    std::string name;
    std::getline(file, name);
    
    int actionCount;
    file >> actionCount;
    file.ignore(); // Skip newline
    
    Cutscene* cutscene = createCutscene(name);
    
    for (int i = 0; i < actionCount; ++i) {
        CutsceneAction action;
        int type;
        file >> type;
        action.type = static_cast<CutsceneActionType>(type);
        file >> action.timestamp;
        file >> action.duration;
        file >> action.targetId;
        for (int j = 0; j < 6; ++j) {
            file >> action.parameters[j];
        }
        file.ignore(); // Skip space
        std::getline(file, action.stringParam);
        
        cutscene->addAction(action);
    }
    
    file.close();
    return true;
}

bool CutsceneEditor::isCutscenePlaying() const {
    return m_currentCutscene != nullptr && m_currentCutscene->isPlaying();
}

std::vector<std::string> CutsceneEditor::getAllCutsceneNames() const {
    std::vector<std::string> names;
    for (const auto& pair : m_cutscenes) {
        names.push_back(pair.first);
    }
    return names;
}

} // namespace Engine
