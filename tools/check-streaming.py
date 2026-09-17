"""Check isolated streaming traversal and report measured costs without gameplay claims."""
import argparse,csv,json
from pathlib import Path
from stream_telemetry import full_coverage
p=argparse.ArgumentParser(description=__doc__);p.add_argument('capture',type=Path);p.add_argument('--actors',action='store_true');p.add_argument('--scheduled',action='store_true');p.add_argument('--frame-budget',action='store_true');p.add_argument('--appearance',action='store_true');p.add_argument('--output',type=Path);a=p.parse_args()
if a.appearance and not (a.actors and a.frame_budget):p.error('--appearance requires --actors --frame-budget')
if a.scheduled and not a.actors:p.error('--scheduled requires --actors')
def read(path):
    with path.open(newline='') as f:return [{k:float(v) for k,v in row.items()} for row in csv.DictReader(f)]
main=read(a.capture);aux=read(a.capture.with_suffix('.stream.csv'));errors=[]
def require(test,message):
    if not test:errors.append(message)
def distribution(values):
    v=sorted(values)
    return {k:v[int((len(v)-1)*q)] if v else None for k,q in [('p50',.5),('p95',.95),('p99',.99),('max',1)]}
require(bool(main) and bool(aux),'Missing native samples')
if not main or not aux:raise SystemExit('\n'.join(errors))
mode=11 if a.appearance else 8 if a.actors else 7
require(all(x['portrait_fixture']==mode and not x['world_active'] for x in main),'Unexpected fixture or active account')
require(all(x['fixture']==mode for x in aux),'Unexpected companion fixture')
require(min(x['free_kib'] for x in main)>=8192,'Insufficient measured headroom')
require(max(x['failures']+x['region_failures'] for x in main)==0,'Asset/region failure')
require(max(x['errors'] for x in aux)==0,'Traversal failure')
require(max(x['cycles'] for x in aux)>=1,'No complete boundary/Abbey/camp cycle')
require(set(range(8)).issubset({int(x['stage']) for x in aux}),'Missing traversal stage')
for field,limit in [('read_bytes',65536),('read_ops',8),('scan_bytes',65536),('index_read_bytes',32768),('index_read_ops',8),('index_checked',512)]:
    require(max(x[field] for x in aux)<=limit,'Quota exceeded: '+field)
require({(32,48),(32,49)}.issubset({(int(x['tile_x']),int(x['tile_y'])) for x in main if x['region_loaded']}),'Both terrain tiles not crossed')
require(any(x['scenario_stage']==3 and x['x']<-9080 for x in main),'Boundary south leg missing')
require(any(x['scenario_stage']==5 and x['x']>-8906 and x['y']<-159 for x in main),'Abbey interior endpoint missing')
require(any(x['scenario_stage']==6 and x['x']>-8804 for x in main),'Camp endpoint missing')
lookup={(int(x['time_ms']),int(x['frame'])):x for x in aux};paired=[]
for i,row in enumerate(main[:-1]):
    key=(int(row['time_ms']),int(row['replay_frame']))
    if key in lookup:
        paired.append(dict(lookup[key],frame_interval_ms=main[i+1]['frame_ms']))
require(len(paired)>=len(main)*.9,'Too few aligned companion samples')
require(all(x['fog']==(x['stage']!=1) for x in aux),'Fog comparison phase mismatch')
if a.frame_budget:
    coverage=full_coverage(main,aux)
    require(coverage['passed'],'Incomplete or duplicate stream frame identities; cannot accept shared quotas')
    require(all(x.get('budget_enabled')==1 for x in aux),'Shared frame budget not active')
    for field,limit in [('read_bytes',65536),('read_ops',16),('scan_bytes',65536),('allocations',3)]:
        require(all(0<=x.get('total_budget_'+field,-1)<=limit for x in aux),'Combined quota exceeded: '+field)
        require(all(x.get('total_budget_'+field,-1)==sum(x.get(who+'_budget_'+field,-1) for who in ('index','world','npc','player')) for x in aux),'Quota accounting mismatch: '+field)
result={'passed':not errors,'errors':errors,'main_samples':len(main),'stream_samples':len(aux),'aligned_samples':len(paired),
    'cycles':max(x['cycles'] for x in aux),'minimum_free_kib':min(x['free_kib'] for x in main),
    'frame_ms':distribution([x['frame_ms'] for x in main[1:]]),
    'costs':{field:distribution([x[field] for x in paired]) for field in ['region_ms','stream_ms','frame_interval_ms']},
    'stage_costs':{str(stage):{field:distribution([x[field] for x in paired if x['stage']==stage]) for field in ['region_ms','stream_ms','frame_interval_ms']} for stage in range(8)},
    'maxima':{field:max(x[field] for x in aux) for field in ['read_bytes','read_ops','scan_bytes','pending_bytes','pending_index_bytes','index_bytes','scene_bytes','waits','errors','cancelled','index_read_bytes','index_checked']},
    'worst_frames':sorted(paired,key=lambda x:x['frame_interval_ms'],reverse=True)[:12],
    'scope':'Real packs, production streamer/collision/renderer, deterministic offline movement and explicit phase-4 relocation. No entities, account, combat, input replay or physical controller. xemu guest timing only. Not a full-world or hardware release gate.'}
