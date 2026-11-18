# Java Edition 1.12.2 references

The user's installed vanilla `minecraft-1.12.2-client.jar` was inspected locally
with CFR 0.152 and Forge MCP 1.12.2 SRG class mappings. Decompiled Java remains in
the ignored build directory; the client contains C++ implementations of the
following behavior rather than the Java runtime.

* `bax` / `WorldGenTrees`: small oak, height `4 + nextInt(3)`, clearance checks,
  grass/dirt support, four crown layers, random corner trimming and vertical oak
  log metadata. Random uses Java's 48-bit LCG and bounded nextInt. No vines/cocoa.
* `vp` / `EntityLivingBase`: ordinary land/air movement at 20 Hz, jump `0.42f`,
  gravity `0.08`, vertical drag `0.98f`, horizontal drag `0.91f`, block
  slipperiness `0.6`, acceleration compensation `0.16277136 / friction^3`,
  air control `0.02`, sprint multiplier `1.3` and sprint-jump boost `0.2`.
  Input damping `0.98`, small-motion cutoff `0.003` and jump cooldown are applied.
  The C++ collision solver uses Y/X/Z clipping, step attempts up to 0.6 and
  sneak edge protection. It currently queries full voxel collision boxes.
* `bpx`, `bqj`, `brq` / `ModelBiped`, `ModelPlayer`, `ModelBox`: classic Steve
  dimensions, pivots, six-face skin rectangles, outer layers and opposite limb
  swings (`0.6662`, leg amplitude `1.4`). Uses the player render scale `0.9375`.

The texture importer reads `assets/minecraft/textures/entity/steve.png` from the
local JAR into the ignored resource pack. Local Windows/Switch packages include
it; GitHub builds use authored fallback art unless a resource pack is supplied.

Not covered: exact vanilla biome/seed generation, lava swimming, ladders,
elytra, potions, combat or networking. The player is not a full Java entity.
Tree anchors use this project's deterministic grid, while tree shapes and Java
random operations follow the inspected small-oak generator. Clipped crowns are
reconstructed from the same world coordinates on both sides of chunk seams.

Verification covers reference jump recurrence, walking/sprinting speeds,
collisions and sneaking at ledges, oak support/obstruction/canopy/seams, Java random,
mouse-wheel wrapping, and rendered Steve/trees in the graphics smoke test.

* `aec` / `InventoryPlayer` and `agi` / `ContainerPlayer`: 36 main slots,
  nine-slot hotbar, stack cap 64, the original slot coordinates (8/84/142),
  2x2 input at 98/18 and output at 154/28, with slot splitting, merging,
  cursor swapping, quick transfer and return-on-close semantics. C++ uses an
  independent item model; selected recipes are explicitly ported, not a Java VM.
* `afp` / `FoodStats`: 20 food, initial saturation 5, exhaustion threshold 4,
  ten-tick saturated healing and 80-tick ordinary healing/starvation. Normal
  starvation stops at one HP. Eating takes 32 ticks. Fall damage follows
  ceil(fallDistance - 3), without enchantment/effect modifiers.
* `bsa` / `PlayerControllerMP`: creative instant break/reach 5, survival reach
  4.5, break progress and a five-tick break delay. The C++ block table implements
  a subset of hardness, drop and pickaxe harvest-tier rules.

Inventory/container GUI images, hearts, hunger and item icons are read from the
user's installed 1.12.2 resource JAR. Repository fallback art remains authored.
Survival/creative inventories are deliberately separate to preserve gathered
supplies when changing mode; that convenience differs from vanilla's shared bag.
There is no armor system, furnace or complete recipe set. Persistence uses a separate native format.

* `aqe` / `BlockDynamicLiquid`, `aru` / `BlockLiquid`: water levels 0 source,
  1-7 horizontal and 8-15 falling; five-game-tick scheduling, downward priority,
  horizontal decay by one, source regeneration with two adjacent sources and
  support, removal decay, and a four-cell minimum-cost search for downhill routes.
  Height is `1 - (effectiveLevel + 1)/9`; falling uses effective level zero.
  The renderer averages corner heights with source/falling weighting and draws
  water after opaque blocks with depth writes disabled. Active updates are bounded
  to 1024 scheduled entries per game tick; saturated workloads may advance late.
  Unloaded boundaries wait and are revisited on chunk arrival.
* `vp` / `EntityLivingBase` water travel: acceleration .02, input damping .98,
  motion drag .8, gravity .02, swim jump .04, obstacle shore boost .3. `BlockLiquid`
  flow gradients normalize to an entity acceleration of .014 per tick. The C++
  movement solver still uses its swept voxel clipping. Water clears fall distance;
  air is 300 ticks, then drowning deals two HP every 20 ticks. No Depth Strider.
* `buq` / `EntityRenderer.hurtCameraEffect`: Z tilt in degrees
  `-sin(((hurtTime - partialTicks)/10)^4 * PI) * 14`. This client has no directional
  combat yet, so attacked-at-yaw is zero. Tilt changes the rendered camera only.
  Underwater FOV uses the original 60/70 multiplier.
* `bmp` / `GuiContainerCreative` and `ahp` / `CreativeTabs`: 195x136 body,
  five rows of nine catalog slots, hotbar at 112, six top/six bottom tabs and
  a scrollbar. Categories use the implemented item subset, with empty sections
  for unsupported content. The importer reads unmodified `tab_items.png` and
  `tabs.png`; Search and the player bag share this body's layout.
* `bok` / `GuiWorldSelection`, `boi` / `GuiCreateWorld`: title Singleplayer ->
  saved-world list -> creation with world name, seed and game mode. Numeric seeds
  parse signed 64-bit values; text seeds use Java UTF-16 String.hashCode. An empty
  seed chooses a time-based seed. The C++ world generator and RLE save format are
  independent of Java terrain and Anvil. Inventories and chunk edits persist;
  chunk snapshots are file-atomic with fallback backups, not a transactional database.

Water tests cover flow/drainage/source regeneration and frontier resumption;
movement tests check the water recurrence, current impulse, collision and hurt tilt.
The graphics smoke covers water rendering, source pickup, survival bucket pouring,
underwater/drowning, tabs/search, creation fields, world mode, edited chunk eviction
and world reopen with both bags, pose and vitals.
