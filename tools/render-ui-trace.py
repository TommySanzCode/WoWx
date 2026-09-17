"""Inspect native UI command layout on the host; this is not an xemu capture."""
import argparse,math,struct
from pathlib import Path
from PIL import Image,ImageDraw
p=argparse.ArgumentParser(description=__doc__);p.add_argument('atlas',type=Path);p.add_argument('trace',type=Path);p.add_argument('--output',type=Path,required=True);a=p.parse_args()
data=a.atlas.read_bytes();header=struct.unpack_from('<4s9I',data)
assert header[0]==b'WXU1' and header[1] in (1,2) and header[2:4]==(512,512) and header[9]==len(data)
pixels=data[header[8]:];assert len(pixels)==512*512*4
def morton(x,y):return sum(((x>>b)&1)<<(2*b)|((y>>b)&1)<<(2*b+1) for b in range(9))
rgba=bytearray(len(pixels))
for y in range(512):
    for x in range(512):
        offset=4*morton(x,y);b,g,r,alpha=pixels[offset:offset+4];start=(y*512+x)*4;rgba[start:start+4]=bytes((r,g,b,alpha))
atlas=Image.frombytes('RGBA',(512,512),bytes(rgba));trace=a.trace.read_bytes();magic,count=struct.unpack_from('<4sI',trace)
assert magic==b'WQV1' and count<=2048 and len(trace)==8+160*count
canvas=Image.new('RGBA',(640,480),(36,48,56,255))
for q in range(count):
    v=[struct.unpack_from('<10f',trace,8+160*q+40*i) for i in range(4)]
    assert all(math.isfinite(n) for vertex in v for n in vertex)
    x0,y0=v[0][:2];x1,y1=v[2][:2];u0,t0=v[0][8:];u1,t1=v[2][8:]
    assert 0<=x0<x1<=640 and 0<=y0<y1<=480
    left,top,right,bottom=math.floor(x0),math.floor(y0),math.ceil(x1),math.ceil(y1)
    sx=(u1-u0)*512/(x1-x0);sy=(t1-t0)*512/(y1-y0)
    image=atlas.transform((right-left,bottom-top),Image.Transform.AFFINE,
                          (sx,0,u0*512+(left-x0)*sx,0,sy,t0*512+(top-y0)*sy),resample=Image.Resampling.BILINEAR)
    tinted=[]
    for c,channel in enumerate(image.split()):
        factor=max(0,min(1,v[0][4+c]));tinted.append(channel.point([round(i*factor) for i in range(256)]))
    image=Image.merge('RGBA',tuple(tinted))
    canvas.alpha_composite(image,(left,top))
ImageDraw.Draw(canvas).text((20,448),'HOST SOFTWARE PREVIEW - synthetic units, not xemu',fill=(255,255,255,255))
a.output.parent.mkdir(parents=True,exist_ok=True);canvas.convert('RGB').save(a.output);print(a.output)
