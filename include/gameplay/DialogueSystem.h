#ifndef JJM_DIALOGUE_SYSTEM_H
#define JJM_DIALOGUE_SYSTEM_H

#include <string>
#include <vector>
#include <map>
#include <functional>
#include <memory>
#include <optional>
#include <unordered_map>
#include <queue>
#include <variant>

namespace JJM {
namespace Gameplay {

// =============================================================================
// Dialogue Types and Enums
// =============================================================================

/**
 * @brief Speaker emotion/expression
 */
enum class DialogueEmotion {
    Neutral,
    Happy,
    Sad,
    Angry,
    Surprised,
    Fearful,
    Disgusted,
    Confused,
    Thoughtful,
    Excited,
    Custom
};

/**
 * @brief Dialogue node types
 */
enum class DialogueNodeType {
    Speech,             // Normal dialogue
    Choice,             // Player choice
    Branch,             // Conditional branch
    Action,             // Execute action
    Random,             // Random selection
    Hub,                // Return point for choices
    Start,              // Entry point
    End                 // Exit point
};

/**
 * @brief Text animation styles
 */
enum class TextAnimationStyle {
    Instant,            // Show all at once
    Typewriter,         // Character by character
    WordByWord,         // Word by word
    FadeIn,             // Fade in characters
    Wavy,               // Wavy text animation
    Shake               // Shaking text
};

/**
 * @brief Voice line info
 */
struct VoiceLine {
    std::string audioPath;
    float duration;
    std::vector<std::pair<float, std::string>> lipSyncData;
    std::string language;
    
    VoiceLine() : duration(0.0f) {}
};

/**
 * @brief Speaker portrait configuration
 */
struct SpeakerPortrait {
    std::string imagePath;
    std::string position;           // "left", "right", "center"
    DialogueEmotion emotion;
    bool highlighted;
    bool flipped;
    float scale;
    
    SpeakerPortrait()
        : position("left")
        , emotion(DialogueEmotion::Neutral)
        , highlighted(true)
        , flipped(false)
        , scale(1.0f)
    {}
};

/**
 * @brief Rich text formatting tag
 */
struct TextTag {
    std::string type;               // "color", "size", "bold", "shake", etc.
    std::string value;
    int startIndex;
    int endIndex;
};

// =============================================================================
// Dialogue Conditions and Actions
// =============================================================================

/**
 * @brief Variable types for dialogue conditions
 */
using DialogueVariable = std::variant<int, float, bool, std::string>;

/**
 * @brief Condition for dialogue branching
 */
struct DialogueCondition {
    std::string variableName;
    std::string comparison;         // "==", "!=", ">", "<", ">=", "<="
    DialogueVariable value;
    
    bool evaluate(const std::unordered_map<std::string, DialogueVariable>& variables) const;
};

/**
 * @brief Compound condition (AND/OR)
 */
struct CompoundCondition {
    std::vector<DialogueCondition> conditions;
    bool requireAll;                // true = AND, false = OR
    
    bool evaluate(const std::unordered_map<std::string, DialogueVariable>& variables) const;
};

/**
 * @brief Action to execute during dialogue
 */
struct DialogueAction {
    enum class Type {
        SetVariable,
        AddItem,
        RemoveItem,
        AddExperience,
        PlaySound,
        PlayAnimation,
        TriggerEvent,
        StartQuest,
        CompleteQuest,
        ChangeRelationship,
        Custom
    };
    
    Type type;
    std::string target;
    DialogueVariable value;
    std::function<void()> customAction;
};

// =============================================================================
// Dialogue Nodes and Options
// =============================================================================

struct DialogueOption {
    std::string text;
    std::string nextNodeId;
    std::function<bool()> condition;
    
    // Enhanced features
    std::optional<CompoundCondition> advancedCondition;
    std::vector<DialogueAction> actions;
    std::string tooltip;
    bool hidden;                    // Hidden until conditions met
    bool once;                      // Can only be selected once
    bool selected;                  // Has been selected before
    int skillCheck;                 // Required skill level
    std::string skillType;          // Type of skill check
    std::string responsePreview;    // Preview of NPC response
    
