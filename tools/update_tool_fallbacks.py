"""Draw this project's own fallback icons for the extended tool catalog."""
import csv
import math
import re
from pathlib import Path
from PIL import Image, ImageDraw

root = Path(__file__).resolve().parents[1]
folder = root / "client/textures"
items = Image.open(folder / "items.png").convert("RGBA")
entries = [next(csv.reader([re.fullmatch(r"ITEM\((.*)\)", line)[1]]))
           for line in (root / "client/include/items.def").read_text().splitlines()]
colors = {"WOOD": "#aa7845", "STONE": "#9b9fa5", "IRON": "#dbe5e5",
          "DIAMOND": "#4ce2ce", "GOLD": "#ffda50"}
for index, fields in enumerate(entries):
    if index < 21:
        continue
    name = fields[0]
    tile = Image.new("RGBA", (16, 16))
    d = ImageDraw.Draw(tile)
    family = name.split("_", 1)[0]
    if family in colors:
        color = colors[family]
        d.line([(3, 13), (11, 5)], fill="#37271d", width=4)
        d.line([(3, 13), (11, 5)], fill="#ad7c42", width=2)
        if name.endswith("PICKAXE"):
            d.line([(4, 3), (11, 3), (13, 6)], fill=color, width=3)
        elif name.endswith("AXE"):
            d.polygon([(7, 1), (12, 2), (14, 5), (11, 8), (7, 6)], fill=color)
        elif name.endswith("SHOVEL"):
            d.polygon([(10, 1), (14, 2), (15, 5), (12, 8), (8, 5)], fill=color)
        elif name.endswith("HOE"):
            d.line([(5, 2), (11, 2), (13, 5)], fill=color, width=3)
        else:
            d.line([(6, 10), (13, 3)], fill=color, width=4)
            d.line([(4, 8), (8, 12)], fill="#514337", width=2)
    elif name == "SHEARS":
        d.line([(3, 3), (12, 12)], fill="#dce4e6", width=3)
        d.line([(12, 3), (3, 12)], fill="#aebbc1", width=3)
        d.ellipse((1, 10, 6, 15), outline="#704e39", width=2)
        d.ellipse((10, 10, 15, 15), outline="#704e39", width=2)
    elif name == "FLINT_AND_STEEL":
        d.arc((1, 3, 13, 13), 180, 480, fill="#dce4e6", width=3)
        d.polygon([(7, 8), (11, 6), (14, 12), (10, 15)], fill="#5f6573")
    elif name == "FISHING_ROD":
        d.line([(2, 14), (11, 2)], fill="#b88b51", width=3)
        d.line([(11, 2), (14, 5), (14, 12), (12, 14)], fill="#d8d8d0", width=1)
    elif name == "STRING":
        d.arc((2, 1, 13, 13), 0, 300, fill="#ecece0", width=2)
        d.line([(5, 8), (3, 14), (10, 12)], fill="#ecece0", width=2)
    elif name == "RAW_FISH":
        d.ellipse((2, 5, 12, 11), fill="#aaa474")
        d.polygon([(11, 8), (15, 4), (15, 12)], fill="#7d9369")
        d.point((4, 7), fill="black")
    else:
        d.ellipse((1, 1, 14, 14), fill="#efce64" if name == "CLOCK" else "#a3afb8", outline="#383a40", width=2)
        d.line([(8, 8), (8, 3)], fill="#cc4040", width=2)
    items.paste(tile, ((index % 16) * 16, (index // 16) * 16))
for start, compass in [(64, False), (96, True)]:
    for frame in range(32):
        tile = Image.new("RGBA", (16, 16))
        d = ImageDraw.Draw(tile)
        d.ellipse((1, 1, 14, 14), fill="#8e9ca8" if compass else "#d7b452", outline="#393944", width=2)
        angle = frame / 32 * math.tau
        d.line([(8, 8), (8 + math.sin(angle)*5, 8 - math.cos(angle)*5)], fill="#e24343" if compass else "#273951", width=2)
        n = start + frame
        items.paste(tile, ((n % 16)*16, (n // 16)*16))
rod_index = next(i for i, fields in enumerate(entries) if fields[0] == "FISHING_ROD")
items.paste(items.crop(((rod_index%16)*16, (rod_index//16)*16, (rod_index%16)*16+16, (rod_index//16)*16+16)), (0,128))
items.save(folder / "items.png")

atlas = Image.open(folder / "texture_atlas.png").convert("RGBA")
d = ImageDraw.Draw(atlas)
for x, base in [(11,"#855e36"),(12,"#ab9762"),(13,"#9b8753")]:
    d.rectangle((x*16,64,x*16+15,79), fill=base)
    for y in range(66,80,4):
        d.line((x*16,y,x*16+15,y), fill="#5d462d" if x==11 else "#bba778")
for frame in range(32):
    x = frame*16
    d.rectangle((x,112,x+15,127), fill=(0,0,0,0))
    for a in range(2,15,4):
        y = 114+(frame+a)%6
        d.polygon([(x+a-2,127),(x+a, y),(x+a+2,y+4),(x+a+3,127)], fill="#ec751c")
        d.line((x+a,120,x+a,127), fill="#ffdf59",width=2)
atlas.save(folder / "texture_atlas.png")
atlas.resize((256,256),Image.Resampling.BOX).save(folder / "texture_atlas_mipmap.png")
