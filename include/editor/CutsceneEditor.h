#pragma once

#include <string>
#include <vector>
#include <memory>
#include <unordered_map>

// Cutscene editor and playback system
namespace Engine {

enum class CutsceneActionType {
    CameraMove,
    CameraLookAt,
    ShowDialog,
    PlayAnimation,
    PlaySound,
    SpawnObject,
    TriggerEvent,
    Wait
};

struct CutsceneAction {
    CutsceneActionType type;
    float timestamp;
    float duration;
    std::string targetId;
    float parameters[6];
    std::string stringParam;
};

class Cutscene {
public:
    Cutscene(const std::string& name);
    ~Cutscene();

    // Actions
    void addAction(const CutsceneAction& action);
    void removeAction(int index);
    void clearActions();
    
    // Playback
    void play();
    void pause();
    void stop();
    void update(float deltaTime);
    void seek(float time);
    
    // State
    bool isPlaying() const { return m_playing; }
    bool isPaused() const { return m_paused; }
    float getCurrentTime() const { return m_currentTime; }
    float getDuration() const;
    
    // Properties
    const std::string& getName() const { return m_name; }
    int getActionCount() const { return m_actions.size(); }
    const CutsceneAction* getAction(int index) const;

private:
    void executeAction(const CutsceneAction& action);

    std::string m_name;
    std::vector<CutsceneAction> m_actions;
    float m_currentTime;
    bool m_playing;
    bool m_paused;
    int m_currentActionIndex;
};

class CutsceneEditor {
public:
    static CutsceneEditor& getInstance();

    // Cutscene management
    Cutscene* createCutscene(const std::string& name);
    void deleteCutscene(const std::string& name);
    Cutscene* getCutscene(const std::string& name);
    
    // Playback
    void playCutscene(const std::string& name);
    void stopCurrentCutscene();
    void update(float deltaTime);
    
    // Save/Load
    bool saveCutscene(const std::string& name, const std::string& filePath);
    bool loadCutscene(const std::string& filePath);
    
    // Query
    Cutscene* getCurrentCutscene() const { return m_currentCutscene; }
    bool isCutscenePlaying() const;
    std::vector<std::string> getAllCutsceneNames() const;

private:
    CutsceneEditor();
    CutsceneEditor(const CutsceneEditor&) = delete;
    CutsceneEditor& operator=(const CutsceneEditor&) = delete;

    std::unordered_map<std::string, std::unique_ptr<Cutscene>> m_cutscenes;
    Cutscene* m_currentCutscene;
};

} // namespace Engine
