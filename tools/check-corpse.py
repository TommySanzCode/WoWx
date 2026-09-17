"""Check an uninterrupted native corpse-run replay, including ghost reconnect."""
import argparse,csv,json,math
from pathlib import Path
p=argparse.ArgumentParser(description=__doc__);p.add_argument('capture',type=Path);a=p.parse_args()
rows=list(csv.DictReader(a.capture.open()));last=rows[-1] if rows else {}
stages=[]
for r in rows:
    stage=int(r['scenario_stage'])
    if stage and (not stages or stages[-1]!=stage):stages.append(stage)
active=[r for r in rows if r['world_active']=='1']
def phase(n):return [r for r in active if int(r['scenario_stage'])==n]
initial=phase(13)[-1] if phase(13) else {}
ghost=phase(19);walk=phase(20);mapped=phase(16)
steps=[math.dist([float(x[k]) for k in ('x','y','z')],[float(y[k]) for k in ('x','y','z')]) for x,y in zip(walk,walk[1:])]
reclaims=[r for prev,r in zip(rows,rows[1:]) if int(r['corpse_reclaims'])>int(prev['corpse_reclaims'])]
checks={
 'correct_scenario':bool(rows) and all(r['scenario_kind']=='16' for r in rows),
 'complete_controller_route':stages==[1,2,3,4,10,11,12,13,14,15,16,17,18,19,20,21,22,6,7,8],
 'death_and_release':any(r['death_state']=='1' for r in phase(14)) and int(last.get('spirit_releases',0))>=1,
 'server_graveyard':any(r['death_state']=='2' and abs(float(r['x'])+8935.325)<2 and abs(float(r['y'])+188.646)<2 for r in mapped),
 'corpse_timer_received':any(int(r['corpse_wait_ms'])>0 for r in rows),
 'corpse_map_marker':len(mapped)>200 and all(r['map_open']=='1' and r['map_ready']=='1' and r['map_body']=='1' for r in mapped[2:-2]),
 'ghost_reconnect':bool(ghost) and any(r['world_active']=='0' for r in rows if r['scenario_stage']=='18') and all(r['death_state']=='2' for r in ghost) and int(last.get('corpse_queries',0))>=2,
 'walked_back':len(walk)>300 and sum(steps)>100 and max(steps,default=999)<1.1,
 'reclaim_distance_and_timer':bool(reclaims) and all(r['death_state']=='2' and r['corpse_ready']=='1' and int(r['corpse_distance_mm'])<=39000 and r['corpse_wait_ms']=='0' and (int(r['corpse_low']) or int(r['corpse_high'])) for r in reclaims),
 'corpse_resurrection':int(last.get('resurrections',0))==1 and all(r['healer_requests']=='0' for r in rows),
 'alive_after_reconnect':last.get('scenario_stage')=='8' and last.get('world_active')=='1' and last.get('death_state')=='0' and int(last.get('player_health',0))>0 and bool(ghost) and int(last['world_revision'])>int(ghost[-1]['world_revision']),
 'progress_preserved':bool(initial) and all(last.get(k)==initial[k] for k in ('player_guid_low','player_guid_high','player_level','player_xp','inventory_money','inventory_count')),
 'no_kills_or_loot':bool(rows) and all(r['kills']=='0' and r['looted_items']=='0' and r['looted_money']=='0' for r in rows),
 'headroom':bool(rows) and min(int(r['free_kib']) for r in rows)>=8192,
 'no_failures':bool(rows) and all(all(r[k]=='0' for k in ('failures','region_failures','book_failures','avatar_select_failures','map_failures','text_failures')) and r['lobby_phase']!='4' for r in rows),
 'no_packet_faults':not a.capture.with_suffix('.faults.jsonl').exists() or not a.capture.with_suffix('.faults.jsonl').read_text().strip(),
}
result={'passed':all(checks.values()),'checks':checks,'stages':stages,'ghost_walk_yards':sum(steps),
 'scope':'Injected native controller replay. Real server death, spirit release, corpse map, ghost logout/reconnect, collision-constrained return, timed reclaim and alive reconnect. Physical controls and hardware remain separate.'}
a.capture.with_suffix('.corpse-check.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result,indent=2));raise SystemExit(not result['passed'])
