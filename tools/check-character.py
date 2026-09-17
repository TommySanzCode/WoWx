"""Verify WXC9 native controller character creation/switch/restore evidence."""
import argparse,csv,json
from pathlib import Path
parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('capture',type=Path);parser.add_argument('--avatar-switch',action='store_true');args=parser.parse_args()
path=args.capture;rows=list(csv.DictReader(path.open()));stages=[]
for r in rows:
    stage=int(r['scenario_stage'])
    if stage and (not stages or stages[-1]!=stage):stages.append(stage)
active=[r for r in rows if r['world_active']=='1']
initial=next((r for r in active if r['scenario_stage']=='2'),None);last=rows[-1] if rows else {}
def guid(r):return int(r['player_guid_low'])|(int(r['player_guid_high'])<<32)
new=[r for r in active if initial and guid(r)!=guid(initial) and r['scenario_stage']=='10']
fresh=4 in stages
checks={
 'correct_scenario':bool(rows) and all(r['scenario_kind']==('13' if args.avatar_switch else '9') for r in rows),
 'passed_route':stages==([1,2,3,4,5,6,7,10,11,12,13,8] if fresh else [1,2,3,7,10,11,12,13,8]),
 'controller_picker_used':any(r['lobby_phase']=='2' and int(r['lobby_count'])>=2 for r in rows),
 'controller_creation_used':not fresh or {0,1,2,3}<={int(r['character_screen']) for r in rows if r['lobby_phase']=='2'},
 'creation_accepted':not fresh or any(r['character_result']=='46' and r['character_result_revision']=='1' for r in rows),
 'character_entered':bool(new) and (not fresh or any(r['player_level']=='1' and r['player_xp']=='0' and r['inventory_money']=='0' for r in new)),
 'new_character_avatar':any(r['avatar_ready']=='1' and int(r['avatar_drawn'])>0 and r['avatar_missing']=='0' for r in new),
 'original_character_restored':bool(initial) and last.get('world_active')=='1' and guid(last)==guid(initial) and int(last['world_revision'])>=3,
 'progress_preserved':bool(initial) and all(last.get(f)==initial[f] for f in ('player_level','player_xp','inventory_money','inventory_count','equipment_16_entry')),
 'position_preserved':bool(initial) and all(abs(float(last[f])-float(initial[f]))<.01 for f in ('x','y','z','yaw')),
 'headroom':bool(rows) and min(int(r['free_kib']) for r in rows)>=8192,
 'no_asset_or_session_failures':bool(rows) and all(r['failures']=='0' and r['region_failures']=='0' and r['lobby_phase']!='4' for r in rows),
 'no_packet_failures':not path.with_suffix('.faults.jsonl').exists() or not path.with_suffix('.faults.jsonl').read_text().strip(),
}
if args.avatar_switch:
 checks.update({
  'female_profile_drawn':any(r['player_identity']=='65793' and r['avatar_matched']=='1' and r['avatar_ready']=='1' and int(r['avatar_drawn'])>0 and r['avatar_missing']=='0' and r['avatar_profile_changes']=='2' for r in new),
  'original_profile_drawn':last.get('player_identity')=='257' and last.get('avatar_matched')=='1' and last.get('avatar_ready')=='1' and int(last.get('avatar_drawn',0))>0 and last.get('avatar_missing')=='0',
  'exactly_three_profile_loads':last.get('avatar_select_attempts')=='3' and last.get('avatar_profile_changes')=='3',
  'no_selection_failure':all(r['avatar_select_failures']=='0' for r in rows),
 })
result={'passed':all(checks.values()),'created_new_character':fresh,'checks':checks,'stages':stages,
 'original_guid':guid(initial) if initial else None,'other_guids':sorted({guid(r) for r in new}),
 'scope':'Client-injected controller input through the character picker, on-screen keyboard and creation confirmation. Server creation, entry and original progress restoration are checked; physical controls remain separate.'}
path.with_suffix('.character-check.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result,indent=2))
raise SystemExit(0 if result['passed'] else 1)
