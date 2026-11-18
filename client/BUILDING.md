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

WASD moves; mouse looks; Space/Left Ctrl moves vertically in creative mode.
Left click removes, right click places, middle click picks a block.
1-9 selects the hotbar, I opens inventory, Tab toggles survival, F11 fullscreen.
Gravity/collisions are enabled by default. Space jumps; Shift sprints.
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
separate grass/log faces. Existing GUI and light maps remain from the supplied
source. Imported files stay local and are ignored by Git.

## Nintendo Switch homebrew

Requires devkitPro/devkitA64, libnx, switch-sdl2, switch-glad and switch-glm.
Run `make -f Makefile.switch`. The output is build/switch/MineCppClone.nro;
its shaders and supplied textures are embedded as RomFS.
The GitHub workflow also builds this target. Hardware behavior requires a Switch
test; successful cross-compilation alone does not confirm it.

Left stick moves; right stick looks; A jumps/ascends; B descends; click left stick
to sprint; X opens inventory; Y toggles survival; L/R changes hotbar slot;
ZR removes; ZL places; Plus pauses/resumes; right-stick click changes camera.
Menus: D-pad Up/Down selects, A confirms. Inventory selection currently needs touch or
mouse input; the default hotbar is available with L/R.

## Graphics smoke test

`MineCppClone.exe --smoke-test` checks title rendering, surface spawn,
pause freezing physics/chunk generation, third-person rendering, block placement
and removal, and return-to-title/restart. It checks GL errors and writes BMP frames.
The cave regression scene also checks the bedrock floor and torch geometry.
`client_movement_tests` checks falls, jumps, walls, ceilings, high-speed collision,
torch bounds, axis-aligned block selection and the 4.5-block reach limit.
It exits after 105 frames and requires a real OpenGL context.

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
The lowest terrain layer is unbreakable bedrock, and unloaded chunk boundaries
keep their exposed faces until the neighboring chunk arrives.
