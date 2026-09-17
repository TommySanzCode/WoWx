"""Check WXTA (version 10) equip/remove/reconnect evidence; no avatar claim."""
import csv,json,sys
from pathlib import Path
path=Path(sys.argv[1]);rows=list(csv.DictReader(path.open()));stages=[]
for row in rows:
    stage=int(row['scenario_stage'])
    if stage and (not stages or stages[-1]!=stage):stages.append(stage)
initial=next((r for r in rows if r['scenario_stage']=='2'),None)
final=rows[-1] if rows else {}
def gear(r):return tuple(int(r[f'equipment_16_{f}']) for f in ('entry','display','type'))
checks={
 'correct_scenario':bool(rows) and all(r.get('scenario_kind')=='7' for r in rows),
 'controller_scenario_passed':stages==list(range(1,9)),
 'initial_gear_resolved':bool(initial) and all(gear(initial)),
 'removed_gear_published':any(r['scenario_stage']=='4' and gear(r)==(0,0,0) for r in rows),
 'reconnected_gear_matches':bool(initial) and final.get('world_active')=='1' and final.get('world_revision')=='2' and final.get('equipment_ready_mask')=='524287' and gear(final)==gear(initial),
 'money_inventory_xp_unchanged':bool(initial) and all(final[f]==initial[f] for f in ('inventory_money','inventory_count','player_xp')),
 'position_unchanged':bool(initial) and all(abs(float(final[f])-float(initial[f]))<0.01 for f in ('x','y','z','yaw')),
 'headroom':bool(rows) and min(int(r['free_kib']) for r in rows)>=8192,
 'no_asset_failures':bool(rows) and all(r['failures']=='0' and r['region_failures']=='0' for r in rows),
 'no_packet_failures':not path.with_suffix('.faults.jsonl').exists() or not path.with_suffix('.faults.jsonl').read_text().strip(),
}
result={'passed':all(checks.values()),'checks':checks,'stages':stages,
 'scope':'Native state-driven controller replay through the inventory UI. Resolves/removes/restores equipped item metadata and reconnects. Does not validate avatar drawing or physical controller input.'}
path.with_suffix('.check.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result,indent=2))
if not result['passed']:raise SystemExit(1)
