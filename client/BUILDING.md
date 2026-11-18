# Graphical client

This client is adapted from the supplied mc-main source. See ORIGIN.md for
provenance; the original README and embedded author notices are retained.

## Windows

Requires CMake, a C++17 compiler, Python 3 and network access on first configure.
SDL2, GLM and GLAD are fetched at their specified release versions.

```
cmake -S . -B build/client -DMC_BUILD_CLIENT=ON -DCMAKE_BUILD_TYPE=Release
cmake --build build/client --config Release
cmake --install build/client --config Release --prefix dist/MineCppClone
```

Run MineCppClone.exe from the installed directory. Shaders and textures are
resolved relative to the executable. Requires OpenGL 3.3.

WASD moves; mouse looks; double Space toggles creative flight, Space/Shift ascends/descends.
Hold left click to mine; right click places/uses; middle click selects an existing block (creates it in creative).
Mouse wheel or 1-9 selects the hotbar, E/I opens inventory, Tab switches mode, F11 fullscreen.
Gravity/collisions are enabled by default. Space jumps; Ctrl sprints while moving forward; Shift sneaks and protects ledges.
Escape pauses/resumes (or closes inventory). Losing window focus also pauses.
F5 cycles first person, third person behind, and third person front; F7 wireframe.
The title menu uses the six panorama faces supplied in Original-Panorama-old.zip;
see ORIGIN-PANORAMA.md. Select buttons with mouse or Up/Down and Enter.
Singleplayer opens saved worlds. Select a world and Play, or Create New World:
enter a name/optional seed and choose Survival or Creative before creating.
Up/Down or wheel selects a saved world; Left/Right selects the action, Enter confirms.
Creation fields use Up/Down/Tab; names use the bundled font's printable ASCII set.
The Switch build opens the system keyboard when A confirms a name/seed field.
World edits, water levels, player pose/velocity, flight, health/food/air, dropped items,
hotbar and both inventories persist on chunk unload, every 30 active seconds,
Back to Title and normal exit. Pausing also freezes water.

Windows saves live in `%APPDATA%/C-mine/C-mine/saves/`; Switch uses
`sdmc:/switch/C-mine/saves/`. Each world has versioned metadata and compressed
chunk snapshots. Writes use temporary files and recoverable previous-file backups.
This is this client's save format; Java Anvil worlds cannot be imported.

## Original 1.12.2 block textures

The included atlas is from the supplied source; its provenance is unverified.
To use original block textures from a local 1.12.2 client JAR or resource ZIP:

```
python -m pip install Pillow
python tools/import_textures.py CLIENT.jar --output dist/MineCppClone/resource_pack
```

This imports the blocks supported by the client, with grass/foliage tint and
separate grass/log faces, all 32 frames of still/flowing water, inventory/crafting/creative GUI images and tabs, item icons and Steve.
Imported files stay local and are ignored by Git.

## Nintendo Switch homebrew

Requires devkitPro/devkitA64, libnx, switch-sdl2, switch-glad and switch-glm.
Run `make -f Makefile.switch`. The output is build/switch/MineCppClone.nro;
its shaders and supplied textures are embedded as RomFS.
The GitHub workflow also builds this target. Hardware behavior requires a Switch
test; successful cross-compilation alone does not confirm it.

Left stick moves; right stick looks; A jumps/ascends (double-tap to fly in creative); B crouches/descends; click left stick
to sprint; B sneaks in survival/descends in creative; X opens inventory; Y toggles survival; L/R changes hotbar slot;
ZR removes; ZL places; Plus pauses/resumes; right-stick click changes camera.
Menus: D-pad Up/Down selects, A confirms. In inventories, D-pad moves the
selection, A moves stacks, ZL splits/deposits one, L+A quick-moves; R switches
creative category tabs. In world selection, Up/Down changes world and Left/Right changes action. L/R changes hotbar slot outside inventories.

## Graphics smoke test