    DialogueOption()
        : hidden(false)
        , once(false)
        , selected(false)
        , skillCheck(0)
    {}
};

struct DialogueNode {
    std::string id;
    std::string speaker;
    std::string text;
    std::vector<DialogueOption> options;
    std::function<void()> onEnter;
    std::function<void()> onExit;
    
    // Enhanced features
    DialogueNodeType nodeType;
    SpeakerPortrait portrait;
    VoiceLine voiceLine;
    std::vector<TextTag> textTags;
    TextAnimationStyle textAnimation;
    float textSpeed;                // Characters per second
    
    // Branching
    std::optional<CompoundCondition> condition;
    std::string trueNodeId;
    std::string falseNodeId;
    
    // Actions
    std::vector<DialogueAction> enterActions;
    std::vector<DialogueAction> exitActions;
    
    // Random node selection
    std::vector<std::pair<std::string, float>> randomNodes;  // nodeId, weight
    
    // Metadata
    std::string comment;            // Editor comment
    std::vector<std::string> tags;
    bool visited;
    int visitCount;
    
    DialogueNode()
        : nodeType(DialogueNodeType::Speech)
        , textAnimation(TextAnimationStyle::Typewriter)
        , textSpeed(30.0f)
        , visited(false)
        , visitCount(0)
    {}
};

// =============================================================================
// Dialogue Tree
// =============================================================================

/**
 * @brief Complete dialogue tree/conversation
 */
class DialogueTree {
private:
    std::string m_id;
    std::string m_name;
    std::map<std::string, DialogueNode> m_nodes;
    std::string m_startNodeId;
    std::unordered_map<std::string, DialogueVariable> m_localVariables;
    
    // Metadata
    std::string m_author;
    std::string m_description;
    std::vector<std::string> m_characters;
    
public:
    DialogueTree(const std::string& id);
    
    // Node management
    void addNode(const DialogueNode& node);
    void removeNode(const std::string& nodeId);
    DialogueNode* getNode(const std::string& nodeId);
    const DialogueNode* getNode(const std::string& nodeId) const;
    bool hasNode(const std::string& nodeId) const;
    
    // Properties
    void setStartNode(const std::string& nodeId) { m_startNodeId = nodeId; }
    const std::string& getStartNode() const { return m_startNodeId; }
    const std::string& getId() const { return m_id; }
    const std::string& getName() const { return m_name; }
    void setName(const std::string& name) { m_name = name; }
    
    // Local variables
    void setVariable(const std::string& name, const DialogueVariable& value);
    DialogueVariable getVariable(const std::string& name) const;
    bool hasVariable(const std::string& name) const;
    
    // Query
    std::vector<std::string> getAllNodeIds() const;
    std::vector<std::string> getCharacters() const { return m_characters; }
    int getNodeCount() const { return static_cast<int>(m_nodes.size()); }
    
    // Validation
    bool validate(std::vector<std::string>& errors) const;
    std::vector<std::string> findDeadEnds() const;
    std::vector<std::string> findUnreachableNodes() const;
};

// =============================================================================
// Dialogue Callbacks
// =============================================================================

/**
 * @brief Callbacks for dialogue events
 */
struct DialogueCallbacks {
    std::function<void(const std::string& treeId)> onDialogueStart;
    std::function<void(const std::string& treeId)> onDialogueEnd;
    std::function<void(const DialogueNode&)> onNodeEnter;
    std::function<void(const DialogueNode&)> onNodeExit;
    std::function<void(const DialogueOption&, int index)> onOptionSelected;
    std::function<void(const std::string& speaker, const std::string& text)> onTextDisplay;
    std::function<void(const VoiceLine&)> onVoiceLinePlay;
    std::function<void(const SpeakerPortrait&)> onPortraitChange;
    std::function<void(const DialogueAction&)> onActionExecute;
};

// =============================================================================
// Dialogue System
// =============================================================================

class DialogueSystem {
public:
    static DialogueSystem& getInstance();
    
