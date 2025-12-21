#include "gameplay/QuestSystem.h"
#include <iostream>

namespace JJM {
namespace Gameplay {

QuestSystem& QuestSystem::getInstance() {
    static QuestSystem instance;
    return instance;
}

QuestSystem::QuestSystem() {
}

QuestSystem::~QuestSystem() {
}

void QuestSystem::registerQuest(const Quest& quest) {
    quests[quest.id] = quest;
}

void QuestSystem::startQuest(const std::string& questId) {
    auto it = quests.find(questId);
    if (it == quests.end()) return;
    
    Quest& quest = it->second;
    if (quest.status != QuestStatus::NotStarted) return;
    
    quest.status = QuestStatus::Active;
    
    if (quest.onStart) {
        quest.onStart();
    }
    
    std::cout << "Quest started: " << quest.title << std::endl;
}

void QuestSystem::completeQuest(const std::string& questId) {
    auto it = quests.find(questId);
    if (it == quests.end()) return;
    
    Quest& quest = it->second;
    if (quest.status != QuestStatus::Active) return;
    
    quest.status = QuestStatus::Completed;
    
    if (quest.onComplete) {
        quest.onComplete();
    }
    
    std::cout << "Quest completed: " << quest.title << std::endl;
}

void QuestSystem::failQuest(const std::string& questId) {
    auto it = quests.find(questId);
    if (it == quests.end()) return;
    
    Quest& quest = it->second;
    if (quest.status != QuestStatus::Active) return;
    
    quest.status = QuestStatus::Failed;
    
    if (quest.onFail) {
        quest.onFail();
    }
    
    std::cout << "Quest failed: " << quest.title << std::endl;
}

void QuestSystem::updateObjectiveProgress(const std::string& questId, int objectiveIndex, int progress) {
    auto it = quests.find(questId);
    if (it == quests.end()) return;
    
    Quest& quest = it->second;
    if (quest.status != QuestStatus::Active) return;
    
    if (objectiveIndex >= 0 && objectiveIndex < static_cast<int>(quest.objectives.size())) {
        QuestObjective& obj = quest.objectives[objectiveIndex];
        obj.currentProgress = progress;
        
        if (obj.currentProgress >= obj.targetProgress) {
            obj.completed = true;
        }
        
        checkQuestCompletion(quest);
    }
}

void QuestSystem::completeObjective(const std::string& questId, int objectiveIndex) {
    auto it = quests.find(questId);
    if (it == quests.end()) return;
    
    Quest& quest = it->second;
    if (objectiveIndex >= 0 && objectiveIndex < static_cast<int>(quest.objectives.size())) {
        quest.objectives[objectiveIndex].completed = true;
        quest.objectives[objectiveIndex].currentProgress = quest.objectives[objectiveIndex].targetProgress;
        
        checkQuestCompletion(quest);
    }
}

Quest* QuestSystem::getQuest(const std::string& questId) {
    auto it = quests.find(questId);
    return it != quests.end() ? &it->second : nullptr;
}

std::vector<Quest*> QuestSystem::getActiveQuests() {
    std::vector<Quest*> result;
    for (auto& pair : quests) {
        if (pair.second.status == QuestStatus::Active) {
            result.push_back(&pair.second);
        }
    }
    return result;
}

std::vector<Quest*> QuestSystem::getCompletedQuests() {
    std::vector<Quest*> result;
    for (auto& pair : quests) {
        if (pair.second.status == QuestStatus::Completed) {
            result.push_back(&pair.second);
        }
    }
    return result;
}

bool QuestSystem::isQuestActive(const std::string& questId) const {
    auto it = quests.find(questId);
    return it != quests.end() && it->second.status == QuestStatus::Active;
}

bool QuestSystem::isQuestCompleted(const std::string& questId) const {
    auto it = quests.find(questId);
    return it != quests.end() && it->second.status == QuestStatus::Completed;
}

void QuestSystem::checkQuestCompletion(Quest& quest) {
    bool allCompleted = true;
    for (const auto& obj : quest.objectives) {
        if (!obj.completed) {
            allCompleted = false;
            break;
        }
    }
    
    if (allCompleted) {
        completeQuest(quest.id);
    }
}

} // namespace Gameplay
} // namespace JJM
