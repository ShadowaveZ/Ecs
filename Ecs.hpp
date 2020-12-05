#pragma once

#include <bitset>
#include <queue>
#include <vector>
#include <unordered_map>

namespace Ecs {
    using Type = std::vector<unsigned char>;
    using TypeHash = std::bitset<64>;
    using EntityId = unsigned int;

    struct Archetype {
        Type type;
        std::vector<std::vector<void*>> components;
        std::vector<EntityId> entities;

        template<typename ComponentType>
        ComponentType* GetChunk();
    };

    struct Record {
        Archetype* archetype;
        unsigned int index;
    };

    struct World {
        std::unordered_map<EntityId, Record> entities;
        std::unordered_map<TypeHash, Archetype*> archetypes;
        std::queue<EntityId> removed;
        EntityId entityIndex = 0;
        bool shouldStop;

        EntityId CreateEntity();
        Archetype* GetArchetype(TypeHash hash);

        ~World();

        bool Run();
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

    Entity::Entity(World& world)
        : world(&world), id(world.CreateEntity()) {}

    Entity::~Entity(){
        Record& record = world->entities.at(id);
        Archetype* archetype = record.archetype;
        for(unsigned char index = 0; index < archetype->type.size(); index++) {
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
    Entity& Entity::Add(){
        Record& record = world->entities.at(id);
        Archetype* archetype = record.archetype;
        std::queue<void*> data;
        for(unsigned char index = 0; index < archetype->type.size(); index++) {
            data.push(archetype->components[index][record.index]);
            archetype->components[index][record.index] = archetype->components[index][archetype->entities.size() - 1];
            archetype->components[index].pop_back();
        }
        archetype->entities[record.index] = archetype->entities[archetype->entities.size() - 1];
        archetype->entities.pop_back();
        world->entities.at(archetype->entities[record.index]).index = record.index;
        unsigned char componentId = Component<ComponentType>();
        TypeHash hash = Hash(archetype->type);
        Type type = { componentId };
        hash |= Hash(type);
        archetype = world->GetArchetype(hash);
        for(unsigned char index = 0; index < archetype->type.size(); index++) {
            if(archetype->type[index] == componentId)
                archetype->components[index].push_back(new ComponentType());
            else {
                archetype->components[index].push_back(data.front());
                data.pop();
            }
        }
        record.archetype = archetype;
        archetype->entities.push_back(id);
        return *this;
    }

    template<typename ComponentType, typename... Arguments>
    Entity& Entity::Set(Arguments&&... arguments){
        if(!Has<ComponentType>())
            Add<ComponentType>();
        Record& record = world->entities.at(id);
        Archetype* archetype = record.archetype;
        unsigned char componentId = Component<ComponentType>();
        for(unsigned char index = 0; index < archetype->type.size(); index++) {
            if(archetype->type[index] == componentId)
                archetype->components[index][record.index] = new ComponentType{arguments...};
        }
        return *this;
    }

    template<typename ComponentType>
    Entity& Entity::Remove(){
        Record& record = world->entities.at(id);
        Archetype* archetype = record.archetype;
        unsigned char componentId = Component<ComponentType>();
        std::queue<void*> data;
        for(unsigned char index = 0; index < archetype->type.size(); index++) {
            if(archetype->type[index] == componentId) {
                (*(ComponentType*)archetype->components[index][record.index]).~ComponentType();
            } else 
                data.push(archetype->components[index][record.index]);
            archetype->components[index][record.index] = archetype->components[index][archetype->entities.size() - 1];
            archetype->components[index].pop_back();
        }
        archetype->entities[record.index] = archetype->entities[archetype->entities.size() - 1];
        archetype->entities.pop_back();
        world->entities.at(archetype->entities[record.index]).index = record.index;
        TypeHash hash = Hash(archetype->type);
        Type type = { componentId };
        hash ^= Hash(type);
        archetype = world->GetArchetype(hash);
        for(unsigned char index = 0; index < archetype->type.size(); index++) {
            archetype->components[index].push_back(data.front());
            data.pop();
        }
        record.archetype = archetype;
        archetype->entities.push_back(id);
        record.index = archetype->entities.size();
        return *this;
    }

    template<typename ComponentType>
    bool Entity::Has(){
        Archetype* archetype = world->entities.at(id).archetype;
        unsigned char componentId = Component<ComponentType>();
        for(unsigned char index = 0; index < archetype->type.size(); index++) {
            if(archetype->type[index] == componentId)
                return true;
        }
        return false;
    }

    template<typename ComponentType>
    ComponentType& Entity::Get(){
        Record& record = world->entities.at(id);
        Archetype* archetype = record.archetype;
        unsigned char componentId = Component<ComponentType>();
        for(unsigned char index = 0; index < archetype-> type.size(); index++){
            if(archetype->type[index] == componentId)
                return *(ComponentType*)archetype->components[index][record.index];
        }
        throw std::invalid_argument("Component does not exist");
    }

    template<typename... ComponentTypes, typename... Arguments, typename Function>
    void System(World& world, Function function, Arguments&&... arguments){
        TypeHash hash = Hash({Component<ComponentTypes>()...});
        for(auto& pair : world.archetypes){
            std::bitset<64> temporary = hash;
            if((temporary &= pair.first) == hash){
                for(EntityId id : pair.second->entities)
                    function(pair.second->GetChunk<ComponentTypes>()[id]..., arguments...);
            }
        }
    }

    template<typename ComponentType>
    ComponentType* Archetype::GetChunk(){
        unsigned char componentId = Component<ComponentType>();
        for(unsigned char index = 0; index < type.size(); index++){
            if(type[index] == componentId)
                return (ComponentType*)(*components[index].data());
        }
        throw std::invalid_argument("Chunk does not exist");
    }
    
    EntityId World::CreateEntity() {
        EntityId id;
        if(removed.empty())
            id = entityIndex++;
        else {
            id = removed.front();
            removed.pop();
        }
        Record record;
        record.archetype = GetArchetype(TypeHash());
        record.index = record.archetype->entities.size();
        record.archetype->entities.push_back(id);
        entities.insert({id, record});
        return id;
    }

    Archetype* World::GetArchetype(TypeHash hash) {
        std::unordered_map<TypeHash, Archetype*>::iterator iterator = archetypes.find(hash);
        if(iterator != archetypes.end())
            return iterator->second;
        Type type;
        for(unsigned char index = 0; index < hash.size(); index++) {
            if(hash[index] == 1)
                type.push_back(index);
        }
        Archetype* archetype = new Archetype();
        archetype->type = type;
        archetype->components.resize(type.size());
        archetypes.insert({hash, archetype});
        return archetype;
    }

    World::~World(){
        entities.clear();
        for(unsigned int index = 0; index < archetypes.size(); index++)
            delete archetypes[index];
        archetypes.clear();
        for(unsigned int index = 0; index < removed.size(); index++)
            removed.pop();
        shouldStop = true;
    }

    bool World::Run(){
        return !shouldStop;
    }
}