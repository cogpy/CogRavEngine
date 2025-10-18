#define RVE_TESTING_ACCESS 1
#include <RavEngine/CognitiveAgent.hpp>
#include <RavEngine/AgentOrchestrator.hpp>
#include <RavEngine/AtomSpace.hpp>
#include <RavEngine/World.hpp>
#include <RavEngine/Entity.hpp>
#include <RavEngine/App.hpp>
#include <RavEngine/Debug.hpp>
#include <iostream>

using namespace RavEngine;
using namespace std;

// needed for linker
const std::string_view RVE_VFS_get_name(){
    return "";
}
const std::span<const char> cmrc_get_file_data(const std::string_view& path) {
    return {};
}

#undef assert

#define assert(cond) \
{\
    Debug::Assert(cond, "Debug assertion failed! {}:{}",__FILE__,__LINE__);\
}

/**
 * Test AtomSpace functionality
 */
void test_atomspace() {
    Debug::Log("Testing AtomSpace...");
    
    AtomSpace atomSpace;
    
    // Test atom creation
    auto conceptID = atomSpace.AddAtom(AtomType::Concept, "test_concept");
    assert(conceptID > 0);
    
    // Test atom retrieval
    auto* atom = atomSpace.GetAtom(conceptID);
    assert(atom != nullptr);
    assert(atom->GetName() == "test_concept");
    assert(atom->GetType() == AtomType::Concept);
    
    // Test truth value update
    TruthValue tv(0.8f, 0.9f);
    atomSpace.UpdateTruthValue(conceptID, tv);
    
    atom = atomSpace.GetAtom(conceptID);
    assert(atom->GetTruthValue().strength == 0.8f);
    assert(atom->GetTruthValue().confidence == 0.9f);
    
    // Test link creation
    auto targetID = atomSpace.AddAtom(AtomType::Predicate, "test_predicate");
    auto linkID = atomSpace.AddLink(conceptID, targetID, "relates_to");
    assert(linkID > 0);
    
    auto* link = atomSpace.GetLink(linkID);
    assert(link != nullptr);
    assert(link->GetSource() == conceptID);
    assert(link->GetTarget() == targetID);
    assert(link->GetRelation() == "relates_to");
    
    // Test querying
    auto concepts = atomSpace.QueryByType(AtomType::Concept);
    assert(concepts.size() >= 1);
    
    auto links = atomSpace.QueryLinksBySource(conceptID);
    assert(links.size() >= 1);
    
    // Test duplicate atom handling
    auto duplicateID = atomSpace.AddAtom(AtomType::Concept, "test_concept");
    assert(duplicateID == conceptID);
    
    // Test statistics
    assert(atomSpace.GetAtomCount() >= 2);
    assert(atomSpace.GetLinkCount() >= 1);
    
    Debug::Log("AtomSpace tests passed!");
}

/**
 * Test CognitiveAgent with a simple world
 */
void test_cognitive_agent() {
    Debug::Log("Testing CognitiveAgent...");
    
    struct TestWorld : public World {};
    auto world = New<TestWorld>();
    
    // Create entity with cognitive agent
    auto entity = world->Instantiate<Entity>();
    auto& agent = entity.EmplaceComponent<CognitiveAgent>(entity);
    
    // Initialize agent
    agent.Start();
    
    // Verify initial state
    assert(agent.GetState() == AgentState::Processing);
    assert(agent.GetAtomSpace() != nullptr);
    assert(agent.GetAtomSpace()->GetAtomCount() > 0); // Should have self-awareness atoms
    
    // Add a goal
    Goal testGoal("test_goal", 0.8f);
    bool goalCompleted = false;
    testGoal.completionCheck = [&goalCompleted]() { return goalCompleted; };
    agent.AddGoal(testGoal);
    
    // Add an action
    bool actionExecuted = false;
    AgentAction testAction("test_action", 
        [&actionExecuted](float fps) { 
            actionExecuted = true; 
            return true; 
        }, 
        1.0f);
    agent.RegisterAction("test_action", testAction);
    
    // Add perception
    Perception testPerception("vision");
    testPerception.confidence = TruthValue(0.9f, 0.8f);
    agent.AddPerception(testPerception);
    
    // Run agent tick
    agent.Tick(1.0f);
    
    // Verify agent processed the input
    assert(agent.GetAtomSpace()->GetAtomCount() > 2); // Should have more atoms
    
    // Run more ticks to allow action execution
    for (int i = 0; i < 5; i++) {
        agent.Tick(1.0f);
    }
    
    // Verify action was executed
    assert(actionExecuted);
    
    // Test self-awareness methods
    agent.FormBeliefAbout("world_is_real", TruthValue(1.0f, 1.0f));
    agent.IntrospectState();
    agent.UpdateGoalPriorities();
    
    // Verify awareness level is valid
    assert(agent.GetAwarenessLevel() >= 0.0f && agent.GetAwarenessLevel() <= 1.0f);
    
    agent.Stop();
    
    Debug::Log("CognitiveAgent tests passed!");
}

