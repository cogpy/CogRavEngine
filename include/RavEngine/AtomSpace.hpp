#pragma once
#include "Types.hpp"
#include "Map.hpp"
#include "Vector.hpp"
#include "SpinLock.hpp"
#include "mathtypes.hpp"
#include <memory>
#include <string>
#include <variant>
#include <functional>

namespace RavEngine {

    // Atom types for knowledge representation
    enum class AtomType {
        Concept,      // Represents concepts/objects
        Predicate,    // Represents relationships
        Sensation,    // Sensory input
        Goal,         // Agent goals
        Action,       // Executable actions
        Memory        // Memory traces
    };

    // Atom truth value representing certainty and strength
    struct TruthValue {
        float strength = 0.5f;  // [0, 1] - how true is this
        float confidence = 0.5f; // [0, 1] - how confident we are
        
        TruthValue() = default;
        TruthValue(float s, float c) : strength(s), confidence(c) {}
        
        // Merge truth values
        TruthValue operator+(const TruthValue& other) const {
            float newStrength = (strength + other.strength) / 2.0f;
            float newConfidence = std::min(confidence + other.confidence, 1.0f);
            return TruthValue(newStrength, newConfidence);
        }
    };

    // Base class for atoms in the knowledge base
    class Atom {
    public:
        using atom_id_t = uint64_t;
        
    private:
        atom_id_t id;
        AtomType type;
        std::string name;
        TruthValue truthValue;
        uint64_t timestamp;
        
    public:
        Atom(atom_id_t id, AtomType type, const std::string& name)
            : id(id), type(type), name(name), timestamp(0) {}
        
        atom_id_t GetID() const { return id; }
        AtomType GetType() const { return type; }
        const std::string& GetName() const { return name; }
        const TruthValue& GetTruthValue() const { return truthValue; }
        void SetTruthValue(const TruthValue& tv) { truthValue = tv; }
        uint64_t GetTimestamp() const { return timestamp; }
        void SetTimestamp(uint64_t ts) { timestamp = ts; }
    };

    // Link between atoms
    class Link {
    public:
        using link_id_t = uint64_t;
        
    private:
        link_id_t id;
        Atom::atom_id_t source;
        Atom::atom_id_t target;
        std::string relation;
        TruthValue truthValue;
        
    public:
        Link(link_id_t id, Atom::atom_id_t src, Atom::atom_id_t tgt, const std::string& rel)
            : id(id), source(src), target(tgt), relation(rel) {}
        
        link_id_t GetID() const { return id; }
        Atom::atom_id_t GetSource() const { return source; }
        Atom::atom_id_t GetTarget() const { return target; }
        const std::string& GetRelation() const { return relation; }
        const TruthValue& GetTruthValue() const { return truthValue; }
        void SetTruthValue(const TruthValue& tv) { truthValue = tv; }
    };

    /**
     * AtomSpace - Knowledge representation system inspired by OpenCog
     * Stores atoms (concepts, predicates, goals) and links (relationships)
     */
    class AtomSpace {
    private:
        UnorderedMap<Atom::atom_id_t, std::unique_ptr<Atom>> atoms;
        UnorderedMap<Link::link_id_t, std::unique_ptr<Link>> links;
        UnorderedMap<std::string, Atom::atom_id_t> nameToAtomID;
        
        Atom::atom_id_t nextAtomID = 1;
        Link::link_id_t nextLinkID = 1;
        
        mutable SpinLock atomMutex;
        mutable SpinLock linkMutex;
        
    public:
        AtomSpace() = default;
        
        // Create or retrieve an atom
        Atom::atom_id_t AddAtom(AtomType type, const std::string& name) {
            std::lock_guard lock(atomMutex);
            
            // Check if atom already exists
            auto it = nameToAtomID.find(name);
            if (it != nameToAtomID.end()) {
                return it->second;
            }
            
            // Create new atom
            auto id = nextAtomID++;
            atoms[id] = std::make_unique<Atom>(id, type, name);
            nameToAtomID[name] = id;
            return id;
        }
        
        // Create a link between atoms
        Link::link_id_t AddLink(Atom::atom_id_t source, Atom::atom_id_t target, const std::string& relation) {
            std::lock_guard lock(linkMutex);
            
            auto id = nextLinkID++;
            links[id] = std::make_unique<Link>(id, source, target, relation);
            return id;
        }
        
        // Get atom by ID
        Atom* GetAtom(Atom::atom_id_t id) {
            std::lock_guard lock(atomMutex);
            auto it = atoms.find(id);
            return it != atoms.end() ? it->second.get() : nullptr;
        }
        
        // Get atom by name
        Atom* GetAtomByName(const std::string& name) {
            std::lock_guard lock(atomMutex);
            auto it = nameToAtomID.find(name);
            if (it != nameToAtomID.end()) {
                return atoms[it->second].get();
            }
            return nullptr;
        }
        
        // Get link by ID
        Link* GetLink(Link::link_id_t id) {
            std::lock_guard lock(linkMutex);
            auto it = links.find(id);
            return it != links.end() ? it->second.get() : nullptr;
        }
        
        // Update atom truth value
        void UpdateTruthValue(Atom::atom_id_t id, const TruthValue& tv) {
            std::lock_guard lock(atomMutex);
            if (auto it = atoms.find(id); it != atoms.end()) {
                it->second->SetTruthValue(tv);
            }
        }
        
        // Query atoms by type
        Vector<Atom*> QueryByType(AtomType type) {
            std::lock_guard lock(atomMutex);
            Vector<Atom*> results;
            for (auto& [id, atom] : atoms) {
                if (atom->GetType() == type) {
                    results.push_back(atom.get());
                }
            }
            return results;
        }
        
        // Query links by source atom
        Vector<Link*> QueryLinksBySource(Atom::atom_id_t source) {
            std::lock_guard lock(linkMutex);
            Vector<Link*> results;
            for (auto& [id, link] : links) {
                if (link->GetSource() == source) {
                    results.push_back(link.get());
                }
            }
            return results;
        }
        
        // Clear all data
        void Clear() {
            std::lock_guard lock1(atomMutex);
            std::lock_guard lock2(linkMutex);
            atoms.clear();
            links.clear();
            nameToAtomID.clear();
            nextAtomID = 1;
            nextLinkID = 1;
        }
        
        // Get statistics
        size_t GetAtomCount() const {
            std::lock_guard lock(atomMutex);
            return atoms.size();
        }
        
        size_t GetLinkCount() const {
            std::lock_guard lock(linkMutex);
            return links.size();
        }
    };
}
