"""Tiny original diagnostic mesh; contains no game assets and no gameplay map."""
import math,struct
def motion_block():
    tracks=[];keys=[]
    for values in (((.2,.4,1,0),(1,.35,.1,0),(.2,.4,1,0)),((.3,0,0,0),(1,0,0,0),(.3,0,0,0)),None,((0,0,0,0),(.5,0,0,0),(0,0,0,0))):
        if values is None:tracks.append((0,0,0,0));continue
        tracks.append((len(keys),3,4000,3));keys.extend((stamp,*value) for stamp,value in zip((0,2000,4000),values))
    return struct.pack('<II5f',0x31544d57,len(keys),1,1,1,1,.75)+b''.join(struct.pack('<4I',*t) for t in tracks)+b''.join(struct.pack('<I4f',*k) for k in keys)+bytes((128-len(keys))*20)
def pack(motion=False,lighting=False):
    parts=[]
    def quad(vertices,flags,kind=2,pixels=(0,0x805080c0,0,0x805080c0)):
        parts.append((vertices,flags,kind,pixels))
    quad([(-2,-8,0),(14,-8,0),(14,8,0),(-2,8,0)],0,4,(0xffffffff,)*4)
    # Each material overlays a common opaque board; black additive texels must
    # leave that board untouched. Alpha-key and opaque are deliberately distinct.
    for row,z in enumerate((3.0,.4)):
        for mode in range(7):
            y=5.4-mode*1.8
            flags=2 if mode==1 else mode<<7
            if row:flags|=(1<<10)|(1<<11)
            if row and mode==0:flags|=1<<12
            if row and mode==1:flags|=1<<13
            quad([(10,y+.72,z-.62),(10,y-.72,z-.62),(10,y-.72,z+.62),(10,y+.72,z+.62)],flags)
    quad([(11,7,-.7),(11,-7,-.7),(11,-7,4.2),(11,7,4.2)],(1<<10)|(1<<11),pixels=(0xff404040,)*4)
    entries=[];payload=bytearray();offset=32+64*len(parts)
    def append(data):
        at=offset+len(payload);payload.extend(data);return at
    for i,(v,flags,kind,pixels) in enumerate(parts):
        if lighting and 1<=i<=14:
            append(struct.pack('<4I',0xff0000ff,0xff00ff00,0xffff0000,0xffffffff));flags|=131072
            if i>=8:flags|=262144
            if i in (1,2,8,9):pixels=(0xffffffff,)*4
        if motion and 1<=i<=14:append(motion_block());flags|=16384|32768|65536
        vo=append(b''.join(struct.pack('<8f',*p,-1,0,0,*uv) for p,uv in zip(v,[(0,1),(1,1),(1,0),(0,0)])))
        io=append(struct.pack('<6H',0,1,2,0,2,3));to=append(struct.pack('<4I',*pixels))
        center=[(max(p[k] for p in v)+min(p[k] for p in v))/2 for k in range(3)]
        radius=max(math.dist(center,p) for p in v)
        entries.append(struct.pack('<9I4f3I',kind,i,4,6,vo,io,to,2,2,*center,radius,flags|1,0,1))
    return struct.pack('<4sIII3fI',b'WXP1',8 if lighting else 7 if motion else 6,len(parts),64,0,0,0,offset+len(payload))+b''.join(entries)+payload
