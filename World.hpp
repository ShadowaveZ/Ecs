#pragma once

#include <queue>
#include <unordered_map>
#include "Declarations.hpp"

namespace Ecs {
    struct World {
        std::unordered_map<EntityId, Record> entities;
        std::unordered_map<TypeHash, Archetype*> archetypes;
        std::queue<EntityId> removed;

        ~World();
    };

    World::~World(){
        entities.clear();
        for(unsigned int index = 0; index < archetypes.size(); index++)
            delete archetypes[index];
        archetypes.clear();
        while(!removed.empty())
            removed.pop();
    }
}