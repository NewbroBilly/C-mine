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
    atlas.paste(tile("torch_on"), (48, 16))
    torch_top = tile("torch_on").crop((7, 6, 9, 8)).resize((16, 16), Image.Resampling.NEAREST)
    atlas.paste(torch_top, (32, 16))
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
                      "crafting.png": "container/crafting_table.png", "icons.png": "icons.png"}
        args.output.mkdir(parents=True, exist_ok=True)
        for target, source in gui_assets.items():
            (args.output / target).write_bytes(archive.read("assets/minecraft/textures/gui/" + source))
        source = Image.open(io.BytesIO(archive.read("assets/minecraft/textures/gui/container/creative_inventory/tab_items.png"))).convert("RGBA")
        creative = Image.new("RGBA", (256, 256))
        creative.paste(source.crop((0, 0, 176, 112)), (0, 0))
        creative.paste(source.crop((0, 17, 176, 35)), (0, 107))
        # Reuse the original hotbar strip under six catalog rows.
        creative.paste(source.crop((0, 112, 176, 136)), (0, 126))
        creative.paste(creative.crop((0, 0, 7, 150)).transpose(Image.Transpose.FLIP_LEFT_RIGHT), (169, 0))
        creative.save(args.output / "creative.png")
        items = Image.new("RGBA", (256, 256))
        catalog = Path(__file__).resolve().parents[1] / "client/include/items.def"
        for i, line in enumerate(catalog.read_text().splitlines()):
            fields = next(csv.reader([re.fullmatch(r"ITEM\((.*)\)", line)[1]]))
            image = Image.open(io.BytesIO(archive.read("assets/minecraft/textures/items/" + fields[2] + ".png"))).convert("RGBA")
            items.paste(image.crop((0, 0, 16, 16)), ((i % 16)*16, (i // 16)*16))
        items.save(args.output / "items.png")
    args.output.mkdir(parents=True, exist_ok=True)
    (args.output / "player.png").write_bytes(skin)
    atlas.save(args.output / "texture_atlas.png")
    atlas.resize((256, 256), Image.Resampling.BOX).save(args.output / "texture_atlas_mipmap.png")
    print(f"Imported 1.12.2 block textures into {args.output}")


if __name__ == "__main__":
    main()
