#pragma once

#include "Declarations.hpp"
#include "Archetype.hpp"
#include "World.hpp"

namespace Ecs {
    struct Entity {
        World* world;
        EntityId id;

        Entity(World& world);
        ~Entity();

        template<typename ComponentType>
        Entity& Add();

        template<typename ComponentType, typename... Arguments>
        Entity& Set(Arguments&&... arguments);

        template<typename ComponentType>
        Entity& Remove();

        template<typename ComponentType>
        bool Has();

        template<typename ComponentType>
        ComponentType& Get();
    };

    Entity::Entity(World& world) {
        this->world = &world;
        if(world.removed.empty())
            this->id = world.entities.size();
        else {
            this->id = world.removed.front();
            world.removed.front();
        }
        Record record;
        if(world.archetypes.find(TypeHash()) == world.archetypes.end())
            world.archetypes.insert({TypeHash(), new Archetype(TypeHash())});
        record.archetype = world.archetypes.at(TypeHash());
        record.archetype->entities.push_back(id);
        record.index = record.archetype->entities.size();
        world.entities.insert({id, record});
    }

    Entity::~Entity() {
        Record& record = world->entities.at(id);
        Archetype* archetype = record.archetype;
        for(int index = 0; index < archetype->type.size(); index++) {
            archetype->components[index].emplace(archetype->components[index].begin() + record.index, archetype->components[index][archetype->entities.size() - 1]);
            delete archetype->components[index].back();
            archetype->components[index].pop_back();
        }
        archetype->entities.emplace(archetype->entities.begin() + record.index, archetype->entities[archetype->entities.size() - 1]);
        archetype->entities.pop_back();
        world->entities.at(archetype->entities[record.index]).index = record.index;
        world->entities.erase(id);
        world->removed.push(id);
    }

    template<typename ComponentType>
    Entity& Entity::Add() {
        Record& record = world->entities.at(id);
        Archetype* archetype = record.archetype;
        std::queue<void*> data;
        for(int index = 0; index < archetype->type.size(); index++) {
            data.push(archetype->components[index][record.index]);
            archetype->components[index].emplace(archetype->components[index].begin() + record.index, archetype->components[index][archetype->entities.size() - 1]);
            archetype->components[index].pop_back();
        }
        archetype->entities.emplace(archetype->entities.begin() + record.index, archetype->entities[archetype->entities.size() - 1]);
        archetype->entities.pop_back();
        world->entities[archetype->entities[record.index]].index = record.index;
        ComponentId componentId = Component<ComponentType>();
        TypeHash hash = Hash(archetype->type);
        hash |= Hash({componentId});
        if(world->archetypes.find(hash) == world->archetypes.end())
            world->archetypes.insert({hash, new Archetype(hash)});
        archetype = world->archetypes.at(hash);
        for(int index = 0; index < archetype->type.size(); index++) {
            if(archetype->type[index] == componentId)
                archetype->components[index].push_back(new ComponentType());
            else {
                archetype->components[index].push_back(data.front());
                data.pop();
            }
        }
        record.archetype = archetype;
        record.index = archetype->entities.size();
        archetype->entities.push_back(id);
        return *this;
    }

    template<typename ComponentType, typename... Arguments>
    Entity& Entity::Set(Arguments&&... arguments) {
        if(!Has<ComponentType>())
            Add<ComponentType>();
        Record& record = world->entities.at(id);
        Archetype* archetype = record.archetype;
        ComponentId componentId = Component<ComponentType>();
        for(int index = 0; index < archetype->type.size(); index++) {
            if(archetype->type[index] == componentId)
                (*(ComponentType*)archetype->components[index][record.index]) = ComponentType{arguments...};
        }
        return *this;
    }

    template<typename ComponentType>
    Entity& Entity::Remove() {
        Record& record = world->entities.at(id);
        Archetype* archetype = record.archetype;
        ComponentId componentId = Component<ComponentType>();
        std::queue<void*> data;
        for(int index = 0; index < archetype->type.size(); index++) {
            if(archetype->type[index] == componentId)
                (*(ComponentType*)archetype->components[index][record.index]).~ComponentType();
            else
                data.push(archetype->components[index][record.index]);
            archetype->components[index][record.index] = archetype->components[index][archetype->entities.size() - 1];
            archetype->components[index].pop_back();
        }
        archetype->entities[record.index] = archetype->entities[archetype->entities.size() - 1];
        archetype->entities.pop_back();
        world->entities.at(archetype->entities[record.index]).index = record.index;
        TypeHash hash = Hash(archetype->type);
        hash ^= Hash({componentId});
        if(world->archetypes.find(hash) == world->archetypes.end())
            world->archetypes.insert({hash, new Archetype(hash)});
        archetype = world->archetypes.at(hash);
        for(int index = 0; index < archetype->type.size(); index++) {
            archetype->components[index].push_back(data.front());
            data.pop();
        }
        record.archetype = archetype;
        record.index = archetype->entities.size();
        archetype->entities.push_back(id);
        return *this;
    }

    template<typename ComponentType>
    bool Entity::Has() {
        Archetype* archetype = world->entities.at(id).archetype;
        ComponentId componentId = Component<ComponentType>();
        for(ComponentId typeComponent : archetype->type) {
            if(typeComponent == componentId)
                return true;
        }
        return false;
    }

    template<typename ComponentType>
    ComponentType& Entity::Get() {
        Record& record = world->entities.at(id);
        Archetype* archetype = record.archetype;
        ComponentId componentId = Component<ComponentType>();
        for(int index = 0; index < archetype->type.size(); index++) {
            if(archetype->type[index] == componentId)
                return *(ComponentType*)archetype->components[index][record.index];
        }
        throw std::invalid_argument("Entity: " + std::to_string(id) + " does not contain ComponentId: " + std::to_string(componentId));
    }
}