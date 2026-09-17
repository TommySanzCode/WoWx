"""Accept scoped global-WMO routing/collision/rendering; never full instance gameplay."""
import argparse,csv,json
from pathlib import Path
from stream_telemetry import full_coverage
p=argparse.ArgumentParser(description=__doc__);p.add_argument('capture',type=Path);p.add_argument('--manifest',type=Path,required=True);a=p.parse_args()
def read(path):
    with path.open(newline='') as f:return [{k:float(v) for k,v in row.items()} for row in csv.DictReader(f)]
def distribution(values):
    v=sorted(values);return {k:v[int((len(v)-1)*q)] if v else None for k,q in [('p50',.5),('p95',.95),('p99',.99),('max',1)]}
main=read(a.capture);rows=read(a.capture.with_suffix('.stream.csv'));manifest=json.loads(a.manifest.read_text())
if not main or not rows:raise SystemExit('Missing native samples')
errors=[]
def require(test,message):
    if not test:errors.append(message)
coverage=full_coverage(main,rows);require(coverage['passed'],'Missing, duplicate or foreign companion identities')
require(all(x['portrait_fixture']==13 and not x['world_active'] and not x['replay_active'] and not x['packets_received'] for x in main),'Wrong fixture or active gameplay/input')
require(all(x['fixture']==13 and not x['errors'] and x['budget_enabled']==1 for x in rows),'Fixture or frame-budget errors')
require(all(not x['failures'] and not x['region_failures'] for x in main),'Asset/region/fixture failure')
require(min(x['free_kib'] for x in main)>=8192,'Insufficient measured headroom')
require(max(x['cycles'] for x in rows)>=1,'No complete map cycle')
require(set(int(x['map']) for x in main)==set(manifest['maps']),'Missing or unexpected maps')
maps={}
for stage,(map_id,x,y,z) in enumerate(manifest['points']):
    sample=[r for r in main if r['scenario_stage']==stage]
    require(bool(sample),'Missing map '+str(map_id))
    if not sample:continue
    require(all(r['scenario_stage']==stage and abs(r['x']-x)<.01 and abs(r['y']-y)<.01 and abs(r['z']-z)<.01 for r in sample),'Wrong map/inspection point')
    require(any(r['scenario_frame']>=120 and r['region_loaded']>=1 and r['draws']>0 for r in sample),'No completed streamed collision/render hold for '+str(map_id))
    maps[(str(stage)+':'+str(map_id)) if manifest.get('environment') else str(map_id)]={'samples':len(sample),'minimum_free_kib':min(r['free_kib'] for r in sample),'maximum_draws':max(r['draws'] for r in sample),'frame_ms':distribution([r['frame_ms'] for r in sample]),'maximum_collision_ready_frames':max(r['scenario_frame'] for r in sample)}
for field,limit in [('read_bytes',65536),('read_ops',16),('scan_bytes',65536),('allocations',3)]:
    require(all(0<=r['total_budget_'+field]<=limit for r in rows),'Shared quota exceeded: '+field)
    require(all(r['total_budget_'+field]==sum(r[w+'_budget_'+field] for w in ('index','world','npc','player')) for r in rows),'Quota accounting mismatch')
result={'passed':not errors,'errors':errors,'coverage':coverage,'main_samples':len(main),'stream_samples':len(rows),
        'first_frame':main[0]['replay_frame'],'last_frame':main[-1]['replay_frame'],'cycles':max(r['cycles'] for r in rows),
        'minimum_free_kib':min(r['free_kib'] for r in main),'frame_ms':distribution([r['frame_ms'] for r in main[1:]]),
        'costs':{key:distribution([r[key] for r in rows]) for key in ('region_ms','stream_ms')},
        'maxima':{key:max(r[key] for r in rows) for key in ('index_bytes','pending_index_bytes','scene_bytes','total_budget_read_bytes','total_budget_read_ops','total_budget_scan_bytes','total_budget_allocations')},
        'maps':maps,'scope':manifest['scope']+' Timings are xemu guest clock, not physical Xbox results.'}
