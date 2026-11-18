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
    if family in colors and any(name.endswith(suffix) for suffix in ("PICKAXE", "AXE", "SHOVEL", "HOE", "SWORD")):
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
    elif index >= 49:
        # Project-authored fallback sprites. Imported vanilla art is never committed.
        foods = {"CARROT":"#f28c24", "POTATO":"#c3a16b", "BAKED_POTATO":"#d2b577",
                 "BEETROOT":"#b83a58", "MELON_SLICE":"#e35d5c", "COOKIE":"#ba8044",
                 "PUMPKIN_PIE":"#d5903a", "RAW_BEEF":"#b34b4b", "STEAK":"#865138",
                 "RAW_PORKCHOP":"#e89e9c", "COOKED_PORKCHOP":"#b47c54", "COOKED_FISH":"#bfb08c",
                 "COOKED_CHICKEN":"#d7aa70", "RABBIT":"#dbaf9f", "COOKED_RABBIT":"#ac7851", "COOKED_MUTTON":"#8a5841"}
        if name in foods:
            d.ellipse((2,4,13,12),fill=foods[name],outline="#503d30")
            d.line((4,6,10,6),fill="#f0ce97")
            if name=="CARROT":d.polygon([(4,5),(12,6),(3,14)],fill=foods[name]);d.line((10,2,10,5),fill="#5b9641",width=2)
        elif name in ("PAPER","BOOK","LEATHER"):
            d.polygon([(2,2),(11,1),(14,12),(4,14)],fill="#ede6cb" if name=="PAPER" else "#98653f",outline="#40372f")
            d.line((5,4,11,3),fill="#c6b898")
        elif name in ("EGG","GHAST_TEAR"):
            d.ellipse((4,2,11,14),fill="#f4eee0",outline="#aaa69a")
        elif name in ("SUGAR_CANE","BLAZE_ROD","BONE","FEATHER"):
            color={"SUGAR_CANE":"#8bbb4f","BLAZE_ROD":"#f8c544","BONE":"#e7e2cd","FEATHER":"#dbded9"}[name]
            d.line((3,13,12,3),fill="#4c493a",width=5);d.line((3,13,12,3),fill=color,width=3)
        else:
            color={"CHARCOAL":"#39383c","QUARTZ":"#e4dfd2","CLAY_BALL":"#9aa5b0","SUGAR":"#eaeaea",
                   "BONE_MEAL":"#ece8d5","SLIME_BALL":"#85c26d","ENDER_PEARL":"#287b73","BLAZE_POWDER":"#f2a72b",
                   "NETHER_WART":"#b94244","MAGMA_CREAM":"#e7a43f","ENDER_EYE":"#72b98a","GOLD_NUGGET":"#eac84d",
                   "IRON_NUGGET":"#c9cecf","PRISMARINE_SHARD":"#71a89b","PRISMARINE_CRYSTALS":"#b7dcba"}.get(name,"#afb8bb")
            d.polygon([(3,5),(9,2),(14,7),(11,13),(4,14),(1,9)],fill=color,outline="#3c4544")
            d.line((4,6,8,4),fill="#dddcb7",width=2)
    else:
        d.ellipse((1, 1, 14, 14), fill="#efce64" if name == "CLOCK" else "#a3afb8", outline="#383a40", width=2)
        d.line([(8, 8), (8, 3)], fill="#cc4040", width=2)
    items.paste(tile, ((index % 16) * 16, (index // 16) * 16))
for start, compass in [(160, False), (192, True)]:
    for frame in range(32):
        tile = Image.new("RGBA", (16, 16))
        d = ImageDraw.Draw(tile)
        d.ellipse((1, 1, 14, 14), fill="#8e9ca8" if compass else "#d7b452", outline="#393944", width=2)
        angle = frame / 32 * math.tau
        d.line([(8, 8), (8 + math.sin(angle)*5, 8 - math.cos(angle)*5)], fill="#e24343" if compass else "#273951", width=2)
        n = start + frame
        items.paste(tile, ((n % 16)*16, (n // 16)*16))
rod_index = next(i for i, fields in enumerate(entries) if fields[0] == "FISHING_ROD")
items.paste(items.crop(((rod_index%16)*16, (rod_index//16)*16, (rod_index%16)*16+16, (rod_index//16)*16+16)), (0,224))
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

# Transparent fracture stages for builds without imported resources.
atlas=Image.open(folder / "texture_atlas.png").convert("RGBA")
for stage in range(10):
    tile=Image.new("RGBA",(16,16));d=ImageDraw.Draw(tile)
    for branch in range(stage+1):
        a=branch*2.4;px=8;py=8
        for step in range(1,6):
            nx=round(8+math.cos(a)*step*1.5);ny=round(8+math.sin(a)*step*1.5)
            d.line((px,py,nx,ny),fill=(40,40,40,180));px,py=nx,ny;a+=.13*(-1 if step%2 else 1)
    atlas.paste(tile,(stage*16,160))
atlas.save(folder / "texture_atlas.png")
atlas.resize((256,256),Image.Resampling.BOX).save(folder / "texture_atlas_mipmap.png")

# Expanded cube catalog uses unused rows 8+ of the block atlas.
import random
import colorsys
atlas = Image.open(folder / "texture_atlas.png").convert("RGBA")
seen = set()
for line in (root / "client/include/extra_blocks.def").read_text().splitlines():
    match = re.match(r"(?:EXTRA|BASE)_BLOCK\((.*)\)",line)
    if not match: continue
    fields = next(csv.reader([match[1]],skipinitialspace=True))
    for offset in (2,5,8):
        x,y,texture=int(fields[offset]),int(fields[offset+1]),fields[offset+2]
        if y<8 or (x,y) in seen:continue
        seen.add((x,y));rng=random.Random(texture)
        colors={"white":(218,220,218),"orange":(216,128,43),"magenta":(172,71,166),"light_blue":(83,161,202),
                "yellow":(228,194,53),"lime":(111,174,44),"pink":(219,140,167),"gray":(67,74,76),"silver":(150,154,150),
                "cyan":(42,137,147),"purple":(119,66,161),"blue":(57,71,157),"brown":(114,78,48),"green":(80,109,42),
                "red":(166,53,46),"black":(34,38,43)}
        base=(128,130,128)
        if any(t in texture for t in ["wool_colored_","concrete_","hardened_clay_stained_"]):
            base=colors[texture.split("colored_")[-1] if "colored_" in texture else texture.split("stained_")[-1] if "stained_" in texture else texture[9:]]
        elif "granite" in texture:base=(151,107,90)
        elif "diorite" in texture:base=(196,196,187)
        elif "planks" in texture or "log" in texture:base=(135,96,58) if "jungle" in texture else (173,96,54) if "acacia" in texture else (75,51,34)
        elif "red_sand" in texture:base=(181,102,48)
        elif "hardened_clay" in texture:base=(153,95,69)
        elif "hay" in texture:base=(190,158,50)
        elif "melon" in texture:base=(100,149,49)
        elif "pumpkin" in texture:base=(201,129,41)
        elif "purpur" in texture:base=(162,119,162)
        elif "prismarine" in texture:base=(64,132,115)
        elif "sea_lantern" in texture:base=(184,216,197)
        elif "coal" in texture:base=(40,41,41)
        tile=Image.new("RGBA",(16,16));d=ImageDraw.Draw(tile)
        for py in range(16):
            for px in range(16):
                noise=rng.randint(-10,10)
                if "planks" in texture and py%4==0:noise-=25
                if "log" in texture and px%4==0:noise-=25
                if "melon_side" in texture and px%3==0:noise-=30
                d.point((px,py),fill=tuple(max(0,min(255,c+noise)) for c in base)+(255,))
        atlas.paste(tile,(x*16,y*16))
atlas.save(folder / "texture_atlas.png")
atlas.resize((256,256),Image.Resampling.BOX).save(folder / "texture_atlas_mipmap.png")
