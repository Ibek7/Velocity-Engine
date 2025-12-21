#ifndef JJM_QUEST_SYSTEM_H
#define JJM_QUEST_SYSTEM_H

#include <string>
#include <vector>
#include <map>
#include <functional>

namespace JJM {
namespace Gameplay {

enum class QuestStatus { NotStarted, Active, Completed, Failed };

struct QuestObjective {
    std::string description;
    int currentProgress;
    int targetProgress;
    bool completed;
    
    float getProgressPercent() const {
        return targetProgress > 0 ? (float)currentProgress / targetProgress : 0.0f;
    }
};

struct Quest {
    std::string id;
    std::string title;
    std::string description;
    QuestStatus status;
    std::vector<QuestObjective> objectives;
    std::map<std::string, int> rewards;
    
    std::function<void()> onStart;
    std::function<void()> onComplete;
    std::function<void()> onFail;
};

class QuestSystem {
public:
    static QuestSystem& getInstance();
    
    void registerQuest(const Quest& quest);
    void startQuest(const std::string& questId);
    void completeQuest(const std::string& questId);
    void failQuest(const std::string& questId);
    
    void updateObjectiveProgress(const std::string& questId, int objectiveIndex, int progress);
    void completeObjective(const std::string& questId, int objectiveIndex);
    
    Quest* getQuest(const std::string& questId);
    std::vector<Quest*> getActiveQuests();
    std::vector<Quest*> getCompletedQuests();
    
    bool isQuestActive(const std::string& questId) const;
    bool isQuestCompleted(const std::string& questId) const;

private:
    QuestSystem();
    ~QuestSystem();
    
    void checkQuestCompletion(Quest& quest);
    
    std::map<std::string, Quest> quests;
};

} // namespace Gameplay
} // namespace JJM

#endif