if manifest.get('environment'):
    env=read(a.capture.with_suffix('.environment.csv'));lights=read(a.capture.with_suffix('.lighting.csv'))
    identities={'environment':full_coverage(main,env),'lighting':full_coverage(main,lights)}
    for key,value in identities.items():require(value['passed'],'Missing '+key+' companion identity')
    require(all(not r['errors'] for r in env),'Environment fixture error')
    held=[];cases=[]
    by_id={(r['time_ms'],r['replay_frame']):r for r in main}
    light_by_id={(r['time_ms'],r['frame']):r for r in lights}
    for stage,case in enumerate(manifest['environment']['cases']):
        sample=[r for r in env if r['stage']==stage and by_id.get((r['time_ms'],r['frame']),{}).get('scenario_frame',0)>=30]
        require(len(sample)>=60,'Missing held environment case '+str(stage));held+=sample
        require(all(r['known']==1 and r['ready']==1 and not r['pending'] and r['environment']==case['environment'] and r['liquid_type']==case['liquid_type'] and abs(r['depth']-case['depth'])<.02 for r in sample),'Wrong environment/liquid/depth in case '+str(stage))
        for r in sample:
            light=light_by_id.get((r['time_ms'],r['frame']),{})
            require(light.get('environment')==case['environment'],'Fog environment mismatch')
            require(light.get('sky_quads')==(192 if case['environment']==0 else 0),'Sky state escaped environment')
            require(light.get('enabled')==(0 if case['environment']==1 else 1),'Fog enable state mismatch')
        cases.append({'stage':stage,'map':case['map'],'label':case['label'],'expected':case['environment'],'held_samples':len(sample)})
    require({r['environment'] for r in held}=={c['environment'] for c in manifest['environment']['cases']},'Missing requested environment coverage')
    result['environment']={'coverage':identities,'cases':cases,'minimum_known_records':min((r['records'] for r in held),default=0),'maximum_bytes':max((r['bytes'] for r in env),default=0),'maximum_pending':max((r['pending'] for r in env),default=0),'sample_ms':distribution([r['sample_ms'] for r in env])}
    result['passed']=not errors
if manifest.get('material_probe'):
    materials=read(a.capture.with_suffix('.materials.csv'));identity=full_coverage(main,materials)
    require(identity['passed'],'Material companion identity loss')
    main_by_id={(r['time_ms'],r['replay_frame']):r for r in main}
    require(all(sum(r['blend_'+str(i)] for i in range(7))==main_by_id.get((r['time_ms'],r['frame']),{}).get('draws') for r in materials),'Material draws differ from renderer count')
    probe=[r for r in materials if main_by_id[(r['time_ms'],r['frame'])]['map']==999 and main_by_id[(r['time_ms'],r['frame'])]['scenario_frame']>=120]
    require({r['fog'] for r in probe}=={0,1},'Missing fog on/off resident material probe')
    require(bool(probe) and all(all(r['blend_'+str(i)]>=2 for i in range(7)) for r in probe),'Missing submitted blend modes')
    require(bool(probe) and all(int(r['flags'])&0x3c00==0x3c00 for r in probe),'Missing material flag submissions')
    if manifest.get('vertex_lighting'):
        require(bool(probe) and all(int(r['flags'])&0x60000==0x60000 for r in probe),'Missing vertex colour/baked-light state')
        require(any(int(r['flags'])&0x60000==0x60000 and main_by_id[(r['time_ms'],r['frame'])]['map']!=999 for r in materials),'No original WMO vertex lighting submitted')
    if manifest.get('material_motion'):
        require(bool(probe) and all(r['motion_draws']==14 for r in probe),'Missing animated material submissions')
        require(len({r['motion_hash'] for r in probe})>100,'Material uniforms did not animate')
        if 429 in manifest['maps']:
            authored=[r for r in materials if main_by_id[(r['time_ms'],r['frame'])]['map']==429 and 20<=main_by_id[(r['time_ms'],r['frame'])]['scenario_frame']<=60 and r['motion_draws']>0]
            require(len(authored)>20 and len({r['motion_hash'] for r in authored})>20,'Missing authored Dire Maul material variation')
    result['materials']={'coverage':identity,'probe_frames':len(probe),'maximum_by_mode':[max((r['blend_'+str(i)] for r in materials),default=0) for i in range(7)],'scope':'GPU submissions and screenshots; no pixel-exact original-client comparison.'}
    if manifest.get('material_motion'):result['materials']['motion_probe_hashes']=len({r['motion_hash'] for r in probe})
    result['passed']=not errors
