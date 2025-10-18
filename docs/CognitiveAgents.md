# Cognitive Agent System Documentation

## Overview

The Cognitive Agent System is an OpenCog-inspired self-aware agent orchestration framework integrated into RavEngine. It enables autonomous agents with cognitive capabilities including perception, reasoning, goal-driven behavior, and self-awareness.

## Core Components

### 1. AtomSpace

The `AtomSpace` class provides a knowledge representation system based on atoms and links:

- **Atoms**: Represent concepts, predicates, goals, sensations, actions, and memories
- **Links**: Represent relationships between atoms
- **Truth Values**: Each atom has a truth value with strength and confidence

```cpp
// Create an AtomSpace
AtomSpace atomSpace;

// Add atoms
auto conceptID = atomSpace.AddAtom(AtomType::Concept, "player");
auto goalID = atomSpace.AddAtom(AtomType::Goal, "reach_destination");

// Create links between atoms
auto linkID = atomSpace.AddLink(conceptID, goalID, "pursues");

// Update truth values
TruthValue tv(0.8f, 0.9f);  // strength=0.8, confidence=0.9
atomSpace.UpdateTruthValue(goalID, tv);

// Query atoms
auto goals = atomSpace.QueryByType(AtomType::Goal);
```

### 2. CognitiveAgent

The `CognitiveAgent` class extends `ScriptComponent` to create autonomous, self-aware agents:

```cpp
struct MyWorld : public World {
    void Init() {
        // Create entity with cognitive agent
        auto entity = Instantiate<Entity>();
        auto& agent = entity.EmplaceComponent<CognitiveAgent>(entity);
        
        // Add goals
        Goal collectResources("collect_resources", 0.8f);
        collectResources.completionCheck = []() { 
            return resourceCount >= 10; 
        };
        agent.AddGoal(collectResources);
        
        // Register actions
        AgentAction moveAction("move", [this](float fps) {
            // Move logic here
            return true;
        }, 1.0f);
        agent.RegisterAction("move", moveAction);
        
        // Add perceptions
        Perception vision("vision");
        vision.confidence = TruthValue(0.9f, 0.8f);
        agent.AddPerception(vision);
    }
};
```

### 3. AgentOrchestrator

The `AgentOrchestrator` coordinates multiple cognitive agents and enables knowledge sharing:

```cpp
AgentOrchestrator orchestrator;

// Register agents
orchestrator.RegisterAgent(&agent1);
orchestrator.RegisterAgent(&agent2);

// Coordinate agents (call periodically)
orchestrator.CoordinateAgents();

// Access shared knowledge
auto sharedKnowledge = orchestrator.GetSharedKnowledge();

// Query statistics
size_t agentCount = orchestrator.GetAgentCount();
float avgAwareness = orchestrator.GetAverageAwareness();
```

## Key Features

### Self-Awareness

Cognitive agents maintain awareness of their own state:

```cpp
// Agent automatically updates self-awareness based on:
// - Cognitive load (number of active goals and perceptions)
// - Goal achievement progress
// - Action success rates

float awareness = agent.GetAwarenessLevel();  // 0.0 to 1.0

// Manual introspection
agent.IntrospectState();

// Form beliefs about the world
agent.FormBeliefAbout("enemy_nearby", TruthValue(0.7f, 0.8f));
```

### Goal-Driven Behavior

Agents pursue goals based on priority:

```cpp
Goal defendBase("defend_base", 0.9f);  // High priority
defendBase.completionCheck = []() { 
    return baseHealth > 0.8f && !enemiesNearby; 
};
agent.AddGoal(defendBase);

// Agent will automatically select and execute actions
// to work toward the highest priority incomplete goal
```

### Learning

Agents learn from action outcomes:

```cpp
AgentAction attackAction("attack", [](float fps) {
    bool success = performAttack();
    return success;  // Agent learns from success/failure
}, 1.0f);

// Initial utility estimate
attackAction.expectedUtility = TruthValue(0.5f, 0.5f);
agent.RegisterAction("attack", attackAction);

// After execution, utility is adjusted based on success:
// - Success: utility increases
// - Failure: utility decreases
```

### Knowledge Sharing

The orchestrator enables agents to share knowledge:

