"""Check a native quest-7 run; explicitly distinguish continuation from a fresh loop."""
import argparse,csv,json,math
from pathlib import Path
parser=argparse.ArgumentParser();parser.add_argument('capture',type=Path);parser.add_argument('--return-only',action='store_true');parser.add_argument('--guid',type=int,default=2);args=parser.parse_args()
p=args.capture;rows=list(csv.DictReader(p.open()));last=rows[-1] if rows else {};stages=[]
for r in rows:
    stage=int(r['scenario_stage'])
    if stage and (not stages or stage!=stages[-1]):stages.append(stage)
active=[r for r in rows if r['world_active']=='1' and r['player_guid_low']==str(args.guid)]
# Quest-complete and player/inventory updates are separate server packets.
# Compare the settled state immediately before logout, after the driver's wait.
before=next((r for r in reversed(active) if r['scenario_stage']=='31'),None)
journal=[r for r in active if r.get('journal_ready')=='1' and r.get('journal_id')=='7']
starting_kills=int(journal[0]['journal_progress']) if journal else -1
walked=0.;ordinary=True
for a,b in zip(active,active[1:]):
    if a['world_revision']!=b['world_revision']:continue
    distance=math.hypot(float(b['x'])-float(a['x']),float(b['y'])-float(a['y']));elapsed=(int(b['time_ms'])-int(a['time_ms']))/1000
    walked+=distance
    if elapsed<0 or distance>max(0,elapsed)*8+.5:ordinary=False
checks={
 'correct_scenario':bool(rows) and all(r['scenario_kind']=='11' for r in rows),
 'driver_passed':last.get('scenario_stage')=='8' and 9 not in stages,
 'journal_objectives':any(r.get('journal_detail')=='1' and r.get('journal_story')=='0' for r in journal),
 'journal_description':any(r.get('journal_detail')=='1' and r.get('journal_story')=='1' for r in journal),
 'remaining_kills_earned':0<=starting_kills<10 and max((int(r['kills']) for r in active),default=0)>=10-starting_kills,
 'combat_and_loot':any(int(r['damage_dealt'])>0 and int(r['looted_items'])>0 and int(r['casts_accepted'])>0 for r in active),
 'quest_reward':any(r['completed_quest']=='7' and r['active_quests']=='0' for r in active),
 'walked_back':all(s in stages for s in (27,28,29,30,31,6,7,8)) and walked>140 and ordinary,
 'saved_reconnect':bool(before) and last.get('world_active')=='1' and last.get('player_guid_low')==str(args.guid) and int(last['world_revision'])>=3 and all(last[k]==before[k] for k in ('player_xp','player_level','inventory_count','inventory_money','active_quests')),
 'position_restored':bool(before) and all(abs(float(last[k])-float(before[k]))<.01 for k in ('x','y','z')),
 'headroom':bool(rows) and min(int(r['free_kib']) for r in rows)>=8192,
 'no_failures':bool(rows) and all(r['failures']=='0' and r['region_failures']=='0' and r['lobby_phase']!='4' for r in rows),
 'no_packet_faults':not p.with_suffix('.faults.jsonl').exists() or not p.with_suffix('.faults.jsonl').read_text().strip(),
}
if args.return_only:
    del checks['remaining_kills_earned'];del checks['combat_and_loot'];checks['objectives_already_complete']=starting_kills==10
result={'passed':all(checks.values()),'checks':checks,'stages':stages,'walked_yards':walked,'starting_kills':starting_kills,'return_only':args.return_only,
 'fresh_quest_loop':all(checks.values()) and starting_kills==0 and 20 in stages and 21 in stages,
 'scope':'Native injected controller samples, quest journal, actual combat/loot and walking/reward/reconnect. Starting kills above zero are a continuation of earlier progress, not a fresh continuous quest loop. Physical pad, Xbox hardware and death recovery are separate.'}
p.with_suffix('.camp-check.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result,indent=2));raise SystemExit(0 if result['passed'] else 1)