if manifest.get('liquids'):
    materials=read(a.capture.with_suffix('.materials.csv'));identity=full_coverage(main,materials)
    require(identity['passed'],'Missing liquid/material companion identities')
    by_id={(r['time_ms'],r['replay_frame']):r for r in main}
    require(all(sum(r['blend_'+str(i)] for i in range(7))==by_id.get((r['time_ms'],r['frame']),{}).get('draws') for r in materials),'Liquid pass lost ordinary material submissions')
    require(all(r['liquid_draws']==r['sequence_draws'] and (int(r['sequence_frames'])&~0x3fffffff)==0 for r in materials),'Invalid liquid texture frame submission')
    submitted={}
    for map_id in manifest['maps']:
        seen=[r for r in materials if by_id[(r['time_ms'],r['frame'])]['map']==map_id and r['liquid_draws']>0]
        mask=0
        for r in seen:mask|=int(r['sequence_frames'])
        if seen:
            require(mask==0x3fffffff,'Not all original texture frames submitted on map '+str(map_id))
            submitted[str(map_id)]={'samples':len(seen),'frame_mask':mask,'maximum_liquid_draws':max(r['liquid_draws'] for r in seen)}
    require(len(submitted)>=(1 if manifest.get('terrain') else 2),'Missing original liquid maps')
    require(any(c['environment']==2 for c in manifest['environment']['cases']),'Missing below-water view')
    if manifest.get('terrain'):
        requested={(r['map'],r['x'],r['y']) for r in manifest['regions']}
        visited={(int(r['map']),int(r['tile_x']),int(r['tile_y'])) for r in main if r['scenario_frame']>=120}
        require(requested<=visited,'A terrain region did not complete its inspection hold')
        terrain_stages={i for i,c in enumerate(manifest['environment']['cases']) if c.get('source_kind')=='MCLQ'}
        require(bool(terrain_stages),'Missing original MCLQ cases')
        drawn={int(by_id[(r['time_ms'],r['frame'])]['scenario_stage']) for r in materials if r['liquid_draws']>0}
        require(terrain_stages<=drawn,'A terrain liquid probe drew no liquid surface')
        result['terrain_regions']={'requested':sorted(requested),'visited':sorted(visited),'scope':'Synthetic camera relocations, not collision-constrained walking.'}
    else:
        require(any(c['liquid_type']==3 for c in manifest['environment']['cases']),'Missing original magma view')
        lava_maps={c['map'] for c in manifest['environment']['cases'] if c['liquid_type']==3}
        require(any(str(m) in submitted for m in lava_maps),'Original magma was not drawn')
    result['liquids']={'coverage':identity,'maps':submitted,'scope':'Original-data surfaces and all 30 texture frames submitted; screenshots separate. No swimming, pixel parity, live actors beneath water or physical controls verified.'}
    result['passed']=not errors

a.capture.with_suffix('.global-check.json').write_text(json.dumps(result,indent=2)+'\n')
print(json.dumps(result,indent=2));raise SystemExit(bool(errors))
