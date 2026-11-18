"""Generate project-authored fallback art. Vanilla art is imported separately."""
from pathlib import Path
import random
from PIL import Image,ImageDraw
root=Path(__file__).resolve().parents[1]/'client/textures'
widgets=Image.new('RGBA',(256,256))
for y,base in [(46,48),(66,112),(86,137)]:
 d=ImageDraw.Draw(widgets);rng=random.Random(y)
 d.rectangle((0,y,199,y+19),fill=(0,0,0))
 for py in range(y+1,y+19):
  for x in range(1,199):
   n=base+rng.randrange(-9,10);d.point((x,py),fill=(n,n,n if y!=86 else min(255,n+26),255))
 d.line((1,y+1,198,y+1),fill=(205,205,205));d.line((1,y+1,1,y+18),fill=(205,205,205))
 d.line((1,y+18,198,y+18),fill=(55,55,55));d.line((198,y+1,198,y+18),fill=(55,55,55))
widgets.save(root/'widgets.png')
colors={'pig':(222,153,153),'cow':(91,70,60),'sheep':(184,169,148),'sheep_fur':(234,233,224),'zombie':(80,121,65),'creeper':(60,142,60)}
for name,color in colors.items():
 size=(64,64 if name=='zombie' else 32);image=Image.new('RGBA',size);d=ImageDraw.Draw(image);rng=random.Random(name)
 for y in range(size[1]):
  for x in range(64):
   n=rng.randrange(-16,17);c=tuple(max(0,min(255,a+n)) for a in color)
   if name=='cow' and rng.random()<.27:c=(205,198,177)
   if name=='zombie' and y>=16:c=(49,119,137) if x>15 else (65,69,104)
   d.point((x,y),fill=(*c,255))
 d.rectangle((9,11,10,12),fill=(25,24,22));d.rectangle((13,11,14,12),fill=(25,24,22))
 image.save(root/(name+'.png'))
for name,color in [('leather',(160,101,64)),('chainmail',(127,136,141)),('iron',(210,215,220)),('gold',(235,192,58)),('diamond',(69,204,190))]:
 for layer in (1,2):
  image=Image.new('RGBA',(64,32),(*color,255));d=ImageDraw.Draw(image)
  for y in range(0,32,4):d.line((0,y,63,y),fill=tuple(int(c*.8) for c in color)+(255,))
  # Opening at the front of the helmet; model boxes mask all unused pixels.
  d.rectangle((9,10,14,15),fill=(0,0,0,0))
  if name=='chainmail':
   for y in range(32):
    for x in range(64):
     if (x+y)%3==0:d.point((x,y),fill=(0,0,0,0))
  image.save(root/f'{name}_layer_{layer}.png')
print('Generated classic fallback widgets, entity skins and armor layers')