`MineCppClone.exe --smoke-test` checks title rendering, surface spawn,
pause freezing physics/chunk generation, third-person rendering, block placement
and removal, and return-to-title/restart. It checks GL errors and writes BMP frames.
The cave regression scene also checks the bedrock floor and torch geometry.
`client_movement_tests` checks falls, jumps, walls, ceilings, high-speed collision,
torch bounds, axis-aligned block selection and the 4.5-block reach limit.
It exits after 415 frames and requires a real OpenGL context. It also exercises
world creation/name/seed/mode, falling/spreading water, buckets, drowning,
hurt-camera roll, creative search/categories, chunk eviction/reload and a saved-world
reopen. Smoke saves/options are isolated from normal player data.
It also sweeps the camera through stacked water blocks to catch fog/FOV flicker
at internal block boundaries; only the exposed top surface ends submersion.
Fresh terrain is checked for prefilled source water and an empty fluid queue.

## PrismLauncher resource source

For an installed 1.12.2 instance, the vanilla resource JAR is usually in the shared
`PrismLauncher/libraries/com/mojang/minecraft/1.12.2/minecraft-1.12.2-client.jar`,
not inside the instance's `minecraft` folder. Pass that JAR to the importer above.
Supported block textures, including bedrock, are imported; the torch icon and
narrow shaft use separate atlas tiles. Existing saves/mods/resource packs in the
instance are not modified.

Switch can load imported atlases from `sdmc:/switch/C-mine/resource_pack/`.
Copy the supplied `switch` folder from the local Switch ZIP onto the SD card root.
The NRO has built-in fallback textures when that folder is absent.
The lowest terrain layer is unbreakable bedrock in survival, and unloaded chunk boundaries
keep their exposed faces until the neighboring chunk arrives.

## 1.12.2 behavior references

See VANILLA-REFERENCE.md for the inspected vanilla classes and implemented scope.
Land/air movement advances at 20 Hz and rendering interpolates between ticks.
Oak shapes follow the small-oak generator; candidate density and terrain remain
this project's generator, so seeds do not reproduce vanilla worlds.
The imported player skin is Steve (64x64), with classic four-pixel arms,
ModelBox UV layout and the outer skin layers. The bundled fallback is custom art.

## Settings and expanded block inventory

Options is available from the title screen and pause menu. Click a setting or
select it with Up/Down and use Left/Right or Enter. Escape/Done returns to the
previous menu; returning from in-game settings keeps the game paused.
FOV, sensitivity, render distance (2-8 chunks), VSync, ambient occlusion,
fullscreen and mouse Y inversion apply immediately and persist. Windows stores
preferences in SDL's C-mine application-data folder; Switch uses
`sdmc:/switch/C-mine/options.txt`. F11 and F6 update the same preferences.

E or I opens inventory; Escape/E/I closes it. In creative Search, letters type a query and Escape closes. Survival starts empty, with
36 main slots (nine in the hotbar), offhand storage, and 2x2 personal crafting.
Left click picks up, merges or swaps a stack. Right click takes the larger half
or deposits one. Shift-click transfers between the main inventory and hotbar.
Number keys swap the hovered slot with a hotbar slot. Arrow keys/Enter also work.
Closing returns carried/crafting items; overflow drops into the world.
Craft logs into four planks, two vertical planks into sticks, or four planks into
a crafting table. Place and right-click the table for 3x3 crafting: three planks
above two sticks make a wooden pickaxe. Wood, stone, iron, gold and diamond
pickaxes, axes, shovels, hoes and swords use their 1.12.2 crafting patterns
(including mirrored axes/hoes). Shears, flint and steel, fishing rods, clocks,
compasses, tool repair, torches, bread, sandstone and selected block compression
recipes are supported.
Shift-click the output to craft as much as inventory capacity allows.

Choose Survival or Creative when creating a world in Singleplayer. Tab/Y or the pause menu changes
mode in the current world, preserving separate survival and creative inventories.
Survival spends blocks only after successful placement. Hold LMB/ZR to mine;
hardness, harvest tier, tool speed and durability affect mining and loot. Ground
items have gravity, a pickup delay, automatic collection and a five-minute life.
The HUD shows 20 health and 20 hunger points, fall damage above three blocks,
exhaustion, saturation, regeneration and Normal-difficulty starvation (to one HP).
Hold RMB/ZL for 1.6 seconds to eat food (apples, bread, vegetables, fish and meat). Death drops the inventory;
Respawn restores health/food at the surface without replacing the current world.
Creative has free items, instant breaking, no item consumption/damage/hunger,
and collision-aware flight (double Space/A; Space/A up, Shift/B down).

