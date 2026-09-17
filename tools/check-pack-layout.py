"""Check that regrouping animation headers preserves every entry's payload."""
import argparse
import hashlib
import json
import struct
from pathlib import Path

def inspect(path):
    data = path.read_bytes()
    magic,version,count,stride,*rest = struct.unpack_from('<4s3I3fI',data)
    assert magic==b'WXP1' and version in (5,6,7,8) and stride==64 and rest[-1]==len(data)
    result=[];offsets=[]
    for i in range(count):
        e=struct.unpack_from('<9I4f3I',data,32+i*stride)
        kind,ident,vertices,indices,vo,io,to,width,height=e[:9]
        flags,animation,levels=e[13:]
        assert flags&4 and not flags&96 # Only animated uncompressed avatar entries.
        frames,bones,duration,skin,poses=struct.unpack_from('<5I',data,animation)
        pixels=0;w=width;h=height
        for level in range(levels or 1):pixels+=w*h*4;w//=2;h//=2
        digest=hashlib.sha256()
        for at,size in ((vo,vertices*32),(io,indices*2),(to,pixels),(skin,vertices*8),(poses,frames*bones*48)):
            assert 32+count*stride<=at<=len(data) and size<=len(data)-at
            digest.update(data[at:at+size])
        result.append((kind,ident,vertices,indices,width,height,*e[9:14],levels,frames,bones,duration,digest.hexdigest()))
        offsets.append(animation)
    return result,offsets

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('old',type=Path);p.add_argument('new',type=Path);p.add_argument('--output',type=Path,required=True);a=p.parse_args()
    report=[]
    for original in sorted(a.old.glob('A*.WXP')):
        before,old=inspect(original);after,new=inspect(a.new/original.name)
        assert before==after, f'Payload/animation changed: {original.name}'
        assert new==list(range(new[0],new[0]+len(new)*20,20)), f'Headers not contiguous: {original.name}'
        assert len(new)*20<=65536, f'Index exceeds bounded prefetch: {original.name}'
        report.append({'file':original.name,'entries':len(new),'previous_header_reads':len(old),'new_header_reads':1,'index_bytes':len(new)*20})
    assert len(report)==16
    result={'passed':True,'profiles':report,'scope':'All vertex/index/texture/skin/pose payloads and entry semantics identical; only file layout changed. Read counts describe the runtime index path, not measured hardware timing.'}
    a.output.write_text(json.dumps(result,indent=2)+'\n')
    print(f'{len(report)} profiles: all payloads identical; animation header reads {min(r["previous_header_reads"] for r in report)}-{max(r["previous_header_reads"] for r in report)} -> 1 per profile.')

if __name__=='__main__':main()