if a.actors:
    actors=read(a.capture.with_suffix('.actors.csv'));require(bool(actors),'Missing actor telemetry')
    if a.frame_budget:
        actor_coverage=full_coverage(main,actors)
        require(actor_coverage['passed'],'Incomplete or duplicate actor frame identities; cannot accept shared quotas')
    if actors:
        require(all(x['fixture']==mode for x in actors),'Wrong actor fixture')
        require(max(x['actor_failures']+x['avatar_failures']+x['avatar_gaps'] for x in actors)==0,'Actor failure or incomplete avatar gap')
        require(max(x['npc_missing']+x['avatar_missing'] for x in main)==0,'Missing prepared actor/avatar assets')
        for who in ('actor','avatar'):
            for field,limit in [('read_bytes',65536),('read_ops',8),('scan_bytes',65536)]:require(max(x[who+'_'+field] for x in actors)<=limit,'Actor quota exceeded: '+who+'_'+field)
        require({0,5,16,6}.issubset({int(x['avatar_clip']) for x in actors if x['avatar_ready']}),'Incomplete idle/run/attack/death clip coverage')
        require(max(x['actors_drawn'] for x in actors)>=6,'Too few visible synthetic actors')
        require(all(x['actors_fallback']==8 for x in actors if x['frame']>300),'A requested synthetic unit lost its complete model')
        require(max(x['switches'] for x in actors)>=100,'Rapid idle/run switching not exercised')
        if a.scheduled:
            require(all(x.get('actor_sampled',-1)>=0 for x in actors),'Scheduled animation metrics missing')
            require(any(x.get('actor_skipped',0)>0 for x in actors),'No distant animation work skipped')
            require(any(0<x.get('avatar_palettes',0)<x.get('avatar_sampled',0) for x in actors),'No shared avatar palette reuse')
            for who in ('actor','avatar'):
                require(all(0<=x.get(who+'_palettes',-1)<=x.get(who+'_sampled',-1)<=320 for x in actors),'Invalid animation work: '+who)
        result['actors']={'samples':len(actors),'actor_ms':distribution([x['actor_ms'] for x in actors]),'minimum_fallback_count':min(x['actors_fallback'] for x in actors if x['frame']>300),
            'maxima':{field:max(x[field] for x in actors) for field in ['actor_bytes','avatar_bytes','actor_read_bytes','avatar_read_bytes','actor_read_ops','avatar_read_ops','actor_scan_bytes','avatar_scan_bytes','actor_pending_bytes','avatar_pending_bytes','avatar_gaps','actor_failures','avatar_failures','switches']}}
        result['scope']='Real world/NPC/avatar assets and production streaming/animation/rendering; eight synthetic units and one equipped Human over offline collision routes, explicit phase-4 relocation. No server gameplay, combat, input replay or physical controller/hardware acceptance.'
        if a.appearance:
            require(all(x.get('compose_failures',-1)==0 for x in actors),'Composition failure or absent telemetry')
            require(max(x.get('compose_commits',0) for x in actors)>=12,'Too few completed appearance changes')
            require(max(x.get('compose_cancelled',0) for x in actors)>=10,'Rapid appearance cancellation not exercised')
            require(len({x.get('compose_hash') for x in actors if x['avatar_ready']})>=3,'Missing distinct composed appearances')
            for field,limit in [('read_bytes',65536),('read_ops',8),('work_pixels',8192)]:
                require(all(0<=x.get('compose_'+field,-1)<=limit for x in actors),'Appearance work limit: '+field)
            result['appearance']={field:distribution([x['compose_'+field] for x in actors]) for field in ('read_bytes','read_ops','work_pixels','commits','cancelled')}
            result['scope']+=' Includes same-profile appearance/equipment changes with atomic-published-pixel probes; profile switching is outside this same-profile scenario.'
        if a.scheduled:
            result['animation']={who:{field:distribution([x[who+'_'+field] for x in actors]) for field in ('sampled','skipped','vertices','palettes')} for who in ('actor','avatar')}
if a.frame_budget:
    result['coverage']={'stream':coverage}
    if a.actors:result['coverage']['actors']=actor_coverage
    result['frame_budget']={who:{field:distribution([x[who+'_budget_'+field] for x in aux]) for field in ('read_bytes','read_ops','scan_bytes','allocations')} for who in ('total','index','world','npc','player')}
result['passed']=not errors
(a.output or a.capture.with_suffix('.stream-check.json')).write_text(json.dumps(result,indent=2)+'\n')
print(json.dumps({k:v for k,v in result.items() if k not in ('worst_frames','stage_costs')},indent=2))
raise SystemExit(bool(errors))
