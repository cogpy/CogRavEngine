#pragma once
#include "ScriptComponent.hpp"
#include "AtomSpace.hpp"
#include "Vector.hpp"
#include "Map.hpp"
#include <memory>
#include <functional>

namespace RavEngine {

    // Agent state for tracking internal conditions
    enum class AgentState {
        Idle,
        Processing,
        Acting,
        Learning
    };

    // Perception data from the environment
    struct Perception {
        std::string type;
        std::variant<float, std::string, vector3> data;
        TruthValue confidence;
        uint64_t timestamp;
        
        Perception(const std::string& t) : type(t), timestamp(0) {}
    };

    // Goal structure for goal-directed behavior
    struct Goal {
        std::string name;
        float priority;
        float completion;
        bool active;
        std::function<bool()> completionCheck;
        
        Goal(const std::string& n, float p = 0.5f)
            : name(n), priority(p), completion(0.0f), active(true) {}
    };

    // Action that can be executed by the agent
    struct AgentAction {
        std::string name;
        std::function<bool(float)> execute;
        float cost;
        TruthValue expectedUtility;
        
        AgentAction(const std::string& n, std::function<bool(float)> exec, float c = 1.0f)
            : name(n), execute(exec), cost(c) {}
    };

    /**
     * CognitiveAgent - Self-aware agent component inspired by OpenCog
     * Provides autonomous behavior with perception, reasoning, and action selection
     */
    class CognitiveAgent : public ScriptComponent {
    private:
        std::shared_ptr<AtomSpace> atomSpace;
        AgentState currentState;
        
        Vector<Perception> perceptions;
        Vector<Goal> goals;
        UnorderedMap<std::string, AgentAction> actions;
        
        float awarenessLevel;
        float cognitiveLoad;
        uint64_t tickCount;
        
        // Memory management
        static constexpr size_t MAX_PERCEPTION_HISTORY = 100;
        
        // Internal processing methods
        void ProcessPerceptions();
        void ReasonAboutGoals();
        void SelectAndExecuteAction(float fpsScale);
        void UpdateSelfAwareness();
        
    public:
        CognitiveAgent(Entity owner);
        virtual ~CognitiveAgent() = default;
        
        // ScriptComponent interface
        void Start() override;
        void Tick(float fpsScale) override;
        void Stop() override;
        
        // Agent interface
        void AddPerception(const Perception& perception);
        void AddGoal(const Goal& goal);
        void RegisterAction(const std::string& name, const AgentAction& action);
        
        // State queries
        AgentState GetState() const { return currentState; }
        float GetAwarenessLevel() const { return awarenessLevel; }
        float GetCognitiveLoad() const { return cognitiveLoad; }
        
        // Knowledge access
        std::shared_ptr<AtomSpace> GetAtomSpace() { return atomSpace; }
        const Vector<Goal>& GetGoals() const { return goals; }
        
        // Self-awareness methods
        void IntrospectState();
        void FormBeliefAbout(const std::string& subject, const TruthValue& truth);
        void UpdateGoalPriorities();
    };
}
