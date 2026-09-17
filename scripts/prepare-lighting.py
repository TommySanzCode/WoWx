"""Prepare private Vanilla fog data and an archive-content coverage receipt."""
import argparse,collections,hashlib,json,struct,subprocess,tempfile
from pathlib import Path
from workspace_paths import game_data
ROOT=Path(__file__).resolve().parents[1]
def digest(p):
    with p.open('rb') as f:return hashlib.file_digest(f,'sha256').hexdigest()
def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--data',type=Path,default=game_data())
    parser.add_argument('--output',type=Path,default=ROOT/'build/xbox/LIGHT.WLF')
    parser.add_argument('--report',type=Path,default=ROOT/'build/evidence/lighting-assets.json')
    a=parser.parse_args();cooker=ROOT/'build/host/wowx_assetc.exe';a.output.parent.mkdir(parents=True,exist_ok=True)
    inputs={};tables={}
    with tempfile.TemporaryDirectory(prefix='lighting-',dir=ROOT/'build') as folder:
        for name in ('Light','LightIntBand','LightFloatBand'):
            path=Path(folder)/(name+'.dbc')
            subprocess.run([str(cooker),'--extract',str(a.data),'DBFilesClient\\'+name+'.dbc',str(path)],check=True)
            b=path.read_bytes();count,fields,size,strings=struct.unpack_from('<4I',b,4)
            inputs[name]={'sha256':digest(path),'bytes':len(b),'rows':count,'fields':fields}
            tables[name]=[struct.unpack_from('<'+str(fields)+'I',b,20+i*size) for i in range(count)]
        subprocess.run([str(cooker),'--lighting',str(a.data),str(a.output)],check=True)
    blob=a.output.read_bytes();_,version,nv,np,total,*_=struct.unpack_from('<8I',blob)
    assert version==2
    empty=[]
    for i in range(np):
        offset=32+nv*40+i*1104;profile=struct.unpack_from('<I',blob,offset)[0]
        absent=[b for b in range(11) if struct.unpack_from('<I',blob,offset+4+b*100)[0]==0]
        if absent:empty.append({'profile':profile,'bands':absent,'volumes':[r[0] for r in tables['Light'] if profile in r[7:10]]})
    maps=[]
    for mapid,count in sorted(collections.Counter(r[1] for r in tables['Light']).items()):
        maps.append({'map':mapid,'volumes':count,'global_profiles':[r[7] for r in tables['Light'] if r[1]==mapid and r[6]==0]})
    high_bytes=[{'band_id':r[0],'profile':(r[0]-1)//18+1,'channel':(r[0]-1)%18,'keys':[k for k in range(r[1]) if r[18+k]>0xffffff]}
                for r in tables['LightIntBand'] if (r[0]-1)%18 in (0,1,2,3,4,5,6,7,9) and any(r[18+k]>0xffffff for k in range(r[1]))]
    report={'scope':'Fog, material and sky color data coverage, not prepared geometry or gameplay coverage.','version':version,
            'rgb_high_bytes_ignored_like_wowee':high_bytes,
            'source_dbcs':inputs,'cooker_sha256':digest(cooker),'output':str(a.output.resolve()),'sha256':digest(a.output),
            'file_bytes':total,'resident_bytes':total-32,'volumes':nv,'profiles':np,'maps':maps,'empty_source_profiles':empty,
            'fallback_policy':'Missing map/default/bands or nonpositive fog end uses explicit fallback; indoor classification and weather/water detection remain pending.'}
    a.report.parent.mkdir(parents=True,exist_ok=True);a.report.write_text(json.dumps(report,indent=2)+'\n')
    print(f'Fog receipt: {a.report}; {len(empty)} empty source profiles retained explicitly.')
if __name__=='__main__':main()
