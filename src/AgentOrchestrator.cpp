#include "AgentOrchestrator.hpp"
#include "Debug.hpp"
#include <algorithm>

using namespace RavEngine;

AgentOrchestrator::AgentOrchestrator()
    : sharedKnowledge(std::make_shared<AtomSpace>())
    , orchestratorTick(0)
{
    Debug::Log("AgentOrchestrator: Initialized");
}

void AgentOrchestrator::RegisterAgent(CognitiveAgent* agent) {
    if (!agent) return;
    
    std::lock_guard lock(agentMutex);
    
    // Check if agent already registered
    for (const auto& info : managedAgents) {
        if (info.agent == agent) {
            return;
        }
    }
    
    managedAgents.emplace_back(agent);
    Debug::Log("AgentOrchestrator: Registered agent (total: %zu)", managedAgents.size());
}

void AgentOrchestrator::UnregisterAgent(CognitiveAgent* agent) {
    if (!agent) return;
    
    std::lock_guard lock(agentMutex);
    
    auto it = std::remove_if(managedAgents.begin(), managedAgents.end(),
        [agent](const AgentInfo& info) { return info.agent == agent; });
    
    if (it != managedAgents.end()) {
        managedAgents.erase(it, managedAgents.end());
        Debug::Log("AgentOrchestrator: Unregistered agent (remaining: %zu)", managedAgents.size());
    }
}

void AgentOrchestrator::operator()(CognitiveAgent& agent) const {
    // This operator is called by the ECS system
    // The agent's Tick is already called by ScriptSystem
    // Here we can add orchestration-level coordination
}

void AgentOrchestrator::CoordinateAgents() {
    std::lock_guard lock(agentMutex);
    orchestratorTick++;
    
    if (managedAgents.empty()) {
        return;
    }
    
    // Update agent performance metrics
    for (auto& info : managedAgents) {
        if (!info.agent) continue;
        
        // Calculate performance based on awareness and goal progress
        float awareness = info.agent->GetAwarenessLevel();
        float cogLoad = info.agent->GetCognitiveLoad();
        
        // Performance decreases with high cognitive load
        info.performance = awareness * (1.0f - cogLoad * 0.5f);
        info.lastUpdateTick = orchestratorTick;
    }
    
    // Share knowledge between agents
    if (orchestratorTick % 100 == 0) {
        ShareKnowledge();
    }
}

void AgentOrchestrator::ShareKnowledge() {
    std::lock_guard lock(agentMutex);
    
    if (managedAgents.empty()) {
        return;
    }
    
    // Collect high-confidence beliefs from all agents
    for (auto& info : managedAgents) {
        if (!info.agent) continue;
        
        auto agentSpace = info.agent->GetAtomSpace();
        if (!agentSpace) continue;
        
        // Query for high-confidence concepts
        auto concepts = agentSpace->QueryByType(AtomType::Concept);
        for (auto* atom : concepts) {
            if (atom->GetTruthValue().confidence > 0.8f) {
                // Add to shared knowledge
                auto sharedID = sharedKnowledge->AddAtom(AtomType::Concept, atom->GetName());
                sharedKnowledge->UpdateTruthValue(sharedID, atom->GetTruthValue());
            }
        }
        
        // Share successful actions
        auto actions = agentSpace->QueryByType(AtomType::Action);
        for (auto* atom : actions) {
            if (atom->GetTruthValue().strength > 0.7f) {
                auto sharedID = sharedKnowledge->AddAtom(AtomType::Action, atom->GetName());
                sharedKnowledge->UpdateTruthValue(sharedID, atom->GetTruthValue());
            }
        }
    }
    
    // Distribute shared knowledge back to agents
    for (auto& info : managedAgents) {
        if (!info.agent) continue;
        
        auto agentSpace = info.agent->GetAtomSpace();
        if (!agentSpace) continue;
        
        // Copy high-value shared knowledge to agent
        auto sharedConcepts = sharedKnowledge->QueryByType(AtomType::Concept);
        for (auto* atom : sharedConcepts) {
            if (atom->GetTruthValue().confidence > 0.9f) {
                auto agentID = agentSpace->AddAtom(AtomType::Concept, atom->GetName());
                agentSpace->UpdateTruthValue(agentID, atom->GetTruthValue());
            }
        }
    }
    
    Debug::Log("AgentOrchestrator: Shared knowledge synchronized (%zu atoms)", 
               sharedKnowledge->GetAtomCount());
}

size_t AgentOrchestrator::GetAgentCount() const {
    std::lock_guard lock(agentMutex);
    return managedAgents.size();
}

float AgentOrchestrator::GetAverageAwareness() const {
    std::lock_guard lock(agentMutex);
    
    if (managedAgents.empty()) {
        return 0.0f;
    }
    
    float total = 0.0f;
    for (const auto& info : managedAgents) {
        if (info.agent) {
            total += info.agent->GetAwarenessLevel();
        }
    }
    
    return total / managedAgents.size();
}

float AgentOrchestrator::GetAverageCognitiveLoad() const {
    std::lock_guard lock(agentMutex);
    
    if (managedAgents.empty()) {
        return 0.0f;
    }
    
    float total = 0.0f;
    for (const auto& info : managedAgents) {
        if (info.agent) {
            total += info.agent->GetCognitiveLoad();
        }
    }
    
    return total / managedAgents.size();
}
