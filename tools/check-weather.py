"""Scoped native Vanilla weather-state/lighting acceptance, not complete weather."""
import argparse,csv,json
from pathlib import Path
p=argparse.ArgumentParser(description=__doc__);p.add_argument('capture',type=Path);a=p.parse_args()
read=lambda path:[{k:float(v) for k,v in r.items()} for r in csv.DictReader(path.open())]
rows=read(a.capture);weather=read(a.capture.with_suffix('.weather.csv'));light=read(a.capture.with_suffix('.lighting.csv'));errors=[]
def require(ok,message):
    if not ok:errors.append(message)
def distribution(values):
    v=sorted(values)
    return {k:v[min(len(v)-1,int((len(v)-1)*q))] for k,q in (('p50',.5),('p95',.95),('p99',.99),('max',1))} if v else {}
require(len(rows)>2400 and len(weather)>2400 and len(light)>2400,'Too few samples')
require(all(r['portrait_fixture']==10 for r in rows),'Wrong fixture identity')
require(rows and min(r['free_kib'] for r in rows)>=8192,'Less than 8 MiB free')
require(all(r['failures']==0 and r['ui_failures']==0 for r in rows),'Asset/UI failures')
require(all(r['errors']==0 and r['fixture']==10 and r['revision']==r['mix_revision'] for r in weather),'Weather parse/presentation failure')
require(all(r['catalog_bytes']==451040 and r['failures']==0 and r['wire_version']==3 for r in light),'Lighting catalog failure')
require(all(r['npc_submitted']==3 and r['npc_missing']==0 and r['portrait_target_draws']>0 for r in rows if r['replay_frame']>300),'Actor/portrait rendering incomplete')
lighting={r['frame']:r for r in light};phases={}
types=[0,1,1,2,3,0,1,1,0,3];sounds=[0,8533,8535,8538,8558,0,8535,8535,0,8556];grades=[0,.25,1,.65,.8,0,1,1,0,.4]
for phase in range(10):
    samples=[r for r in weather if r['phase']==phase and r['frame']%240>=180]
    require(len(samples)>=40,f'Phase {phase} not observed after settling');phases[phase]=samples[-1] if samples else {}
    for r in samples:
        require(r['type']==types[phase] and r['sound']==sounds[phase] and abs(r['grade']-grades[phase])<.0001 and abs(r['weight']-grades[phase])<.01,f'Phase {phase}: wrong server weather/weight')
        require(r['valid']==(phase!=8),f'Phase {phase}: reset state wrong')
        l=lighting.get(r['frame'])
        if l:
            require(l['sky_quads']==(0 if phase in (6,7) else 192),f'Phase {phase}: sky override wrong')
            require(l['enabled']==(phase!=6),f'Phase {phase}: fog override wrong')
for phase,expected in ((3,-15.625),(7,-72.5),(8,90),(9,25)):
    instant=[r for r in weather if r['phase']==phase and r['frame']%240==0 and r['frame'] in lighting]
    require(bool(instant),f'Phase {phase}: missing instant edge')
    for r in instant:
        require(r['snap']==1 and abs(lighting[r['frame']]['fog_start']-expected)<.01,f'Phase {phase}: instant change was smoothed or delayed')
for phase,rising in ((1,True),(2,True),(4,True),(5,False)):
    early=[r for r in weather if r['phase']==phase and r['frame']%240<30]
    require(bool(early),f'Phase {phase}: missing smooth edge')
    if early:require((early[0]['weight']<grades[phase] if rising else early[0]['weight']>0) and not early[0]['snap'],f'Phase {phase}: smooth change snapped')
result={'scope':'Offline resident world with synthetic Vanilla 13-byte weather packets, grade-driven lighting, instant/smooth transitions, reset/rejection and explicit environment overrides. No precipitation/audio, live server account, traversal, physical input or hardware acceptance.',
        'pass':not errors,'errors':sorted(set(errors)),'samples':len(rows),'weather_samples':len(weather),'lighting_samples':len(light),'minimum_free_kib':min((r['free_kib'] for r in rows),default=0),
        'guest_frame_ms':distribution([r['frame_ms'] for r in rows]),'light_work_ms':distribution([r['light_ms'] for r in light]),'phases':phases}
output=a.capture.with_suffix('.weather-check.json');output.write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result,indent=2));raise SystemExit(bool(errors))
