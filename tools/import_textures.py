"""Build the client's atlas from a local Minecraft 1.12.2 JAR/resource pack.

Usage: python tools/import_textures.py CLIENT.jar --output GAME/resource_pack
Requires Pillow. No downloads and no modifications to the source archive.
"""
import argparse
import csv
import re
import io
from pathlib import Path
import zipfile

from PIL import Image, ImageChops


def build_atlas(archive):
    atlas = Image.new("RGBA", (512, 512))

    def tile(name):
        # A few placeable blocks are entity textures in the vanilla pack.
        # Keep them in the block atlas so the catalog can use the same
        # three-face texture path as ordinary cubes.
        entity_tiles = {
            # The entity bed atlas contains separate pillow, side and base
            # regions. Keep those regions distinct for the world block model.
            "bed_red_top": ("assets/minecraft/textures/entity/bed/red.png", (0, 0, 16, 16)),
            "bed_red_side": ("assets/minecraft/textures/entity/bed/red.png", (0, 16, 16, 32)),
            "bed_red_bottom": ("assets/minecraft/textures/entity/bed/red.png", (16, 16, 32, 32)),
            "banner": ("assets/minecraft/textures/entity/banner_base.png", (2, 0, 18, 16)),
        }
        if name in entity_tiles:
            path, crop = entity_tiles[name]
            with archive.open(path) as source:
                image = Image.open(io.BytesIO(source.read())).convert("RGBA")
            return image.crop(crop).resize((16, 16), Image.Resampling.NEAREST)
        path = f"assets/minecraft/textures/blocks/{name}.png"
        with archive.open(path) as source:
            image = Image.open(io.BytesIO(source.read())).convert("RGBA")
        # Animated textures use vertical strips: take the first frame.
        return image.crop((0, 0, image.width, image.width)).resize((16, 16), Image.Resampling.NEAREST)

    def tinted(image, rgb):
        return ImageChops.multiply(image, Image.new("RGBA", image.size, (*rgb, 255)))

    mapping = {
        (0, 0): "stone", (1, 0): "dirt", (2, 0): "grass_side",
        (3, 0): "grass_top", (6, 0): "log_oak", (7, 0): "log_oak_top",
        (8, 0): "leaves_oak", (10, 0): "glass", (0, 1): "tallgrass",
        (1, 1): "torch_on", (6, 1): "planks_oak", (4, 1): "bedrock",
    }
    catalog = Path(__file__).resolve().parents[1] / "client/include/extra_blocks.def"
    for line in catalog.read_text().splitlines():
        match = re.match(r"(?:EXTRA|BASE)_BLOCK\((.*)\)", line)
        if match:
            fields = next(csv.reader([match[1]], skipinitialspace=True))
            for offset in (2, 5, 8):
                mapping[(int(fields[offset]), int(fields[offset+1]))] = fields[offset+2]
    grass = (145, 189, 89)
    for (x, y), name in mapping.items():
        image = tile(name)
        if name in ("grass_top", "tallgrass"):
            image = tinted(image, grass)
        elif name == "leaves_oak":
            image = tinted(image, (119, 171, 47))
        elif name == "grass_side":
            image = Image.alpha_composite(image, tinted(tile("grass_side_overlay"), grass))
        elif name == "torch_on":
            image = image.crop((7, 6, 9, 16)).resize((16, 16), Image.Resampling.NEAREST)
        atlas.paste(image, (x * 16, y * 16))
    # Keep all 32 vanilla water animation frames; the renderer selects at 8 fps.
    for row, name in [(5, "water_still"), (6, "water_flow"), (7, "fire_layer_0")]:
        strip = Image.open(io.BytesIO(archive.read(f"assets/minecraft/textures/blocks/{name}.png"))).convert("RGBA")
        frames = strip.height // strip.width
        for frame in range(32):
            i = frame % frames
            image = strip.crop((0, i*strip.width, strip.width, (i+1)*strip.width)).resize((16,16), Image.Resampling.BOX)
            atlas.paste(image, (frame*16, row*16))
    atlas.paste(tile("torch_on"), (48, 16))
    torch_top = tile("torch_on").crop((7, 6, 9, 8)).resize((16, 16), Image.Resampling.NEAREST)
    atlas.paste(torch_top, (32, 16))
    for stage in range(10):
        atlas.paste(tile(f"destroy_stage_{stage}"),(stage*16,160))
    return atlas


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("archive", type=Path)
    parser.add_argument("--output", required=True, type=Path)
    args = parser.parse_args()
    with zipfile.ZipFile(args.archive) as archive:
        atlas = build_atlas(archive)
        skin = archive.read("assets/minecraft/textures/entity/steve.png")
        gui_assets = {"inventory.png": "container/inventory.png",
                      "crafting.png": "container/crafting_table.png", "icons.png": "icons.png", "widgets.png": "widgets.png"}
        args.output.mkdir(parents=True, exist_ok=True)
        for target, source in gui_assets.items():
            (args.output / target).write_bytes(archive.read("assets/minecraft/textures/gui/" + source))
        for target, source in [("creative.png", "tab_items.png"), ("creative_tabs.png", "tabs.png")]:
            (args.output / target).write_bytes(archive.read("assets/minecraft/textures/gui/container/creative_inventory/" + source))
        entities={"pig":"pig/pig", "cow":"cow/cow", "sheep":"sheep/sheep", "sheep_fur":"sheep/sheep_fur", "zombie":"zombie/zombie", "creeper":"creeper/creeper", "chicken":"chicken", "skeleton":"skeleton/skeleton", "spider":"spider/spider", "enderman":"enderman/enderman", "slime":"slime/slime", "ghast":"ghast/ghast", "blaze":"blaze", "witch":"witch", "wither":"wither/wither", "bat":"bat", "rabbit":"rabbit/brown", "wolf":"wolf/wolf", "ocelot":"cat/ocelot", "horse":"horse/horse_brown"}
        for target,source in entities.items():
            (args.output/(target+".png")).write_bytes(archive.read("assets/minecraft/textures/entity/"+source+".png"))
        for material in ("leather","chainmail","iron","gold","diamond"):
            for layer in (1,2):
                target=f"{material}_layer_{layer}.png"
                image=Image.open(io.BytesIO(archive.read("assets/minecraft/textures/models/armor/"+target))).convert("RGBA")
                if material=="leather":
                    image=ImageChops.multiply(image,Image.new("RGBA",image.size,(160,101,64,255)))
                    overlay=Image.open(io.BytesIO(archive.read(f"assets/minecraft/textures/models/armor/leather_layer_{layer}_overlay.png"))).convert("RGBA")
                    image=Image.alpha_composite(image,overlay)
                image.save(args.output/target)
        items = Image.new("RGBA", (512, 512))
        catalog = Path(__file__).resolve().parents[1] / "client/include/items.def"
        entries = catalog.read_text().splitlines()
        if len(entries) > 768: raise ValueError("Static item atlas collides with animation region")
        for i, line in enumerate(entries):
            fields = next(csv.reader([re.fullmatch(r"ITEM\((.*)\)", line)[1]]))
            texture = fields[2]
            # Built-in entity item renderers do not have an item PNG. These
            # compact thumbnails use their original entity texture, not a
            # nonexistent textures/items/shield.png path.
            builtin = {"@shield": ("entity/shield_base", (2, 0, 12, 22)),
                       "@banner": ("entity/banner_base", (0, 0, 20, 40)),
                       "@bed": ("entity/bed/red", (6, 6, 22, 22)),
                       "@skull": ("entity/skeleton/skeleton", (8, 8, 16, 16))}
            if texture in builtin:
                name, crop = builtin[texture]
                image = Image.open(io.BytesIO(archive.read("assets/minecraft/textures/" + name + ".png"))).convert("RGBA").crop(crop)
                image.thumbnail((14, 14), Image.Resampling.NEAREST)
                tile_image = Image.new("RGBA", (16,16));tile_image.paste(image, ((16-image.width)//2, (16-image.height)//2))
                image = tile_image
            else:
                image = Image.open(io.BytesIO(archive.read("assets/minecraft/textures/items/" + texture + ".png"))).convert("RGBA")
            if texture == "spawn_egg":
                tint = {"PIG":(226,153,153),"COW":(112,80,60),"SHEEP":(220,220,220),"ZOMBIE":(78,132,68),"CREEPER":(65,155,60),"CHICKEN":(235,235,235),"SKELETON":(220,220,220),"SPIDER":(55,30,30),"ENDERMAN":(35,20,45),"SLIME":(80,220,100),"GHAST":(235,235,235),"BLAZE":(245,160,50),"WITCH":(90,55,110),"WITHER":(45,45,45),"BAT":(70,45,75),"RABBIT":(170,130,90),"WOLF":(150,150,150),"OCELOT":(210,180,110),"HORSE":(140,90,50)}
                color = tint.get(fields[0].split("_")[0], (160,180,150))
                image = ImageChops.multiply(image, Image.new("RGBA",image.size,(*color,255)))
            items.paste(image.crop((0, 0, 16, 16)), ((i % 32)*16, (i // 32)*16))
        for start, prefix in [(768,"clock"),(800,"compass")]:
            for frame in range(32):
                image=Image.open(io.BytesIO(archive.read(f"assets/minecraft/textures/items/{prefix}_{frame:02d}.png"))).convert("RGBA")
                n=start+frame;items.paste(image.crop((0,0,16,16)),((n%32)*16,(n//32)*16))
        image=Image.open(io.BytesIO(archive.read("assets/minecraft/textures/items/fishing_rod_cast.png"))).convert("RGBA")
        items.paste(image.crop((0,0,16,16)),(0,416))
        for frame in range(3):
            image=Image.open(io.BytesIO(archive.read(f"assets/minecraft/textures/items/bow_pulling_{frame}.png"))).convert("RGBA")
            items.paste(image.crop((0,0,16,16)),((frame+1)*16,416))
        items.save(args.output / "items.png")
    args.output.mkdir(parents=True, exist_ok=True)
    (args.output / "player.png").write_bytes(skin)
    atlas.save(args.output / "texture_atlas.png")
    atlas.resize((256, 256), Image.Resampling.BOX).save(args.output / "texture_atlas_mipmap.png")
    print(f"Imported 1.12.2 block textures into {args.output}")


if __name__ == "__main__":
    main()