Creative has 104 blocks plus 89 items, including empty/water buckets. The original
five-by-nine catalog has 12 tabs: Building, Decoration, Redstone, Transportation,
Miscellaneous, Search, Food, Tools, Combat, Brewing, Materials and Player Inventory.
Tabs with no implemented items are empty. Click a tab, press P or [ / ], or use
Switch R to cycle; the wheel/scrollbar scrolls long lists. Search filters names as
you type (Backspace erases; Escape closes). P is text inside Search; ] still changes
the tab. Shift-click gives a full stack to the selected hotbar slot; normal catalog
click takes an item, clicking again fills the cursor stack. The Player Inventory
tab exposes the current creative bag. As before, survival and creative bags are
separate, unlike Java's shared bag.

Natural water is generated immediately as settled sources below sea level;
chunk loading does not schedule the entire ocean for simulation. Player edits
wake nearby water, and saved disturbed sources/flow resume when chunks reload.
Terrain height is evaluated once per column instead of once per block.
Water sources can also be placed with a
water bucket from Miscellaneous/Search. In survival craft a bucket from three iron
ingots in a V in the 3x3 table; right-click a source to fill, or a surface to pour.
Flowing water cannot be collected. Water updates every five game ticks, falls
before spreading, travels up to seven horizontal cells, seeks downhill routes
within four cells, regenerates sources between two sources with support and drains
when its supply disappears. Loaded chunk seams resume flow when neighbors arrive.
Space/A swims upward; currents push the player, water resets fall distance, and
submerged eyes consume 300 air ticks before two drowning damage every 20 ticks.
The view uses blue underwater fog, narrower underwater FOV, animated translucent
water and the original 1.12.2 hurt-time camera tilt.

## Tools and underground ores

All five tool materials retain their own mining speed, harvest tier and durability.
Gold is fast but has the wooden harvest tier. Axes speed wood, shovels speed soil,
and shears collect leaves/grass and speed wool. Swords are in Combat; the other
tools and utility items are in Tools. Breaking blocks with a sword uses two
durability, while hoes/rods/igniters wear on successful use. Two matching tools can
be repaired together in a crafting grid, with the 5% durability bonus.

Right-click exposed dirt/grass with a hoe for farmland, or grass with a shovel for
a path. These soil blocks currently use full voxel collision; crops, hydration
and trampling are not implemented. Flint and steel starts an animated, light-emitting
fire that causes contact damage and expires after 30 active seconds. Water
extinguishes it; fire does not spread or burn down neighboring blocks.

Aim the fishing rod at water and right-click to cast. A visible float and line
remain in the water; right-click during the `Bite!` window to catch a raw fish.
Fishing currently uses a simple 5-12 second waiting period and fish-only loot,
without enchantments, weather or the full vanilla treasure/junk tables. Changing
the held item or moving more than 32 blocks away cancels the cast.

Clock faces track the 20-minute world day; compass needles point toward the saved
world spawn. Time pauses with the game and persists with the spawn position.
Metadata version 2 reads version 1 worlds, retaining all existing item/block IDs.

New terrain contains coal, iron, gold, redstone, diamond, lapis and emerald ore.
Veins use overlapping ellipsoids and seeded randomness: coal has 20 attempts
below Y=128, iron 20 below 64, gold 2 below 32, redstone 8 below 16, diamond 1
below 16, and lapis 1 with a triangular distribution centered around Y=16.
Vein ellipsoids can extend slightly past the sampled height. Emeralds use 3-8
single-block attempts between Y=4 and 31; there are no biome restrictions yet.
Only stone is replaced: caves, water, bedrock and surface soils are preserved.
Veins continue across chunk boundaries independently of chunk loading order.
Existing saved chunk snapshots are kept; explore new chunks for ores in old worlds.
This generator is deterministic but does not reproduce Java worlds from the same seed.

