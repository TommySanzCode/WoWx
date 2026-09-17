"""Check the native death, spirit-release, Spirit Healer and reconnect scenario."""
import csv,json,sys
from pathlib import Path
path=Path(sys.argv[1]);rows=list(csv.DictReader(path.open()));stages=[]
for row in rows:
    stage=int(row['scenario_stage'])
    if stage and (not stages or stages[-1]!=stage):stages.append(stage)
checks={
 'observed_all_death_scenario_states':stages==list(range(1,9)),
 'received_combat_damage':bool(rows) and max(int(r['damage_received']) for r in rows)>0,
 'reconnected':bool(rows) and rows[-1]['world_active']=='1' and rows[-1]['world_revision']=='2',
 'memory_headroom':bool(rows) and min(int(r['free_kib']) for r in rows)>=8192,
 'no_asset_failures':bool(rows) and all(r['failures']=='0' and r['region_failures']=='0' for r in rows),
 'no_packet_failures':not path.with_suffix('.faults.jsonl').exists() or not path.with_suffix('.faults.jsonl').read_text().strip(),
}
result={'passed':all(checks.values()),'stages':stages,'checks':checks,
 'scope':'State-driven controller replay in 64 MiB xemu. States 4/5/6 require observed dead/ghost/alive player fields. Tests Spirit Healer resurrection, not corpse-run recovery or physical controls.'}
path.with_suffix('.check.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result,indent=2))
if not result['passed']:raise SystemExit(1)
