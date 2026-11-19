"""Expand the project's own item art without bundling Mojang textures.

Run after update_tool_fallbacks.py. Original static and animated icons are
retained, new category-specific silhouettes fill the appended registry slots.
Works with both the old 256px atlas and the current 512px atlas.
"""
import csv
import hashlib
import re
from pathlib import Path
from PIL import Image, ImageDraw

root = Path(__file__).resolve().parents[1]
path = root / "client/textures/items.png"
old = Image.open(path).convert("RGBA")
image = Image.new("RGBA", (512,512))
entries = [next(csv.reader([re.fullmatch(r"ITEM\((.*)\)", line)[1]]))
           for line in (root/"client/include/items.def").read_text().splitlines()]
for n, (name, label, texture, *_rest) in enumerate(entries):
    if n < 113:
        columns = old.width//16
        tile = old.crop(((n%columns)*16,(n//columns)*16,(n%columns)*16+16,(n//columns)*16+16))
    else:
        tile = Image.new("RGBA",(16,16))
        d = ImageDraw.Draw(tile)
        rgb = tuple(70+b%150 for b in hashlib.sha256(name.encode()).digest()[:3])
        if "SPAWN_EGG" in name:
            d.ellipse((4,2,12,14), fill=rgb, outline="#333333")
            d.rectangle((6,4,8,6),fill="#dbdab1");d.rectangle((9,9,11,11),fill="#dbdab1")
        elif "ARROW" in name:
            d.line((2,14,13,3),fill="#a57a45",width=2);d.polygon([(10,1),(15,1),(15,6)],fill="#d5d9df")
        elif "BOW" in name:
            d.arc((1,1,13,15),270,90,fill="#a6753b",width=3);d.line((7,1,7,15),fill="#ddd3c4")
        elif name=="SHIELD":
            d.polygon([(3,2),(13,2),(13,10),(8,15),(3,10)],fill="#86623f",outline="#d3d8da")
        elif "BUCKET" in name or name=="BOWL":
            d.polygon([(2,5),(14,5),(12,14),(4,14)],fill="#adb8c3",outline="#424c58")
            d.ellipse((3,3,13,7),fill="#eeeecc" if "MILK" in name else "#ed712c" if "LAVA" in name else "#604222")
        elif "POTION" in name or "BOTTLE" in name or name=="DRAGON_BREATH":
            d.rectangle((6,1,9,6),fill="#b4d5df");d.ellipse((3,5,12,14),fill=rgb,outline="#bee6ef")
        elif "BOOK" in name or "MAP" in name:
            d.polygon([(2,3),(13,1),(14,12),(3,15)],fill="#dabf8c",outline="#68452c")
            for y in (5,8,11):d.line((5,y,11,y-1),fill="#735535")
        elif "DOOR" in name or name in ("SIGN","PAINTING","ITEM_FRAME","BANNER"):
            d.rectangle((4,1,12,14),fill="#a57945",outline="#573e27");d.rectangle((6,3,10,6),fill=rgb);d.point((10,10),fill="#e7d78b")
        elif "BOAT" in name or "MINECART" in name:
            d.polygon([(1,7),(14,7),(11,13),(4,13)],fill="#957047",outline="#dddddd")
            d.line((2,4,13,11),fill="#60442b",width=2)
        elif name.startswith("RECORD"):
            d.ellipse((1,1,14,14),fill="#313337",outline="#62666b");d.ellipse((6,6,9,9),fill=rgb)
        elif name=="TOTEM":
            d.ellipse((5,1,11,7),fill="#bd9f56");d.polygon([(7,6),(2,8),(3,10),(6,9),(6,14),(10,14),(10,9),(13,10),(14,8),(9,6)],fill="#b69a50")
            d.point((7,4),fill="#2f865d");d.point((9,4),fill="#2f865d")
        elif "DYE" in name or name=="INK_SAC":
            d.polygon([(7,2),(13,8),(11,13),(4,14),(2,9)],fill=rgb,outline="#363636")
        else:
            d.polygon([(7,2),(13,5),(14,10),(9,14),(3,12),(2,6)],fill=rgb,outline="#373335")
            d.line((5,5,9,3,11,5),fill="#eee1b8",width=2)
    image.paste(tile,((n%32)*16,(n//32)*16))
for old_start,new_start,count in [(160,768,32),(192,800,32),(224,832,1)]:
    for k in range(count):
        n=(old_start if old.width==256 else new_start)+k;c=old.width//16
        tile=old.crop(((n%c)*16,(n//c)*16,(n%c)*16+16,(n//c)*16+16))
        m=new_start+k;image.paste(tile,((m%32)*16,(m//32)*16))
bow=next(i for i,e in enumerate(entries) if e[0]=="BOW")
for k in range(3):
    tile=image.crop(((bow%32)*16,(bow//32)*16,(bow%32)*16+16,(bow//32)*16+16))
    d=ImageDraw.Draw(tile);d.line((7,1,3-k,8,7,15),fill="#eee1b8")
    image.paste(tile,((k+1)*16,416))
image.save(path)
print(f"Expanded fallback atlas: {len(entries)} static icons, 65 animated icons, 3 bow frames")
