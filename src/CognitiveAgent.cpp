#include "CognitiveAgent.hpp"
#include "App.hpp"
#include "Debug.hpp"
#include <algorithm>

using namespace RavEngine;

CognitiveAgent::CognitiveAgent(Entity owner)
    : ScriptComponent(owner)
    , atomSpace(std::make_shared<AtomSpace>())
    , currentState(AgentState::Idle)
    , awarenessLevel(0.5f)
    , cognitiveLoad(0.0f)
    , tickCount(0)
{
}

void CognitiveAgent::Start() {
    Debug::Log("CognitiveAgent: Starting agent initialization");
    
    // Initialize self-awareness atoms
    auto selfID = atomSpace->AddAtom(AtomType::Concept, "self");
    auto awarenessID = atomSpace->AddAtom(AtomType::Concept, "awareness");
    atomSpace->AddLink(selfID, awarenessID, "has_property");
    
    // Set initial awareness truth value
    atomSpace->UpdateTruthValue(awarenessID, TruthValue(awarenessLevel, 1.0f));
    
    currentState = AgentState::Processing;
}

void CognitiveAgent::Tick(float fpsScale) {
    tickCount++;
    
    // Update cognitive load based on active goals and perceptions
    cognitiveLoad = (goals.size() * 0.1f + perceptions.size() * 0.05f) / 10.0f;
    cognitiveLoad = std::clamp(cognitiveLoad, 0.0f, 1.0f);
    
    // Process sensory input
    ProcessPerceptions();
    
    // Reason about current goals
    ReasonAboutGoals();
    
    // Update self-awareness
    UpdateSelfAwareness();
    
    // Select and execute actions
    SelectAndExecuteAction(fpsScale);
    
    // Periodic introspection
    if (tickCount % 100 == 0) {
        IntrospectState();
    }
}

void CognitiveAgent::Stop() {
    Debug::Log("CognitiveAgent: Stopping agent");
    currentState = AgentState::Idle;
    perceptions.clear();
    goals.clear();
}

void CognitiveAgent::ProcessPerceptions() {
    if (perceptions.empty()) {
        return;
    }
    
    currentState = AgentState::Processing;
    
    // Convert perceptions into atoms
    for (auto& perception : perceptions) {
        auto perceptID = atomSpace->AddAtom(AtomType::Sensation, perception.type);
        atomSpace->UpdateTruthValue(perceptID, perception.confidence);
        
        // Link to self
        auto selfID = atomSpace->AddAtom(AtomType::Concept, "self");
        atomSpace->AddLink(selfID, perceptID, "perceives");
    }
    
    // Maintain perception history limit
    if (perceptions.size() > MAX_PERCEPTION_HISTORY) {
        perceptions.erase(perceptions.begin(), 
                         perceptions.begin() + (perceptions.size() - MAX_PERCEPTION_HISTORY));
    }
}

void CognitiveAgent::ReasonAboutGoals() {
    if (goals.empty()) {
        return;
    }
    
    // Update goal completion and priority
    for (auto& goal : goals) {
        if (!goal.active) continue;
        
        // Check completion
        if (goal.completionCheck && goal.completionCheck()) {
            goal.completion = 1.0f;
            goal.active = false;
            
            // Create memory of achieved goal
            auto goalAtom = atomSpace->AddAtom(AtomType::Goal, goal.name);
            atomSpace->UpdateTruthValue(goalAtom, TruthValue(1.0f, 1.0f));
        }
        
        // Store goal in atom space
        auto goalID = atomSpace->AddAtom(AtomType::Goal, goal.name);
        TruthValue goalTV(goal.priority, goal.completion);
        atomSpace->UpdateTruthValue(goalID, goalTV);
    }
    
    // Sort goals by priority
    std::sort(goals.begin(), goals.end(), 
        [](const Goal& a, const Goal& b) { 
            return a.priority > b.priority; 
        });
}

