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
Back to Title discards this session; it does not save.
Edits are not persisted: this imported client has no world-save implementation.

## Original 1.12.2 block textures

The included atlas is from the supplied source; its provenance is unverified.
To use original block textures from a local 1.12.2 client JAR or resource ZIP:

```
python -m pip install Pillow
python tools/import_textures.py CLIENT.jar --output dist/MineCppClone/resource_pack
```

This imports the blocks supported by the client, with grass/foliage tint and
separate grass/log faces, inventory/crafting GUI images, item icons and Steve.
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
creative catalog pages. L/R changes hotbar slot outside inventories.

## Graphics smoke test

`MineCppClone.exe --smoke-test` checks title rendering, surface spawn,
pause freezing physics/chunk generation, third-person rendering, block placement
and removal, and return-to-title/restart. It checks GL errors and writes BMP frames.
The cave regression scene also checks the bedrock floor and torch geometry.
`client_movement_tests` checks falls, jumps, walls, ceilings, high-speed collision,
torch bounds, axis-aligned block selection and the 4.5-block reach limit.
It exits after 340 frames and requires a real OpenGL context.

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

E or I opens inventory; Escape/E/I closes it. Survival starts empty, with
36 main slots (nine in the hotbar), offhand storage, and 2x2 personal crafting.
Left click picks up, merges or swaps a stack. Right click takes the larger half
or deposits one. Shift-click transfers between the main inventory and hotbar.
Number keys swap the hovered slot with a hotbar slot. Arrow keys/Enter also work.
Closing returns carried/crafting items; overflow drops into the world.
Craft logs into four planks, two vertical planks into sticks, or four planks into
a crafting table. Place and right-click the table for 3x3 crafting: three planks
above two sticks make a wooden pickaxe. Cobblestone, iron and diamond pickaxes,
torches, bread, sandstone and selected block compression recipes are supported.
Shift-click the output to craft as much as inventory capacity allows.

Select Survival or Creative at the title screen. Tab/Y or the pause menu changes
mode in the current world, preserving separate survival and creative inventories.
Survival spends blocks only after successful placement. Hold LMB/ZR to mine;
hardness, harvest tier, tool speed and durability affect mining and loot. Ground
items have gravity, a pickup delay, automatic collection and a five-minute life.
The HUD shows 20 health and 20 hunger points, fall damage above three blocks,
exhaustion, saturation, regeneration and Normal-difficulty starvation (to one HP).
Hold RMB/ZL for 1.6 seconds to eat an apple/bread. Death drops the inventory;
Respawn restores health/food at the surface without replacing the current world.
Creative has free items, instant breaking, no item consumption/damage/hunger,
and collision-aware flight (double Space/A; Space/A up, Shift/B down).

Creative has 47 building blocks (including bedrock and the crafting table) and
19 resource/tool/food items. P/R changes the catalog page; Shift-click gives a
full stack to the selected hotbar slot. Normal catalog clicks take a cursor item;
click again for a full stack, or press 1-9 over a catalog item to fill that slot.
Original inventory/crafting backgrounds, icons and item textures are imported
from the local 1.12.2 JAR. The creative background is extended to six catalog rows.

Current limits: no world/inventory save files, mobs/combat, armor equipment,
furnace/smelting, full recipe registry, farming, fluids or multiplayer. The armor
slots are part of the original background; offhand is storage only. Sand/gravel
are static and soul sand has no slowdown. This is a C++ subset of 1.12.2 behavior.

`include/extra_blocks.def` is shared by the block registry and the texture importer.
Local packages import each face from the user's 1.12.2 JAR; the repository contains
custom fallback tiles for builds without the resource pack. The extended smoke
test covers settings navigation/persistence, all block mesh/place/remove paths,
glowstone light and keyboard/controller-style inventory selection.

`client_items_tests` checks stack conservation, partial transfers, failed crafting,
recipes, tool tiers, food and fall damage. The GL smoke additionally checks
separate inventories, actual block spending, held-button mining, item drops/pickup,
crafting screens, death/respawn, creative flight and timed food consumption.
