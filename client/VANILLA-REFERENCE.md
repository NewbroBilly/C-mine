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

Not covered: exact vanilla biome/seed generation, water/lava swimming, ladders,
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
There is no armor system, furnace, complete recipe set or world persistence yet.
