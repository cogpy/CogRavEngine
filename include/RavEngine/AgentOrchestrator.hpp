#pragma once
#include "CTTI.hpp"
#include "CognitiveAgent.hpp"
#include "Vector.hpp"
#include "Map.hpp"
#include "SpinLock.hpp"
#include <memory>

namespace RavEngine {
    
    /**
     * AgentOrchestrator - Manages and coordinates multiple cognitive agents
     * Provides centralized control and inter-agent communication
     */
    class AgentOrchestrator : public AutoCTTI {
    public:
        struct AgentInfo {
            CognitiveAgent* agent;
            float performance;
            uint64_t lastUpdateTick;
            
            AgentInfo(CognitiveAgent* a) 
                : agent(a), performance(1.0f), lastUpdateTick(0) {}
        };
        
    private:
        Vector<AgentInfo> managedAgents;
        std::shared_ptr<AtomSpace> sharedKnowledge;
        mutable SpinLock agentMutex;
        uint64_t orchestratorTick;
        
    public:
        AgentOrchestrator();
        
        // Agent management
        void RegisterAgent(CognitiveAgent* agent);
        void UnregisterAgent(CognitiveAgent* agent);
        
        // Orchestration
        void operator()(CognitiveAgent& agent) const;
        void CoordinateAgents();
        void ShareKnowledge();
        
        // Knowledge access
        std::shared_ptr<AtomSpace> GetSharedKnowledge() { return sharedKnowledge; }
        
        // Statistics
        size_t GetAgentCount() const;
        float GetAverageAwareness() const;
        float GetAverageCognitiveLoad() const;
    };
}
