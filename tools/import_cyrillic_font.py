"""Merge the free DejaVu bitmap Cyrillic glyphs from mc into C-mine's atlas.

Usage: python tools/import_cyrillic_font.py path/to/mc/assets/font/font.png
The existing ASCII cells remain unchanged. Requires Pillow.
"""
from pathlib import Path
from PIL import Image
import argparse

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('source',type=Path)
    parser.add_argument('--output',type=Path,default=Path(__file__).resolve().parents[1]/'client/textures/font.png')
    args=parser.parse_args()
    source=Image.open(args.source).convert('RGBA')
    if source.size!=(128,192):raise ValueError('Expected mc DejaVu 128x192 font atlas')
    previous=Image.open(args.output).convert('RGBA')
    if previous.size not in ((128,128),(128,192)):raise ValueError('Unexpected C-mine atlas dimensions')
    atlas=Image.new('RGBA',(128,192))
    for n in range(128):
        x=n%16*8;y=n//16*(8 if previous.height==128 else 12)
        if previous.height==192:y+=2
        atlas.paste(previous.crop((x,y,x+8,y+8)),(x,n//16*12+2))
    for i in range(66):
        n=95+i
        cell=source.crop(((n%16)*8,(n//16)*12,(n%16+1)*8,(n//16+1)*12))
        n=128+i;atlas.paste(cell,((n%16)*8,(n//16)*12))
    atlas.save(args.output)

if __name__=='__main__':main()
