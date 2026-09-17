"""Check the uninterrupted native starter-quest controller replay (WXCA)."""
import csv,json,math,sys
from pathlib import Path
p=Path(sys.argv[1]);rows=list(csv.DictReader(p.open()));last=rows[-1] if rows else {};stages=[]
for r in rows:
    stage=int(r['scenario_stage'])
    if stage and (not stages or stage!=stages[-1]):stages.append(stage)
active=[r for r in rows if r['world_active']=='1' and r['player_guid_low']=='2']
before=next((r for r in active if r['scenario_stage']=='15'),None)
start=next((r for r in active if r['scenario_stage']=='4' and r['player_level']=='1'),None)
walked=0.;ordinary=True
for a,b in zip(active,active[1:]):
    if a['world_revision']!=b['world_revision']:continue
    distance=math.hypot(float(b['x'])-float(a['x']),float(b['y'])-float(a['y']))
    elapsed=(int(b['time_ms'])-int(a['time_ms']))/1000
    walked+=distance
    if elapsed<0 or distance>max(0,elapsed)*8+.5:ordinary=False
checks={
 'correct_scenario':bool(rows) and all(r['scenario_kind']=='10' for r in rows),
 'complete_fresh_route':stages in ([1,2,3,4,10,11,12,13,14,15,6,7,8],[2,3,4,10,11,12,13,14,15,6,7,8]),
 'fresh_character':bool(start) and start['player_xp']=='0' and start['player_level']=='1' and start['active_quests']=='0',
 'quest_accepted_and_completed':any(r['quest_id']=='783' and r['quest_screen']=='2' for r in active) and any(r['completed_quest']=='783' and r['player_xp']=='40' for r in active),
 'followup_accepted':any(r['quest_id']=='7' and r['quest_screen']=='2' for r in active) and bool(before) and before['active_quests']=='1',
 'walked_between_npcs':walked>45 and ordinary,
 'run_animation':any(r['avatar_clip']=='5' and int(r['avatar_drawn'])>0 for r in active),
 'saved_reconnect':bool(before) and last.get('world_active')=='1' and last.get('player_guid_low')=='2' and int(last['world_revision'])>=3 and all(last[k]==before[k] for k in ('player_xp','player_level','inventory_count','inventory_money','active_quests')),
 'position_restored':bool(before) and all(abs(float(last[k])-float(before[k]))<.01 for k in ('x','y','z')),
 'headroom':bool(rows) and min(int(r['free_kib']) for r in rows)>=8192,
 'no_load_or_lobby_failures':bool(rows) and all(r['failures']=='0' and r['region_failures']=='0' and r['lobby_phase']!='4' for r in rows),
 'no_packet_faults':not p.with_suffix('.faults.jsonl').exists() or not p.with_suffix('.faults.jsonl').read_text().strip(),
}
result={'passed':all(checks.values()),'checks':checks,'stages':stages,'walked_yards':walked,
 'capture_start':'Before character selection; initial menu-opening stage may precede recording.',
 'scope':'Native injected pad samples, actual walking collision, starter quest 783, acceptance of followup 7 and reconnect. Does not cover followup combat/loot, death, physical controls or Xbox hardware.'}
p.with_suffix('.starter-check.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result,indent=2));raise SystemExit(0 if result['passed'] else 1)
