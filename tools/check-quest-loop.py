"""Verify an uninterrupted fresh quest/combat/loot/reward/reconnect run (WXCE)."""
import argparse,csv,json,math
from pathlib import Path
parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('capture',type=Path);parser.add_argument('--guid',type=int,default=3);args=parser.parse_args()
p=args.capture;rows=list(csv.DictReader(p.open()));last=rows[-1] if rows else {};stages=[]
for r in rows:
    stage=int(r['scenario_stage'])
    if stage and (not stages or stage!=stages[-1]):stages.append(stage)
active=[r for r in rows if r['world_active']=='1' and int(r['player_guid_low'])==args.guid]
start=next((r for r in active if r['scenario_stage']=='4' and r['player_level']=='1'),None)
before=next((r for r in reversed(active) if r['scenario_stage']=='31'),None)
journal=[r for r in active if r['journal_ready']=='1' and r['journal_id']=='7']
walked=0.;ordinary=True
for a,b in zip(active,active[1:]):
    if a['world_revision']!=b['world_revision']:continue
    distance=math.hypot(float(b['x'])-float(a['x']),float(b['y'])-float(a['y']));elapsed=(int(b['time_ms'])-int(a['time_ms']))/1000
    walked+=distance
    if elapsed<0 or distance>max(0,elapsed)*8+.5:ordinary=False
checks={
 'correct_scenario':bool(rows) and all(r['scenario_kind']=='14' for r in rows),
 'passed':last.get('scenario_stage')=='8' and 9 not in stages,
 'complete_route':all(s in stages for s in (3,4,10,11,12,13,14,15,40,41,42,43,20,21,22,23,24,25,26,27,28,29,30,31,6,7,8)),
 'fresh_character':bool(start) and start['player_xp']=='0' and start['player_level']=='1' and start['active_quests']=='0',
 'starter_reward':any(r['completed_quest']=='783' and r['player_xp']=='40' for r in active),
 'followup_accepted':any(r['quest_id']=='7' and r['quest_screen']=='2' for r in active),
 'journal_read':any(r['journal_detail']=='1' and r['journal_story']=='0' and r['journal_progress']=='0' for r in journal) and any(r['journal_detail']=='1' and r['journal_story']=='1' for r in journal),
 'ten_kills_earned':max((int(r['kills']) for r in active),default=0)>=10,
 'combat_spell_loot':any(int(r['damage_dealt'])>0 and int(r['looted_items'])>0 and int(r['casts_accepted'])>0 and int(r['spell_hits'])>0 for r in active),
 'quest_reward':any(r['completed_quest']=='7' and r['active_quests']=='0' for r in active),
 'ordinary_walking':walked>350 and ordinary,
 'avatar_rendered':any(r['player_identity']=='65793' and r['avatar_ready']=='1' and r['avatar_missing']=='0' and int(r['avatar_drawn'])>0 and r['avatar_clip']=='5' for r in active),
 'saved_reconnect':bool(before) and last.get('world_active')=='1' and last.get('player_guid_low')==str(args.guid) and int(last['world_revision'])>=3 and all(last[k]==before[k] for k in ('player_xp','player_level','inventory_count','inventory_money','active_quests')),
 'position_restored':bool(before) and all(abs(float(last[k])-float(before[k]))<.01 for k in ('x','y','z')),
 'headroom':bool(rows) and min(int(r['free_kib']) for r in rows)>=8192,
 'no_failures':bool(rows) and all(all(r[k]=='0' for k in ('failures','region_failures','book_failures','avatar_select_failures')) and r['lobby_phase']!='4' for r in rows),
 'no_packet_faults':not p.with_suffix('.faults.jsonl').exists() or not p.with_suffix('.faults.jsonl').read_text().strip(),
}
result={'passed':all(checks.values()),'checks':checks,'stages':stages,'walked_yards':walked,
 'scope':'Continuous native injected controller samples from fresh character through quests 783 and 7, combat, spell use, loot, walking return, reward and reconnect. No teleport/SQL state changes. Death, physical controller and hardware are separate gates.'}
p.with_suffix('.quest-loop-check.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result,indent=2));raise SystemExit(not result['passed'])
