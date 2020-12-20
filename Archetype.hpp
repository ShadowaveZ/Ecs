#pragma once

#include "Declarations.hpp"

namespace Ecs {
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
        return nullptr;
    }
}