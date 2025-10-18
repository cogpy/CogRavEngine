/**
 * Example: Cognitive Agent System Usage
 * 
 * This example demonstrates how to create autonomous, self-aware agents
 * using the OpenCog-inspired cognitive agent system in RavEngine.
 */

#include <RavEngine/App.hpp>
#include <RavEngine/World.hpp>
#include <RavEngine/Entity.hpp>
#include <RavEngine/CognitiveAgent.hpp>
#include <RavEngine/AgentOrchestrator.hpp>
#include <RavEngine/Transform.hpp>
#include <RavEngine/Debug.hpp>
#include <ctime>

using namespace RavEngine;

/**
 * Custom cognitive agent that patrols waypoints
 */
class PatrolAgent : public CognitiveAgent {
public:
    PatrolAgent(Entity owner) : CognitiveAgent(owner) {}
    
    void Start() override {
        CognitiveAgent::Start();
        
        // Define patrol goal
        Goal patrolGoal("patrol_waypoints", 0.8f);
        patrolGoal.completionCheck = [this]() {
            return currentWaypoint >= waypoints.size();
        };
        AddGoal(patrolGoal);
        
        // Register movement action
        AgentAction moveAction("move_to_waypoint", [this](float fps) {
            return MoveToNextWaypoint(fps);
        }, 1.0f);
        moveAction.expectedUtility = TruthValue(0.8f, 0.9f);
        RegisterAction("move_to_waypoint", moveAction);
        
        // Initialize waypoints
        waypoints = {
            vector3{0, 0, 0},
            vector3{10, 0, 0},
            vector3{10, 0, 10},
            vector3{0, 0, 10}
        };
    }
    
    void Tick(float fpsScale) override {
        // Update perceptions
        UpdatePerceptions();
        
        // Run cognitive processing
        CognitiveAgent::Tick(fpsScale);
    }

private:
    Vector<vector3> waypoints;
    size_t currentWaypoint = 0;
    
    bool MoveToNextWaypoint(float fps) {
        if (currentWaypoint >= waypoints.size()) {
            return false;
        }
        
        // GetTransform() is guaranteed to return valid reference by RavEngine
        auto& transform = GetTransform();
        auto currentPos = transform.GetWorldPosition();
        auto targetPos = waypoints[currentWaypoint];
        
        // Simple movement toward target
        vector3 direction = targetPos - currentPos;
        float distance = glm::length(direction);
        
        if (distance < 0.1f) {
            // Reached waypoint
            currentWaypoint++;
            
            // Form belief about visiting waypoint
            std::string waypointName = "waypoint_" + std::to_string(currentWaypoint);
            FormBeliefAbout(waypointName, TruthValue(1.0f, 1.0f));
            
            return true;
        }
        
        // Move toward waypoint
        direction = glm::normalize(direction);
        vector3 newPos = currentPos + direction * 5.0f * fps;
        transform.SetWorldPosition(newPos);
        
        return false;  // Still moving
    }
    
    void UpdatePerceptions() {
        // Add vision perception
        Perception vision("vision");
        vision.confidence = TruthValue(0.9f, 0.85f);
        AddPerception(vision);
    }
};

/**
 * Combat-capable cognitive agent
 */
class CombatAgent : public CognitiveAgent {
public:
    CombatAgent(Entity owner) : CognitiveAgent(owner) {}
    
    void Start() override {
        CognitiveAgent::Start();
        
        // Add survival goal
        Goal surviveGoal("survive", 0.95f);  // High priority
        surviveGoal.completionCheck = [this]() {
            return health > 0.8f;
        };
        AddGoal(surviveGoal);
        
        // Add attack goal
        Goal attackGoal("attack_enemy", 0.7f);
        attackGoal.completionCheck = [this]() {
            return enemiesDefeated >= 3;
        };
        AddGoal(attackGoal);
        
        // Register attack action
        AgentAction attackAction("attack", [this](float fps) {
            return PerformAttack();
        }, 2.0f);  // Higher cost
        attackAction.expectedUtility = TruthValue(0.6f, 0.7f);
        RegisterAction("attack", attackAction);
        
        // Register heal action
        AgentAction healAction("heal", [this](float fps) {
            return PerformHeal();
        }, 3.0f);  // High cost
        healAction.expectedUtility = TruthValue(0.8f, 0.9f);
        RegisterAction("heal", healAction);
    }
    
    void Tick(float fpsScale) override {
        // Update perceptions
        DetectEnemies();
        
        // Run cognitive processing
        CognitiveAgent::Tick(fpsScale);
        
        // Update goal priorities based on health
        UpdateGoalPriorities();
    }

private:
    float health = 1.0f;
    int enemiesDefeated = 0;
    
    bool PerformAttack() {
        // Attack logic - using simple random for example purposes
        // In production, use std::mt19937 or game-specific RNG
        static bool seeded = false;
        if (!seeded) {
            srand(static_cast<unsigned>(time(nullptr)));
            seeded = true;
        }
        bool success = (rand() % 100) < 70;  // 70% success rate
        
        if (success) {
            enemiesDefeated++;
            FormBeliefAbout("combat_effective", TruthValue(0.9f, 0.9f));
        }
        
        // Take some damage
        health -= 0.05f;
        
        return success;
    }
    
    bool PerformHeal() {
        health = std::min(1.0f, health + 0.3f);
        FormBeliefAbout("healing_works", TruthValue(1.0f, 1.0f));
        return true;
    }
    
    void DetectEnemies() {
        // Simulate enemy detection
        Perception enemySight("enemy_detected");
        enemySight.confidence = TruthValue(0.85f, 0.9f);
        AddPerception(enemySight);
    }
};

/**
 * Example world with cognitive agents
 */
struct CognitiveWorld : public World {
    Ref<AgentOrchestrator> orchestrator;
    
    void Init() {
        // Create orchestrator
        orchestrator = New<AgentOrchestrator>();
        
        // Create patrol agents
        for (int i = 0; i < 3; i++) {
            auto entity = Instantiate<Entity>();
            auto& agent = entity.EmplaceComponent<PatrolAgent>(entity);
            orchestrator->RegisterAgent(&agent);
        }
        
        // Create combat agents
        for (int i = 0; i < 2; i++) {
            auto entity = Instantiate<Entity>();
            auto& agent = entity.EmplaceComponent<CombatAgent>(entity);
            orchestrator->RegisterAgent(&agent);
        }
        
        // Note: %zu works with Debug::Log which uses fmt library internally
        Debug::Log("Cognitive World initialized with {} agents", 
                   orchestrator->GetAgentCount());
    }
    
    void Tick(float scale) override {
        World::Tick(scale);
        
        // Coordinate all agents
        orchestrator->CoordinateAgents();
        
        // Log statistics periodically
        static int tickCount = 0;
        if (++tickCount % 1000 == 0) {
            Debug::Log("Agent Stats: Avg Awareness={:.2f}, Avg Load={:.2f}",
                      orchestrator->GetAverageAwareness(),
                      orchestrator->GetAverageCognitiveLoad());
        }
    }
};

/**
 * Application entry point
 */
struct CognitiveApp : public App {
    void OnStartup(int argc, char** argv) override {
        SetWindowTitle("Cognitive Agent Demo");
        
        auto world = New<CognitiveWorld>();
        world->Init();
        AddWorld(world);
        
        Debug::Log("Cognitive Agent application started");
    }
    
    void OnFatal(const std::string_view msg) override {
        Debug::Log("Fatal error: %s", msg.data());
    }
};

START_APP(CognitiveApp)
