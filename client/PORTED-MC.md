# Generation and Russian interface (0.7.0)

Selected generation sources were supplied by the user in
`C:/Users/User/Desktop/mc/src` and copied on 2026-11-18 into `ported_mc/`:

- `core/Types.h`, `Random.h`, `Noise.h`, `Noise.cpp`;
- `world/Block.h`, `Block.cpp`, `Chunk.h`;
- `terrain/Biome.h`, `TerrainGenerator.h/.cpp`, `Decorator.h/.cpp`.

These are the supplied clone's generator, not Mojang's original source code.
The files above are preserved as supplied. `src/imported_*.cpp` builds them
on desktop and Switch; `src/generation.cpp` adapts their output to our engine.
No external game loop, renderer, audio, threading, player physics or save system
is incorporated. The external project's build fixes are not needed by this port.
No license statement for these supplied sources was found in that folder;
this document records provenance rather than assigning them a new license.

## Compatibility and limits

New worlds use generator 1: the imported climate/biome, density, cave, aquifer,
ore and decoration algorithms. CMINE metadata version 3 stores that choice.
Versions 1 and 2, and upgraded existing worlds, keep generator 0. Block IDs
and the CMC1 chunk representation are unchanged; new block IDs are appended.

Our height remains 256. Imported Y -64..-1 is sampled into Y 0..31;
source Y 0..159 is translated to Y 32..191 without scaling, and source
Y 160..317 is sampled into Y 192..255. This preserves the block scale of
ordinary hills and trees while compressing the deepest caves and highest peaks.
The sea surface is around Y 95. The bottom layer always remains bedrock.
Generation is deterministic within this version. The 36-chunk terrain cache
contains pristine terrain, never player edits or already decorated neighbors.
Features replay the original 3x3 neighborhood streams, preserving crowns across
chunk boundaries. Water is written as settled sources with no initial flood.

Only supported blocks enter our world: deep ores map to their normal variants;
deepslate, tuff and copper ore map to stone; lava maps to obsidian; unsupported
terracotta colors use plain terracotta; leaves share our oak leaf appearance.
Unsupported flowers, snow layers and plants are omitted. Mushroom structures
use logs, and dungeon shells have no chest contents or functioning spawners.
Biome grass/foliage tinting, lava simulation, mobs and loot are not imported.
Ice, packed ice, spruce/birch logs, podzol and mycelium were added for terrain.
The logs can be mined and crafted into their respective planks.

## Russian language

Russian is the default; Settings → Language switches between Russian and
English and persists the choice. Menus, categories, supported item/block names,
tooltips and gameplay hints are translated. Creative search accepts both
languages, independently of the interface language. Names/search input,
backspace, clipping, centering and widths operate on UTF-8 characters.
The developer debug overlay retains its technical field names.

The Cyrillic bitmap glyphs come from the supplied `mc/assets/font/font.png`,
generated there from DejaVu Sans Mono. `tools/import_cyrillic_font.py` merges
them without resampling, preserving our ASCII glyphs. The font license is
included as `DEJAVU-LICENSE.txt`. Short item labels were cross-checked against
the user's installed 1.12.2 localization. Vanilla texture resources remain in
local resource packs and are not committed to this repository.

## Validation

Generation tests cover seed changes including upper 64-bit seed bits, negative
coordinates, climate diversity, forest crowns and reversed chunk loading,
bounded valid IDs, bedrock, caves, ores and settled water. Language tests cover
all supported item names, bilingual search, Cyrillic case folding, UTF-8 editing
and malformed input. Save tests cover legacy versions and Russian names with
the generator version. The integrated `--smoke-test` exercises language buttons,
new-world creation, Russian search, source water and save/reopen in OpenGL.