void CognitiveAgent::SelectAndExecuteAction(float fpsScale) {
    if (actions.empty() || goals.empty()) {
        currentState = AgentState::Idle;
        return;
    }
    
    currentState = AgentState::Acting;
    
    // Find highest priority active goal
    Goal* activeGoal = nullptr;
    for (auto& goal : goals) {
        if (goal.active && goal.completion < 1.0f) {
            activeGoal = &goal;
            break;
        }
    }
    
    if (!activeGoal) {
        currentState = AgentState::Idle;
        return;
    }
    
    // Select best action for the goal (simple utility-based selection)
    AgentAction* bestAction = nullptr;
    float bestUtility = -1.0f;
    
    for (auto& [name, action] : actions) {
        float utility = action.expectedUtility.strength / (action.cost + 0.1f);
        if (utility > bestUtility) {
            bestUtility = utility;
            bestAction = &action;
        }
    }
    
    // Execute selected action
    if (bestAction && bestAction->execute) {
        bool success = bestAction->execute(fpsScale);
        
        // Update action atom with result
        auto actionID = atomSpace->AddAtom(AtomType::Action, bestAction->name);
        TruthValue resultTV(success ? 1.0f : 0.0f, 1.0f);
        atomSpace->UpdateTruthValue(actionID, resultTV);
        
        // Learn from action result
        if (success) {
            bestAction->expectedUtility.strength = 
                std::min(1.0f, bestAction->expectedUtility.strength + 0.05f);
            activeGoal->completion += 0.1f;
        } else {
            bestAction->expectedUtility.strength = 
                std::max(0.0f, bestAction->expectedUtility.strength - 0.05f);
        }
    }
}

void CognitiveAgent::UpdateSelfAwareness() {
    // Self-awareness is influenced by:
    // - Cognitive load (too much reduces awareness)
    // - Active goals (having direction increases awareness)
    // - Recent action success (learning increases awareness)
    
    float loadFactor = 1.0f - (cognitiveLoad * 0.5f);
    float goalFactor = std::min(1.0f, goals.size() * 0.1f);
    
    awarenessLevel = (awarenessLevel * 0.9f) + (loadFactor * goalFactor * 0.1f);
    awarenessLevel = std::clamp(awarenessLevel, 0.0f, 1.0f);
    
    // Update awareness atom
    auto awarenessID = atomSpace->AddAtom(AtomType::Concept, "awareness");
    atomSpace->UpdateTruthValue(awarenessID, TruthValue(awarenessLevel, 1.0f));
}

void CognitiveAgent::AddPerception(const Perception& perception) {
    perceptions.push_back(perception);
}

void CognitiveAgent::AddGoal(const Goal& goal) {
    goals.push_back(goal);
}

void CognitiveAgent::RegisterAction(const std::string& name, const AgentAction& action) {
    actions[name] = action;
}

void CognitiveAgent::IntrospectState() {
    // Create introspection atoms
    auto selfID = atomSpace->AddAtom(AtomType::Concept, "self");
    auto stateID = atomSpace->AddAtom(AtomType::Concept, "current_state");
    
    // Record state information
    TruthValue stateTV(static_cast<float>(currentState) / 4.0f, 1.0f);
    atomSpace->UpdateTruthValue(stateID, stateTV);
    atomSpace->AddLink(selfID, stateID, "has_state");
    
    // Log introspection
    Debug::Log("CognitiveAgent Introspection: Awareness=%.2f, Load=%.2f, Goals=%zu, Atoms=%zu",
               awarenessLevel, cognitiveLoad, goals.size(), atomSpace->GetAtomCount());
}

void CognitiveAgent::FormBeliefAbout(const std::string& subject, const TruthValue& truth) {
    auto beliefID = atomSpace->AddAtom(AtomType::Concept, subject);
    atomSpace->UpdateTruthValue(beliefID, truth);
    
    auto selfID = atomSpace->AddAtom(AtomType::Concept, "self");
    atomSpace->AddLink(selfID, beliefID, "believes");
}

void CognitiveAgent::UpdateGoalPriorities() {
    // Adjust goal priorities based on awareness and cognitive load
    for (auto& goal : goals) {
        if (cognitiveLoad > 0.8f && goal.priority < 0.9f) {
            // Reduce priority of non-critical goals when overloaded
            goal.priority *= 0.9f;
        } else if (awarenessLevel > 0.7f) {
            // Increase priority of active goals when highly aware
            if (goal.active) {
                goal.priority = std::min(1.0f, goal.priority * 1.05f);
            }
        }
    }
}
