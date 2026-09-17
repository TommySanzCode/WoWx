"""Independent WXP9 metadata inspection and reproducible offline camera probes.

Does not claim playable water, portal visibility or physical traversal. Camera
probes are synthesized from the original prepared bounds and liquid grids.
"""
import argparse,hashlib,json,math,struct
from pathlib import Path

def records(data):
    magic,version,count,stride,*rest=struct.unpack_from('<4sIII3fI',data)
    if magic!=b'WXP1' or version not in (9,10) or stride!=64 or rest[-1]!=len(data):raise ValueError('Invalid environment pack header')
    tag,n,size,offset=struct.unpack_from('<4sIII',data,len(data)-16)
    if tag!=b'WXE1' or n>2048 or not n*128<=size<=2*1024*1024 or offset+size+16!=len(data) or offset<32+count*64:raise ValueError('Invalid environment footer')
    blob=data[offset:-16];out=[];next_offset=n*128
    for i in range(n):
        r=struct.unpack_from('<4I18f4I4f2I',blob,i*128)
        kind,group,flags,liquid=r[:4];inv=r[4:16];lo=r[16:19];hi=r[19:22];nx,ny,ho,fo=r[22:26];corner=r[26:29];tile=r[29]
        if kind not in (1,2) or any(not math.isfinite(x) for x in inv+lo+hi+corner) or any(x>y for x,y in zip(lo,hi)):raise ValueError('Invalid volume')
        entry=dict(kind=kind,group=group,flags=flags,liquid=liquid,inverse=inv,lo=lo,hi=hi,nx=nx,ny=ny,corner=corner,tile=tile)
        if kind==2:
            if not 0<nx<=256 or not 0<ny<=256 or ho!=next_offset or fo!=ho+(nx+1)*(ny+1)*4:raise ValueError('Invalid liquid layout')
            entry['heights']=struct.unpack_from('<'+str((nx+1)*(ny+1))+'f',blob,ho);entry['mask']=blob[fo:fo+nx*ny]
            next_offset=fo+((nx*ny+3)&~3)
        out.append(entry)
    if next_offset!=len(blob):raise ValueError('Unclaimed environment bytes')
    return out,dict(records=n,bytes=size,offset=offset)

def local(r,p):return [sum(r['inverse'][k*4+j]*p[j] for j in range(3))+r['inverse'][k*4+3] for k in range(3)]
def world(r,p):
    inv=r['inverse'];v=[p[k]-inv[k*4+3] for k in range(3)]
    q=[sum(inv[k*4+j]*v[k] for k in range(3)) for j in range(3)]
    return list(struct.unpack('<3f',struct.pack('<3f',*q)))
def height(r,x,y):
    ix,iy=int(x),int(y);x-=ix;y-=iy;w=r['nx']+1;h=r['heights'];i=iy*w+ix
    return h[i]+x*(h[i+1]-h[i])+y*(h[i+w+1]-h[i+1]) if x>=y else h[i]+x*(h[i+w+1]-h[i+w])+y*(h[i+w]-h[i])
def sample(rows,p):
    indoor=[];liquids=[]
    for r in rows:
        q=local(r,p)
        if any(q[k]<r['lo'][k] or q[k]>r['hi'][k] for k in range(3)):continue
        if r['kind']==1 and r['flags']&0x2000:indoor.append((math.prod(b-a for a,b in zip(r['lo'],r['hi'])),r['group']))
        if r['kind']==2:
            x=(q[0]-r['corner'][0])/r['tile'];y=(q[1]-r['corner'][1])/r['tile']
            if not (0<=x<r['nx'] and 0<=y<r['ny']) or r['mask'][int(y)*r['nx']+int(x)]&15==15:continue
            depth=height(r,x,y)-q[2]
            if depth>0:liquids.append((depth,r['liquid'],r['group']))
    env=1 if indoor else 0;liquid=0;depth=0;group=min(indoor)[1] if indoor else 0
    if liquids:
        depth,liquid,g=min(liquids)
        if liquid in (1,2):env=2;group=g
    return dict(environment=env,liquid_type=liquid,depth=depth,group=group)

def probes(folder,maps):
    result=[];dependencies={}
    for map_id in maps:
        path=folder/f'G{map_id:03}.WXP';data=path.read_bytes();rows,meta=records(data)
        dependencies[str(path.resolve())]=hashlib.sha256(data).hexdigest()
        def add(label,p):
            s=sample(rows,p)
            result.append(dict(map=map_id,point=p,label=label,**s))
        # One interior and one nearby point proved outside every prepared group.
        inside=next((r for r in rows if r['kind']==1 and r['flags']&0x2000 and all(b-a>2 for a,b in zip(r['lo'],r['hi']))),None)
        if inside:
            center=[(a+b)/2 for a,b in zip(inside['lo'],inside['hi'])];add('group-center',world(inside,center))
            for axis in range(3):
                found=False
                for sign in (0,1):
                    p=center.copy();p[axis]=inside['hi' if sign else 'lo'][axis]+(1 if sign else -1)
                    p=world(inside,p)
                    if sample(rows,p)['environment']==0:add('outside-group',p);found=True;break
                if found:break
        water=next((r for r in rows if r['kind']==2 and r['liquid'] in (1,2)),None)
        if water:
            for i,mask in enumerate(water['mask']):
                if mask&15==15:continue
                x=i%water['nx']+.5;y=i//water['nx']+.5;z=height(water,x,y)
                p=[water['corner'][0]+x*water['tile'],water['corner'][1]+y*water['tile'],z]
                below=world(water,[p[0],p[1],z-.6]);above=world(water,[p[0],p[1],z+.6])
                if sample(rows,below)['environment']==2 and sample(rows,above)['environment']!=2:
                    add('below-water',below);add('above-water',above);break
    return dict(version=1,scope='Offline camera probes derived from original bounds/grids; no physical movement or swimming.',cases=result,dependencies=dependencies)

if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('folder',type=Path);p.add_argument('--maps',nargs='+',type=int,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args()
    result=probes(a.folder,a.maps);a.output.write_text(json.dumps(result,indent=2)+'\n')
    print(json.dumps({'cases':len(result['cases']),'maps':a.maps,'environments':sorted({x['environment'] for x in result['cases']})}))
