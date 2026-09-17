"""Validate bounded UI submissions during a native character-screen replay."""
import argparse,csv,json,math
from pathlib import Path
p=argparse.ArgumentParser(description=__doc__);p.add_argument('capture',type=Path);p.add_argument('--normal',action='store_true');p.add_argument('--baseline',type=Path);a=p.parse_args()
rows=list(csv.DictReader(a.capture.open()));shown=[r for r in rows if r.get('lobby_phase')=='2']
checks={'atlas_loaded':bool(rows) and all(r.get('ui_ready')=='1' for r in rows),
 'fixed_allocation':bool(rows) and all(r.get('ui_bytes')=='1376256' for r in rows),
 'bounded_submission':bool(rows) and all(0<=int(r.get('ui_quads',-1))<=2048 for r in rows),
 'no_ui_failures':bool(rows) and all(r.get('ui_failures')=='0' for r in rows),
 'memory_headroom':bool(rows) and min(int(r['free_kib']) for r in rows)>=8192}
if not a.normal:
 checks.update({'screens_visited':{0,1,2}<={int(r['character_screen']) for r in shown},
  'rendered_screens':bool(shown) and all(int(r['ui_quads'])>40 for r in shown),
  'returned_to_world':bool(rows) and rows[-1]['world_active']=='1' and rows[-1]['lobby_phase']=='0'})
if a.baseline:
 with a.baseline.open(newline='') as source:before=list(csv.DictReader(source))[-1]
 last=rows[-1] if rows else {}
 fields=['player_guid_low','player_guid_high','player_level','player_xp','inventory_money','inventory_count','active_quests']+[f'equipment_{i}_{k}' for i in range(19) for k in ('entry','display','type')]
 checks.update({'progress_preserved':bool(rows) and all(last[k]==before[k] for k in fields),
  'position_preserved':bool(rows) and all(abs(float(last[k])-float(before[k]))<.01 for k in ('x','y','z')) and abs(math.remainder(float(last['yaw'])-float(before['yaw']),2*math.pi))<.01,
  'input_released':bool(rows) and last['replay_active']=='0' and last['menu']=='0'})
result={'passed':all(checks.values()),'checks':checks,'samples':len(rows),'lobby_samples':len(shown),
 'maximum_quads':max((int(r.get('ui_quads',0)) for r in rows),default=0),
 'scope':'Native bounded UI allocation/submission counters. Actual artwork, legibility and layout require screenshots; replay does not verify physical controllers.'}
a.capture.with_suffix('.ui-check.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result,indent=2));raise SystemExit(not result['passed'])
