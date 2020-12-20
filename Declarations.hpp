#pragma once

#include <bitset>
#include <vector>

namespace Ecs {
    using ComponentId = unsigned char;
    using EntityId = unsigned int;
    using Type = std::vector<unsigned char>;
    using TypeHash = std::bitset<64>;

    struct Archetype;
    struct Entity;
    struct Record;
    struct World;
}