/**
 * Test AgentOrchestrator with multiple agents
 */
void test_agent_orchestrator() {
    Debug::Log("Testing AgentOrchestrator...");
    
    struct TestWorld : public World {};
    auto world = New<TestWorld>();
    
    AgentOrchestrator orchestrator;
    
    // Create multiple agents
    auto entity1 = world->Instantiate<Entity>();
    auto& agent1 = entity1.EmplaceComponent<CognitiveAgent>(entity1);
    agent1.Start();
    
    auto entity2 = world->Instantiate<Entity>();
    auto& agent2 = entity2.EmplaceComponent<CognitiveAgent>(entity2);
    agent2.Start();
    
    // Register agents
    orchestrator.RegisterAgent(&agent1);
    orchestrator.RegisterAgent(&agent2);
    
    assert(orchestrator.GetAgentCount() == 2);
    
    // Add knowledge to agent1
    agent1.FormBeliefAbout("shared_concept", TruthValue(0.95f, 0.95f));
    
    // Run orchestrator coordination
    orchestrator.CoordinateAgents();
    
    // Verify shared knowledge exists
    auto sharedKnowledge = orchestrator.GetSharedKnowledge();
    assert(sharedKnowledge != nullptr);
    
    // Run multiple coordination cycles to trigger knowledge sharing
    for (int i = 0; i < 10; i++) {
        agent1.Tick(1.0f);
        agent2.Tick(1.0f);
        orchestrator.CoordinateAgents();
    }
    
    // Verify statistics
    float avgAwareness = orchestrator.GetAverageAwareness();
    float avgLoad = orchestrator.GetAverageCognitiveLoad();
    
    assert(avgAwareness >= 0.0f && avgAwareness <= 1.0f);
    assert(avgLoad >= 0.0f && avgLoad <= 1.0f);
    
    // Unregister agents
    orchestrator.UnregisterAgent(&agent1);
    assert(orchestrator.GetAgentCount() == 1);
    
    orchestrator.UnregisterAgent(&agent2);
    assert(orchestrator.GetAgentCount() == 0);
    
    Debug::Log("AgentOrchestrator tests passed!");
}

/**
 * Test truth value operations
 */
void test_truth_values() {
    Debug::Log("Testing TruthValue operations...");
    
    TruthValue tv1(0.6f, 0.7f);
    TruthValue tv2(0.8f, 0.5f);
    
    TruthValue merged = tv1 + tv2;
    
    // Strength should be average
    assert(merged.strength == 0.7f);
    
    // Confidence should increase but not exceed 1.0
    assert(merged.confidence >= 0.7f && merged.confidence <= 1.0f);
    
    Debug::Log("TruthValue tests passed!");
}

/**
 * Integration test with goal completion
 */
void test_goal_driven_behavior() {
    Debug::Log("Testing goal-driven behavior...");
    
    struct TestWorld : public World {};
    auto world = New<TestWorld>();
    
    auto entity = world->Instantiate<Entity>();
    auto& agent = entity.EmplaceComponent<CognitiveAgent>(entity);
    agent.Start();
    
    // Create a goal with completion condition
    int actionCount = 0;
    Goal goal("collect_items", 0.9f);
    goal.completionCheck = [&actionCount]() { return actionCount >= 3; };
    agent.AddGoal(goal);
    
    // Register action that progresses toward goal
    AgentAction collectAction("collect",
        [&actionCount](float fps) {
            actionCount++;
            return true;
        },
        0.5f);
    collectAction.expectedUtility = TruthValue(0.8f, 0.9f);
    agent.RegisterAction("collect", collectAction);
    
    // Run until goal is completed
    for (int i = 0; i < 20; i++) {
        agent.Tick(1.0f);
    }
    
    // Verify goal was worked on
    assert(actionCount >= 3);
    
    // Verify goal appears in atom space
    auto goals = agent.GetAtomSpace()->QueryByType(AtomType::Goal);
    assert(goals.size() > 0);
    
    Debug::Log("Goal-driven behavior tests passed!");
}

int main(int argc, char** argv) {
    Debug::Log("Starting CognitiveAgent test suite...");
    
    try {
        test_atomspace();
        test_truth_values();
        test_cognitive_agent();
        test_agent_orchestrator();
        test_goal_driven_behavior();
        
        Debug::Log("\n=== All tests passed! ===\n");
        return 0;
    }
    catch (const std::exception& e) {
        Debug::Log("Test failed with exception: %s", e.what());
        return 1;
    }
}