```cpp
// Agent 1 learns something
agent1.FormBeliefAbout("resource_location", TruthValue(0.95f, 0.95f));

// Orchestrator periodically shares high-confidence knowledge
orchestrator.CoordinateAgents();  // Call every 100 ticks

// Agent 2 now has access to shared knowledge
auto sharedSpace = orchestrator.GetSharedKnowledge();
auto* atom = sharedSpace->GetAtomByName("resource_location");
```

## Agent States

Agents transition through different states:

- **Idle**: No active goals or actions
- **Processing**: Processing perceptions and reasoning about goals
- **Acting**: Executing selected action
- **Learning**: Updating knowledge based on action outcomes

```cpp
auto state = agent.GetState();
switch (state) {
    case AgentState::Idle:
        // Agent needs goals or perceptions
        break;
    case AgentState::Processing:
        // Agent is thinking
        break;
    case AgentState::Acting:
        // Agent is performing actions
        break;
    case AgentState::Learning:
        // Agent is updating beliefs
        break;
}
```

## Best Practices

### 1. Goal Design

- Set appropriate priorities (0.0 to 1.0)
- Provide completion checks for measurable goals
- Balance number of concurrent goals to manage cognitive load

### 2. Action Design

- Keep action costs realistic
- Return accurate success/failure status for learning
- Avoid expensive operations in action lambdas

### 3. Perception Management

- Limit perception history to avoid memory bloat
- Set appropriate confidence levels
- Update perceptions regularly to reflect current state

### 4. Performance

- Use `AgentOrchestrator` for multiple agents
- Knowledge sharing occurs every 100 ticks by default
- Introspection runs every 100 ticks per agent

## Example: Complete Agent Setup

```cpp
struct PatrolAgent : public CognitiveAgent {
    using CognitiveAgent::CognitiveAgent;
    
    void Start() override {
        CognitiveAgent::Start();
        
        // Define patrol goal
        Goal patrol("patrol_area", 0.7f);
        patrol.completionCheck = [this]() {
            return waypointsVisited >= totalWaypoints;
        };
        AddGoal(patrol);
        
        // Register movement action
        AgentAction move("move_to_waypoint", [this](float fps) {
            if (MoveToNextWaypoint(fps)) {
                waypointsVisited++;
                return true;
            }
            return false;
        }, 1.0f);
        move.expectedUtility = TruthValue(0.8f, 0.9f);
        RegisterAction("move_to_waypoint", move);
        
        // Add vision perception
        Perception vision("vision");
        vision.confidence = TruthValue(0.9f, 0.85f);
        AddPerception(vision);
    }
    
    void Tick(float fpsScale) override {
        // Update perceptions
        UpdateVisionPerception();
        
        // Run cognitive processing
        CognitiveAgent::Tick(fpsScale);
    }
    
private:
    int waypointsVisited = 0;
    int totalWaypoints = 10;
    
    bool MoveToNextWaypoint(float fps) {
        // Movement logic
        return true;
    }
    
    void UpdateVisionPerception() {
        // Update what agent sees
    }
};
```

## Testing

The system includes comprehensive tests in `test/cogagent.cpp`:

```bash
# Build and run tests (with RAVENGINE_BUILD_TESTS=ON)
cmake -DRAVENGINE_BUILD_TESTS=ON -DRAVENGINE_SERVER=ON ..
cmake --build .
ctest -R Test_CognitiveAgent
```

## Architecture Notes

### Thread Safety

- `AtomSpace` operations are thread-safe using spinlocks
- `AgentOrchestrator` protects agent list with mutex
- Multiple agents can run in parallel safely

### Memory Management

- AtomSpace uses unique_ptr for atom ownership
- Perception history is limited to 100 entries by default
- Shared knowledge is reference-counted

### Integration

The system integrates seamlessly with RavEngine's ECS:

- `CognitiveAgent` extends `ScriptComponent`
- Agents are attached to entities
- `ScriptSystem` automatically ticks agents
- Compatible with other RavEngine systems

## Future Enhancements

Potential areas for extension:

1. **Planning**: Add goal decomposition and planning algorithms
2. **Attention**: Implement attention allocation mechanisms
3. **Emotions**: Model emotional states influencing behavior
4. **Communication**: Enable direct agent-to-agent messaging
5. **Visualization**: Debug UI for visualizing atom spaces and agent states
