# Minecraft 1.12.2 C++ Edition

A cross-platform C++20 project beginning with a headless core prototype. The target is Java Edition 1.12.2 behavior. This repository does not include Minecraft code, assets, or resource packs.

## Current milestone

The first increment establishes a buildable core boundary and a small test harness:

- 16 x 16 x 256 chunk storage with block metadata.
- Lazy loading of chunks across positive and negative coordinates.
- Flat test terrain used only by this prototype.
- A Java-compatible 48-bit random number generator primitive.
- Basic collision, gravity, jumping, and scheduled water/lava flow approximations.
- Surface spawn queries that only return a two-block-high position above a solid top block.
- Vanilla 1.12.2 block texture paths for a future renderer/resource-pack loader.
- A headless executable that reports a small simulation state.

These pieces are scaffolding. World generation, physics, fluids, and game rules are not yet faithful implementations of Java 1.12.2. The seed is stored by World, but the current flat test terrain does not use it.

## Build

Requires CMake 3.20+ and a C++20 compiler.

    cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
    cmake --build build --config Release
    ctest --test-dir build --output-on-failure

Run the diagnostic executable with an optional seed:

    ./build/minecraft_1122 --seed 1234

## Current progress

Percentages are rough scope estimates for working, version-matched features. Placeholder scaffolding does not count as completed gameplay.

| System | Ready |
|---|---:|
| Engine core | 3% |
| Generation | 1% |
| Physics | 1% |
| Water and lava | 1% |
| Game modes | 0% |
| Mobs | 0% |
| Graphics | 0% |
| Multiplayer | 0% |
| PC version | 0% |
| Console preparation | 1% |

## Roadmap

1. Match the Java 1.12.2 chunk, biome, terrain, cave, and feature generation pipeline against reference seeds.
2. Add block-state and metadata definitions, scheduled block updates, fluid edge cases, and movement parity checks.
3. Add mesh extraction and a renderer behind a graphics API boundary, then a desktop platform and input backend.
4. Add inventory, recipes, survival rules, entities, saves, and resource-pack loading.
5. Add a server-authoritative protocol and dedicated-server executable.
6. Add additional platform and graphics backends only where the required SDKs and distribution permissions are available.

The main missing milestone before a visible PC build is the renderer and platform window/input layer. No console, networking, persistence, or full-game feature is implemented yet.

## Textures

The core exposes `mc::texturePath(Block)` with the standard 1.12.2 paths, for example
`textures/blocks/stone.png`. The project does not redistribute Mojang's copyrighted
resource pack. A renderer can point these paths at a legally obtained 1.12.2 client
resource pack when that layer is added.

## Surface spawning

`World::findSurfaceSpawn(x, z)` scans the loaded column from the top down and returns
the feet position only when the block below is solid and the feet/head cells are air.
It scans the chunk storage directly, so repeated spawn checks do not call `get()` for
all 256 levels or create neighboring chunks.
