#pragma once
#include "../core/Types.h"

namespace mc {

enum class Biome : uint8_t {
    Ocean, DeepOcean, FrozenOcean, WarmOcean, River, FrozenRiver, Beach,
    Plains, Forest, BirchForest, Taiga, SnowyTaiga, SnowyPlains, Desert, Savanna, Jungle, Swamp,
    Mountains, SnowyPeaks, MushroomFields, Badlands, Count
};

struct BiomeInfo {
    const char* name; const char* nameRu;
    float temperature;      // vanilla-like (0.8 plains)
    uint32_t grass, foliage, water;
    bool snowy;
};

inline const BiomeInfo& biomeInfo(Biome b) {
    static const BiomeInfo T[(int)Biome::Count] = {
        {"Ocean", "Океан", 0.5f, 0x8EB971, 0x71A74D, 0x3F76E4, false},
        {"Deep Ocean", "Глубокий океан", 0.5f, 0x8EB971, 0x71A74D, 0x3F76E4, false},
        {"Frozen Ocean", "Замёрзший океан", 0.0f, 0x80B497, 0x60A17B, 0x3938C9, true},
        {"Warm Ocean", "Тёплый океан", 0.8f, 0x8EB971, 0x71A74D, 0x43D5EE, false},
        {"River", "Река", 0.5f, 0x8EB971, 0x71A74D, 0x3F76E4, false},
        {"Frozen River", "Замёрзшая река", 0.0f, 0x80B497, 0x60A17B, 0x3938C9, true},
        {"Beach", "Пляж", 0.8f, 0x91BD59, 0x77AB2F, 0x3F76E4, false},
        {"Plains", "Равнины", 0.8f, 0x91BD59, 0x77AB2F, 0x3F76E4, false},
        {"Forest", "Лес", 0.7f, 0x79C05A, 0x59AE30, 0x3F76E4, false},
        {"Birch Forest", "Берёзовый лес", 0.6f, 0x88BB67, 0x6BA941, 0x3F76E4, false},
        {"Taiga", "Тайга", 0.25f, 0x86B783, 0x68A464, 0x287082, false},
        {"Snowy Taiga", "Снежная тайга", -0.5f, 0x80B497, 0x60A17B, 0x3D57D6, true},
        {"Snowy Plains", "Снежные равнины", 0.0f, 0x80B497, 0x60A17B, 0x3D57D6, true},
        {"Desert", "Пустыня", 2.0f, 0xBFB755, 0xAEA42A, 0x3F76E4, false},
        {"Savanna", "Саванна", 1.2f, 0xBFB755, 0xAEA42A, 0x3F76E4, false},
        {"Jungle", "Джунгли", 0.95f, 0x59C93C, 0x30BB0B, 0x14A2C5, false},
        {"Swamp", "Болото", 0.8f, 0x6A7039, 0x6A7039, 0x617B64, false},
        {"Mountains", "Горы", 0.2f, 0x8AB689, 0x6DA36B, 0x3F76E4, false},
        {"Snowy Peaks", "Снежные вершины", -0.7f, 0x80B497, 0x60A17B, 0x3D57D6, true},
        {"Mushroom Fields", "Грибное поле", 0.9f, 0x55C93F, 0x2BBB0F, 0x3F76E4, false},
        {"Badlands", "Бесплодные земли", 2.0f, 0x90814D, 0x9E814D, 0x3F76E4, false},
    };
    return T[(int)b];
}
inline bool isOceanBiome(Biome b) { return b == Biome::Ocean || b == Biome::DeepOcean || b == Biome::FrozenOcean || b == Biome::WarmOcean; }

} // namespace mc
