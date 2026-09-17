"""Stream-check repeated native walking/UI/reconnect laps without loading the CSV into RAM."""
import argparse,collections,csv,json,math
from pathlib import Path
p=argparse.ArgumentParser(description=__doc__);p.add_argument('capture',type=Path)
p.add_argument('--minimum-seconds',type=int,default=3600);p.add_argument('--minimum-cycles',type=int,default=8)
a=p.parse_args();samples=0;first=None;last={};previous=None;baseline=None;distance=0.;max_gap=0
minimum_free=1<<30;failures=0;ordinary=True;progress=True;avatar=True;kind=True;combat=False
cycles={};frames=collections.Counter();walking_samples=0
progress_keys=('player_guid_low','player_guid_high','player_level','player_xp','inventory_count','inventory_money','active_quests')
with a.capture.open(newline='') as source:
 for r in csv.DictReader(source):
    samples+=1;last=r
    if first is None:first=r
    if previous:max_gap=max(max_gap,int(r['time_ms'])-int(previous['time_ms']))
    minimum_free=min(minimum_free,int(r['free_kib']));frames[int(r['frame_ms'])]+=1
    kind &= r['scenario_kind']=='17'
    failures+=any(int(r[k]) for k in ('failures','region_failures','avatar_select_failures','book_failures','map_failures','text_failures')) or r['lobby_phase']=='4'
    combat |= any(int(r[k]) for k in ('kills','casts_accepted','casts_rejected','looted_items','bundles_bought','items_sold','spirit_releases','corpse_reclaims'))
    if r['world_active']=='1' and int(r['soak_started']):
        if baseline is None and r['scenario_stage']=='4' and int(r['player_level']) and int(r['inventory_count']):baseline=r.copy()
        if previous and previous['world_active']=='1' and previous['player_guid_low']==r['player_guid_low'] and previous['world_revision']==r['world_revision']:
            d=math.hypot(float(r['x'])-float(previous['x']),float(r['y'])-float(previous['y']))
            dt=(int(r['time_ms'])-int(previous['time_ms']))/1000;distance+=d;ordinary &= dt>=0 and d<=dt*8+.5
        if int(r['scenario_stage']) in (20,21,28,29):
            walking_samples+=1;avatar &= r['avatar_ready']=='1' and r['avatar_matched']=='1' and r['avatar_missing']=='0'
        if r['scenario_stage']=='50':
            cycle=int(r['soak_cycles']);c=cycles.setdefault(cycle,{'ui':set(),'maps':set(),'world_revision':int(r['world_revision']),'minimum_free_kib':1<<30,'last_free_kib':0})
            for key in ('map_open','utility_open','inventory_open','book_open','journal_open','menu'):
                if r[key]=='1':c['ui'].add(key)
            if r['map_open']=='1':c['maps'].add(int(r['map_id']))
            c['minimum_free_kib']=min(c['minimum_free_kib'],int(r['free_kib']));c['last_free_kib']=int(r['free_kib'])
            if baseline:progress &= all(r[k]==baseline[k] for k in progress_keys)
    previous=r
expected_ui={'map_open','utility_open','inventory_open','book_open','journal_open','menu'}
lifecycle_path=a.capture.with_suffix('.lifecycle.json')
lifecycle=json.loads(lifecycle_path.read_text()) if lifecycle_path.exists() else None
checks={
 'correct_scenario':samples>0 and kind,
 'completed':last.get('scenario_stage')=='8' and last.get('world_active')=='1',
 'duration':int(last.get('soak_elapsed_ms',0))>=a.minimum_seconds*1000 and int(last.get('soak_limit_ms',0))>=a.minimum_seconds*1000,
 'repeated_complete_laps':len(cycles)>=a.minimum_cycles and int(last.get('soak_cycles',0))==len(cycles),
 'all_menus_every_lap':bool(cycles) and all(c['ui']==expected_ui and {14,30}<=c['maps'] for c in cycles.values()),
 'reconnect_every_lap':len({c['world_revision'] for c in cycles.values()})==len(cycles) and bool(cycles),
 'ordinary_walking':ordinary and distance>=len(cycles)*350 and walking_samples>300,
 'continuous_walking_avatar':avatar and walking_samples>0,
 'progress_preserved':progress and bool(baseline) and all(last.get(k)==baseline[k] for k in progress_keys),
 'returned_to_start':bool(baseline) and all(abs(float(last[k])-float(baseline[k]))<.08 for k in ('x','y','z')) and abs(math.remainder(float(last['yaw'])-float(baseline['yaw']),2*math.pi))<.005,
 'menus_closed_map_freed':bool(last) and all(last[k]=='0' for k in ('map_open','map_ready','map_bytes','utility_open','inventory_open','book_open','journal_open','menu')),
 'no_combat_or_trade':not combat,
 'headroom':samples>0 and minimum_free>=8192,
 'no_failures':samples>0 and failures==0,
 'continuous_capture':samples>0 and max_gap<=2000,
 'recorder_completed_normally':lifecycle is None or lifecycle.get('end_reason')=='terminal_scenario',
 'no_packet_faults':not a.capture.with_suffix('.faults.jsonl').exists() or not a.capture.with_suffix('.faults.jsonl').read_text().strip(),
}
def percentile(q):
    target=int(max(0,samples-1)*q);n=0
    for value,count in sorted(frames.items()):
        n+=count
        if n>target:return value
for c in cycles.values():c['ui']=sorted(c['ui']);c['maps']=sorted(c['maps'])
result={'passed':all(checks.values()),'checks':checks,'samples':samples,'elapsed_ms':int(last.get('soak_elapsed_ms',0)),
 'minimum_free_kib':minimum_free if samples else None,'walked_yards':distance,'maximum_sample_gap_ms':max_gap,
 'frame_ms':{'p50':percentile(.5),'p95':percentile(.95),'p99':percentile(.99),'max':max(frames,default=0)},'cycles':cycles,
 'scope':'Continuous 64 MiB xemu walking, menu allocation/release and saved reconnect laps using injected controller input. Combat, crowded travel, forced network loss and hardware are separate release gates.'}
a.capture.with_suffix('.soak-check.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result,indent=2));raise SystemExit(not result['passed'])
