"""Remove the supplied logo's exterior background, preserving its dark details."""
import argparse
from pathlib import Path
from PIL import Image, ImageChops, ImageDraw
parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('image',type=Path)
parser.add_argument('--output',type=Path,required=True)
args=parser.parse_args()
image=Image.open(args.image).convert('RGB')
r,g,b=image.split()
value=ImageChops.lighter(ImageChops.lighter(r,g),b)
# The near-black outline encloses black shading and letter details. Only key
# the background reachable from the outside; a global color key erases those
# details and makes the panorama show through the logo. The 18 threshold
# removes JPEG background noise while keeping the supplied blue-black outline.
mask=value.point(lambda v:255 if v>18 else 0)
ImageDraw.floodfill(mask,(0,0),128)
alpha=mask.point(lambda v:0 if v==128 else 255)
image=image.convert('RGBA')
image.putalpha(alpha)
image=image.crop(alpha.getbbox())
image.thumbnail((1280,512),Image.Resampling.LANCZOS)
args.output.parent.mkdir(parents=True,exist_ok=True)
image.save(args.output)
print(args.output,image.size)
