#pragma once

#include <bitset>
#include <queue>
#include <vector>
#include <unordered_map>

namespace Ecs {
    using ComponentId = unsigned char;
    using EntityId = unsigned int;
    using Type = std::vector<unsigned char>;
    using TypeHash = std::bitset<64>;

    struct Archetype {
        Type type;
        std::vector<std::vector<void*>> components;
        std::vector<EntityId> entities;

        Archetype(TypeHash hash);

        template<typename ComponentType>
        ComponentType* Data();
    };

    struct Record {
        Archetype* archetype;
        unsigned int index;
    };

    struct World {
        std::unordered_map<EntityId, Record> entities;
        std::unordered_map<TypeHash, Archetype*> archetypes;
        std::queue<EntityId> removed;

        ~World();
    };

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

    template<typename... ComponentTypes, typename... Arguments, typename Function>
    void System(World& world, Function function, Arguments&&... arguments);

    static unsigned char componentId = 0;

    template<typename ComponentType>
    unsigned char Component() {
        static unsigned char id = componentId++;
        return id;
    }

    TypeHash Hash(Type type) {
        long long hash = 0;
        for(unsigned char component : type)
            hash |= (1ull << component);
        return TypeHash(hash);
    }

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
            archetype->components[index][record.index] = archetype->components[index][archetype->entities.size() - 1];
            archetype->components[index].pop_back();
        }
        archetype->entities[record.index] = archetype->entities[archetype->entities.size() - 1];
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
            archetype->components[index][record.index] = archetype->components[index][archetype->entities.size() - 1];
            archetype->components[index].pop_back();
        }
        archetype->entities[record.index] = archetype->entities[archetype->entities.size() - 1];
        archetype->entities.pop_back();
        world->entities.at(archetype->entities[record.index]).index = record.index;
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
                archetype->components[index][record.index] = new ComponentType{arguments...};
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

    template<typename... ComponentTypes, typename... Arguments, typename Function>
    void System(World& world, Function function, Arguments&&... arguments) {
        TypeHash hash = Hash({Component<ComponentTypes>()...});
        for(auto& pair : world.archetypes) {
            TypeHash temporary = hash;
            if((temporary &= pair.first) == hash) {
                for(EntityId id : pair.second->entities)
                    function(pair.second->Data<ComponentTypes>()[id]..., arguments...);
            }
        }
    }

    Archetype::Archetype(TypeHash hash) {
        for(unsigned char index = 0; index < hash.size(); index++) {
            if(hash[index] == 1)
                type.push_back(index);
        }
        components.resize(type.size());
    }

    template<typename ComponentType>
    ComponentType* Archetype::Data(){
        unsigned char componentId = Component<ComponentType>();
        for(unsigned char index = 0; index < type.size(); index++){
            if(type[index] == componentId)
                return (ComponentType*)(*components[index].data());
        }
        throw std::invalid_argument("Chunk does not exist");
    }

    World::~World(){
        entities.clear();
        for(unsigned int index = 0; index < archetypes.size(); index++)
            delete archetypes[index];
        archetypes.clear();
        while(!removed.empty())
            removed.pop();
    }
}