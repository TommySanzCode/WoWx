"""Check the scoped offline native atmosphere fixture, never full-game acceptance."""
import argparse,csv,json,statistics
from pathlib import Path
p=argparse.ArgumentParser(description=__doc__);p.add_argument('capture',type=Path);a=p.parse_args()
read=lambda path:[{k:float(v) for k,v in r.items()} for r in csv.DictReader(path.open())]
rows=read(a.capture);light=read(a.capture.with_suffix('.lighting.csv'));errors=[]
def require(ok,message):
    if not ok:errors.append(message)
def distribution(values):
    v=sorted(values)
    return {k:v[min(len(v)-1,int((len(v)-1)*q))] for k,q in (('p50',.5),('p95',.95),('p99',.99),('max',1))} if v else {}
require(len(rows)>2000 and len(light)>2000,'Too few samples')
require(all(r['portrait_fixture']==9 for r in rows),'Incorrect fixture identity')
require(rows and min(r['free_kib'] for r in rows)>=8192,'Less than 8 MiB free')
require(all(r['failures']==0 and r['ui_failures']==0 for r in rows),'Asset/UI failures')
require(all(r['failures']==0 and r['catalog_bytes']==451040 and r.get('wire_version')==3 for r in light),'Catalog load/identity failure')
require(all(r['npc_submitted']==3 and r['npc_missing']==0 for r in rows if r['replay_frame']>300),'Incomplete scaled actor rendering')
require(all(r['portrait_target_key']==447 and r['portrait_target_draws']>0 for r in rows if r['replay_frame']>300),'Missing independently lit portrait')
phases={}
for phase in range(8):
    samples=[r for r in light if r['phase']==phase and r['frame']%240>=180]
    require(len(samples)>=40,f'Phase {phase} not observed after transition');phases[phase]=samples[-1] if samples else {}
    if samples:
        last=samples[-1];require(last['enabled']==(phase not in (5,7)),f'Phase {phase}: wrong fog state')
        if phase<5:require(last['authored']==1 and last['global_profile'] in (12,13,10),f'Phase {phase}: missing authored fog')
        require(last['sky_quads']==(0 if phase in (4,7) else 192),f'Phase {phase}: incorrect sky environment')
        if phase<5:require(last['palette_mask']==255,f'Phase {phase}: incomplete authored palette')
if all(phases[i] for i in (0,1,2)):
    require(len({phases[i]['rgb'] for i in (0,1,2)})==3,'No day/night/dawn variation')
    require(len({phases[i]['ambient'] for i in (0,1,2)})==3,'No day/night/dawn material variation')
    require(len({phases[i]['sky_top'] for i in (0,1,2)})==3,'No day/night/dawn sky variation')
    require(phases[0]['direction_z']>0 and phases[1]['direction_z']<0,'Reversed day/night light direction')
result={'scope':'Offline resident Northshire fog/material/sky colors, independent portrait and synthetic conditions/clock/non-unit actors. Not complete sky/weather, streaming, live gameplay, physical input or hardware.',
        'pass':not errors,'errors':errors,'samples':len(rows),'lighting_samples':len(light),
        'minimum_free_kib':min((r['free_kib'] for r in rows),default=0),
        'guest_frame_ms':distribution([r['frame_ms'] for r in rows]),'light_work_ms':distribution([r['light_ms'] for r in light]),'phases':phases}
output=a.capture.with_suffix('.lighting-check.json');output.write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result,indent=2));raise SystemExit(bool(errors))
