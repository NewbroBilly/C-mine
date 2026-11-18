"""Prepare the supplied dark-background logo without changing the source file."""
import argparse
from pathlib import Path
from PIL import Image
parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('image',type=Path)
parser.add_argument('--output',type=Path,required=True)
args=parser.parse_args()
image=Image.open(args.image).convert('RGBA')
pixels=image.load()
for y in range(image.height):
 for x in range(image.width):
  r,g,b,_=pixels[x,y];v=max(r,g,b)
  pixels[x,y]=(r,g,b,round(max(0,min(1,(v-12)/16))*255))
image=image.crop(image.getbbox())
image.thumbnail((1280,512),Image.Resampling.LANCZOS)
args.output.parent.mkdir(parents=True,exist_ok=True)
image.save(args.output)
print(args.output,image.size)