    // Legacy API
    void loadDialogue(const std::string& filename);
    void addNode(const DialogueNode& node);
    
    void startDialogue(const std::string& startNodeId);
    void selectOption(int optionIndex);
    void endDialogue();
    
    bool isActive() const;
    const DialogueNode* getCurrentNode() const;
    std::vector<DialogueOption> getAvailableOptions() const;
    
    // Enhanced API
    // Tree management
    void registerTree(std::unique_ptr<DialogueTree> tree);
    void unregisterTree(const std::string& treeId);
    DialogueTree* getTree(const std::string& treeId);
    bool loadTreeFromFile(const std::string& filepath);
    bool saveTreeToFile(const std::string& treeId, const std::string& filepath);
    
    // Dialogue control
    void startDialogue(const std::string& treeId, const std::string& nodeId);
    void advanceDialogue();
    void skipTextAnimation();
    void pauseDialogue();
    void resumeDialogue();
    
    // Global variables
    void setGlobalVariable(const std::string& name, const DialogueVariable& value);
    DialogueVariable getGlobalVariable(const std::string& name) const;
    bool hasGlobalVariable(const std::string& name) const;
    void clearGlobalVariables();
    
    // Relationship system
    void setRelationship(const std::string& character, int value);
    void modifyRelationship(const std::string& character, int delta);
    int getRelationship(const std::string& character) const;
    
    // History
    void enableHistory(bool enable);
    const std::vector<std::pair<std::string, std::string>>& getHistory() const;
    void clearHistory();
    
    // Callbacks
    void setCallbacks(const DialogueCallbacks& callbacks);
    
    // Update
    void update(float deltaTime);
    
    // Query
    const DialogueTree* getCurrentTree() const;
    bool isTextAnimating() const;
    float getTextProgress() const;
    bool isPaused() const { return m_paused; }

private:
    DialogueSystem();
    ~DialogueSystem();
    
    void processNode(const DialogueNode& node);
    void executeAction(const DialogueAction& action);
    std::string evaluateBranch(const DialogueNode& node);
    
    // Legacy
    std::map<std::string, DialogueNode> nodes;
    std::string currentNodeId;
    bool active;
    
    // Enhanced
    std::unordered_map<std::string, std::unique_ptr<DialogueTree>> m_trees;
    DialogueTree* m_currentTree;
    std::string m_currentNodeId;
    
    std::unordered_map<std::string, DialogueVariable> m_globalVariables;
    std::unordered_map<std::string, int> m_relationships;
    
    DialogueCallbacks m_callbacks;
    
    // Text animation state
    float m_textProgress;
    float m_textTimer;
    bool m_textComplete;
    bool m_paused;
    
    // History
    bool m_historyEnabled;
    std::vector<std::pair<std::string, std::string>> m_history;  // speaker, text
};

// =============================================================================
// Dialogue Localization
// =============================================================================

/**
 * @brief Localized string entry
 */
struct LocalizedDialogue {
    std::string key;
    std::unordered_map<std::string, std::string> translations;  // language -> text
    std::unordered_map<std::string, VoiceLine> voiceLines;      // language -> voice
};

/**
 * @brief Dialogue localization manager
 */
class DialogueLocalization {
private:
    static DialogueLocalization* instance;
    
    std::string m_currentLanguage;
    std::unordered_map<std::string, LocalizedDialogue> m_entries;
    
    DialogueLocalization();
    
public:
    static DialogueLocalization* getInstance();
    static void cleanup();
    
    void setLanguage(const std::string& language);
    const std::string& getLanguage() const { return m_currentLanguage; }
    
    void addEntry(const LocalizedDialogue& entry);
    std::string getText(const std::string& key) const;
    VoiceLine getVoiceLine(const std::string& key) const;
    
    bool loadFromFile(const std::string& filepath);
    std::vector<std::string> getAvailableLanguages() const;
};

} // namespace Gameplay
} // namespace JJM

#endif
