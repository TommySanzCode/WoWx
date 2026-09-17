"""Check the read-only walking/reconnect controller scenario (WXCF)."""
import argparse,csv,json,math
from pathlib import Path
p=argparse.ArgumentParser(description=__doc__);p.add_argument('capture',type=Path);p.add_argument('--continuous-avatar',action='store_true');a=p.parse_args()
rows=list(csv.DictReader(a.capture.open()));last=rows[-1] if rows else {};stages=[]
for r in rows:
    stage=int(r['scenario_stage'])
    if stage and (not stages or stages[-1]!=stage):stages.append(stage)
active=[r for r in rows if r['world_active']=='1' and r['player_guid_low']=='2']
first=next((r for r in active if r['scenario_stage']=='4' and int(r['player_level'])),None)
before=next((r for r in reversed(active) if r['scenario_stage']=='33'),None)
distance=0.;ordinary=True
for x,y in zip(active,active[1:]):
    if x['world_revision']!=y['world_revision']:continue
    d=math.hypot(float(y['x'])-float(x['x']),float(y['y'])-float(x['y']));dt=(int(y['time_ms'])-int(x['time_ms']))/1000
    distance+=d
    if dt<0 or d>max(0,dt)*8+.5:ordinary=False
checks={
 'correct_scenario':bool(rows) and all(r['scenario_kind']=='15' for r in rows),
 'complete_route':stages in ([1,2,3,4,20,21,28,29,32,33,6,7,8],[2,3,4,20,21,28,29,32,33,6,7,8]),
 'ordinary_walking':distance>350 and ordinary,
 'saved_progress':bool(first) and all(last[k]==first[k] for k in ('player_level','player_xp','inventory_count','inventory_money','active_quests')),
 'returned_to_start':bool(first) and all(abs(float(last[k])-float(first[k]))<.08 for k in ('x','y','z')),
 'restored_view':bool(first) and abs(math.remainder(float(last['yaw'])-float(first['yaw']),2*math.pi))<.005,
 'saved_reconnect':bool(before) and last.get('world_active')=='1' and last.get('player_guid_low')=='2' and int(last['world_revision'])>=3 and all(abs(float(last[k])-float(before[k]))<.01 for k in ('x','y','z')),
 'no_combat_or_trade':bool(rows) and all(all(r[k]=='0' for k in ('kills','casts_accepted','casts_rejected','looted_items','bundles_bought','items_sold')) for r in rows),
 'headroom':bool(rows) and min(int(r['free_kib']) for r in rows)>=8192,
 'no_failures':bool(rows) and all(all(r[k]=='0' for k in ('failures','region_failures','book_failures','avatar_select_failures')) and r['lobby_phase']!='4' for r in rows),
 'no_packet_faults':not a.capture.with_suffix('.faults.jsonl').exists() or not a.capture.with_suffix('.faults.jsonl').read_text().strip(),
}
if a.continuous_avatar:
    walking=[r for r in active if int(r['scenario_stage']) in (20,21,28,29)]
    checks['continuous_avatar']=bool(walking) and all(r['avatar_matched']=='1' and r['avatar_ready']=='1' and r['avatar_missing']=='0' for r in walking)
timing={}
for stage in (20,21,28,29):
    values=sorted(int(r['frame_ms']) for r in rows if int(r['scenario_stage'])==stage and int(r['frame_ms'])>0)
    if values:timing[str(stage)]={'samples':len(values),'p50':values[(len(values)-1)//2],'p95':values[int((len(values)-1)*.95)],'max':max(values),'mean':sum(values)/len(values)}
result={'passed':all(checks.values()),'checks':checks,'stages':stages,'walked_yards':distance,'walking_stage_ms':timing,
 'scope':'Injected controller walking through the Abbey and to/from camp. Reconnect and progress retention. xemu guest timings are not hardware benchmarks; moving NPCs and host work are uncontrolled.'}
a.capture.with_suffix('.walk-check.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result,indent=2));raise SystemExit(not result['passed'])
