#pragma once

#include "Declarations.hpp"
#include "Archetype.hpp"

namespace Ecs {
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
}