Current limits: no mobs/combat, armor equipment, furnace/smelting, full recipe
registry, crops, lava or multiplayer. The armor slots are part of the original
background; offhand is storage only. Sand/gravel are static and soul sand has no
slowdown. Liquid collision uses voxel terrain; no depth-strider, boats, lava/water
reactions or special redstone/door fluid interactions. Terrain/biomes and save
files are this client's own implementation, not vanilla Java worlds.

`include/extra_blocks.def` is shared by the block registry and the texture importer.
Local packages import each face from the user's 1.12.2 JAR; the repository contains
custom fallback tiles for builds without the resource pack. The extended smoke
test covers settings navigation/persistence, all block mesh/place/remove paths,
glowstone light and keyboard/controller-style inventory selection.

`client_items_tests` checks stack conservation, partial transfers, failed crafting,
recipes, tool tiers, food and fall damage. The GL smoke additionally checks
separate inventories, actual block spending, held-button mining, item drops/pickup,
crafting screens, death/respawn, creative flight and timed food consumption.

`client_water_tests` covers update delay, spread/range, waterfalls, drainage, source
regeneration, downhill routing and unloaded frontiers. `client_saves_tests` covers
player/chunk roundtrips, isolated worlds, negative coordinates, truncated files,
backup recovery, Java seed hashing and creative category/search coverage.

## Flight, isometric icons and hand animations (0.6.0)

Creative flight uses the same 20 Hz/interpolated movement path as walking, with
swept collisions. Double Space/A toggles flight, Space/A ascends, Shift/B descends;
Ctrl while moving forward sprints. Horizontal fly acceleration is .05 (.1 while
sprinting), input damping .98 and drag .91. Vertical input adds .15 blocks/tick,
with .6 drag. Releasing controls brakes into a hover; descending onto ground
turns flight off. Climbing no longer reduces horizontal speed.

Hotbar, both inventories, creative tabs, crafting output and cursor use cached
cube meshes with individual top/side/bottom textures, face lighting and the
1.12.2 GUI transform (30/225/0 degrees, scale .625). Torch and grass remain flat
item sprites, matching the generated item models. Geometry is uploaded once,
not rebuilt per slot or when a block is placed.

The first-person view now shows the held block/item or Steve's empty right hand.
Mining/placing triggers a six-tick swing; item switches lower/raise the held model,
food uses the 32-tick eating curve, and walking bobs the camera/held object.
Third-person attacks also swing the arm. Ten imported destruction stages overlay
full cubes while mining. These are C++ adaptations of the original timing/curves:
flat item sprites have no extruded pixel edges, and animations cover the main
hand only. There is no offhand-use, bow, shield or entity-combat animation yet.

The added catalog includes granite/diorite/andesite and polished variants,
jungle/acacia/dark-oak planks/logs, all wool/concrete colors, coal blocks, red
sand/sandstone, terracotta, hay, melon, pumpkin, purpur, prismarine and sea lanterns.
New recipes include matching log/planks, polished stone, coal/hay/nugget
packing and unpacking, quartz/red sandstone, paper, books, sugar, bone meal,
blaze powder, magma cream, ender eyes and pumpkin pie. Foods use their vanilla
nutrition/saturation values. Resources can be stored/crafted; planting, portals,
teleporting pearls, brewing effects and cooking/smelting are not implemented.
New logs remain upright; hay/farmland/path use full-cube geometry. Animated
prismarine/sea-lantern textures currently display their first resource frame.

Existing save IDs are retained. Static item icons occupy indices 0-159; clock,
compass and cast-rod frames occupy 160-224, avoiding catalog collisions.
The GL smoke compares integrated flight at 10/20/60/144 FPS and captures the expanded
catalog, held block/tool/empty hand, swings, eating and mining cracks.

## Hand and flight-toggle fixes (0.6.1)

The empty right hand uses the 1.12.2 arm transform and shoulder pivot instead of
a centered mesh, restoring its lower-right resting pose and attack arc.
Double Space/A preserves velocity and the interpolation clock when entering or
leaving flight. WASD continues steering during a fall in both game modes, with
the existing vanilla air acceleration. SDL release/press events in one render
frame are accepted, while key-repeat events do not count as extra taps.
Tests cover terminal-speed air steering, both integrated falling modes, rapid
SDL taps, ignored repeats and momentum/interpolation across flight toggles.
