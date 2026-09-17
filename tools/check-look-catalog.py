"""Cross-check prepared appearance keys/geosets against the supplied Vanilla DBCs."""
import argparse,json,struct
from pathlib import Path

def dbc(path):
    b=path.read_bytes();assert b[:4]==b'WDBC'
    n,c,stride,strings=struct.unpack_from('<4I',b,4)
    assert stride==4*c and len(b)==20+n*stride+strings
    return list(struct.iter_unpack('<'+'I'*c,b[20:20+n*stride]))

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('dbc',type=Path);p.add_argument('catalogs',type=Path)
    p.add_argument('--references',action='store_true');p.add_argument('--output',type=Path);a=p.parse_args()
    sections=dbc(a.dbc/'CharSections.dbc');hair=dbc(a.dbc/'CharHairGeosets.dbc');facial=dbc(a.dbc/'CharacterFacialHairStyles.dbc')
    result=[]
    for race in range(1,9):
        for sex in range(2):
            path=a.catalogs/f'L{race:02X}{sex:02X}.WXL';b=path.read_bytes()
            magic,version,size,count,stride,rr,ss,data=struct.unpack_from('<4s7I',b)
            assert (magic,version,size,stride,rr,ss,data)==(b'WXLK',1,len(b),40,race,sex,32+count*40)
            rows=[struct.unpack_from('<4B9I',b,32+i*40) for i in range(count)]
            actual={r[:3]:r for r in rows};assert len(actual)==count
            expected={(r[3],r[4],r[5]) for r in sections if r[1:3]==(race,sex) and not r[9]&1}
            assert {k for k in actual if k[0]<5}==expected,(race,sex,'section key mismatch')
            hg={r[3]:max(1,r[4]) for r in hair if r[1:3]==(race,sex)}
            fg={r[2]:(r[6],r[8],r[7]) for r in facial if r[:2]==(race,sex)}
            assert {k[1]:r[10] for k,r in actual.items() if k[0]==5}==hg
            assert {k[1]:r[10:13] for k,r in actual.items() if k[0]==6}==fg
            samples=[]
            skin_ids={k[2] for k in expected if k[0]==0}
            face_colors={k[2] for k in expected if k[0]==1}
            hair_colors={k[2] for k in expected if k[0]==3}
            feature_colors={k[2] for k in expected if k[0]==2 and k[1] in fg}
            needs_feature=race!=6 and (sex==0 or race in (4,5))
            reachable={k for k in expected if
                (k[0] in (0,1,4) and k[2] in skin_ids&face_colors) or
                (k[0]==2 and k[1] in fg and k[2] in hair_colors) or
                (k[0]==3 and (not needs_feature or k[2] in feature_colors))}
            if a.references:
                raw=(a.catalogs/f'R{race:02X}{sex:02X}.WXV').read_bytes();magic,n,stride=struct.unpack_from('<4sII',raw)
                assert magic==b'WXLR' and stride==56 and len(raw)==12+n*stride
                samples=[struct.unpack_from('<14I',raw,12+i*stride)[:7] for i in range(n)]
                covered=set()
                for r,s,skin,face,style,color,feature in samples:
                    assert (r,s)==(race,sex)
                    covered.update(((0,0,skin),(1,face,skin),(2,feature,color),(3,style,color),(4,0,skin)))
                assert not reachable-covered,(race,sex,'reference coverage missing',sorted(reachable-covered))
            result.append({'race':race,'sex':sex,'rows':count,'bytes':len(b),'sections':len(expected),
                           'excluded_unavailable':sum(r[1:3]==(race,sex) and bool(r[9]&1) for r in sections),
                           'skin_ids':sorted(skin_ids),'hair_styles':sorted({k[1] for k in expected if k[0]==3}),
                           'unreachable_section_keys':sorted(expected-reachable),
                           'facial_features':sorted(fg),'reference_compositions':len(samples)})
    report={'passed':True,'profiles':result,'scope':'Prepared keys and geosets match supplied DBCs; pixel/runtime references are checked separately.'}
    text=json.dumps(report,indent=2)+'\n'
    if a.output:a.output.write_text(text)
    print(f"16 appearance catalogues match Vanilla DBCs; {sum(r['sections'] for r in result)} section rows, {sum(r['reference_compositions'] for r in result)} reference compositions.")
if __name__=='__main__':main()
