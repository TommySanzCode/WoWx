"""Versioned terrain/global-WMO index shared by preparation and staging tools."""
import struct

def filename(map_id,x,y,kind=0):
    if not 0<=map_id<=999 or not 0<=x<64 or not 0<=y<64 or kind not in (0,1) or (kind and (x or y)):
        raise ValueError('Invalid world region identity')
    return f'G{map_id:03}.WXP' if kind else f'M{map_id:03}{x:02}{y:02}.WXP'

def decode(data):
    if len(data)<16:raise ValueError('Truncated world index')
    magic,version,count,stride=struct.unpack_from('<4sIII',data)
    if magic!=b'WXI1' or version not in (1,2) or not 0<count<=8192 or stride!=32 or len(data)!=16+count*32:
        raise ValueError('Invalid world index header')
    result=[];previous=None
    for map_id,x,y,name,size,kind in struct.iter_unpack('<IHH16sII',data[16:]):
        expected=filename(map_id,x,y,kind)
        if (version==1 and kind) or name!=expected.encode().ljust(16,b'\0') or not 32<size<1024**3:
            raise ValueError('Invalid world index record')
        key=(map_id,x,y)
        if previous and (key<=previous[:3] or (map_id==previous[0] and (kind or previous[3]))):
            raise ValueError('Duplicate, unsorted or mixed map regions')
        result.append(dict(map=map_id,x=x,y=y,file=expected,bytes=size,kind=kind));previous=(*key,kind)
    return result

def encode(rows):
    data=bytearray(struct.pack('<4sIII',b'WXI1',2 if any(r.get('kind',0) for r in rows) else 1,len(rows),32))
    for r in rows:
        kind=r.get('kind',0);name=filename(r['map'],r['x'],r['y'],kind)
        if r['file']!=name:raise ValueError('Mismatched region filename')
        data+=struct.pack('<IHH16sII',r['map'],r['x'],r['y'],name.encode(),r['bytes'],kind)
    decode(data)
    return bytes(data)
