"""Validate native controller purchase/sale/reconnect evidence (WXT8)."""
import csv,json,sys
from pathlib import Path
path=Path(sys.argv[1]);rows=list(csv.DictReader(path.open()));stages=[]
for r in rows:
    stage=int(r['scenario_stage'])
    if stage and (not stages or stages[-1]!=stage):stages.append(stage)
checks={
 'controller_scenario_passed':stages==list(range(1,9)),
 'merchant_list_received':bool(rows) and max(int(r['vendor_count']) for r in rows)>0,
 'purchase_received':bool(rows) and max(int(r['bundles_bought']) for r in rows)==1,
 'sale_received':bool(rows) and max(int(r['items_sold']) for r in rows)==1,
 'reconnected_alive':bool(rows) and rows[-1]['world_active']=='1' and rows[-1]['world_revision']=='2' and int(rows[-1]['player_health'])>0,
 'headroom':bool(rows) and min(int(r['free_kib']) for r in rows)>=8192,
 'no_asset_failures':bool(rows) and all(r['failures']=='0' and r['region_failures']=='0' for r in rows),
 'no_packet_failures':not path.with_suffix('.faults.jsonl').exists() or not path.with_suffix('.faults.jsonl').read_text().strip(),
}
result={'passed':all(checks.values()),'checks':checks,'stages':stages,
 'scope':'Native state-driven controller replay. Stage 8 requires the purchased bundle minus one sold item and resulting money to persist through reconnect. Does not cover repairs, buyback, physical controls or all vendors.'}
path.with_suffix('.check.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result,indent=2))
if not result['passed']:raise SystemExit(1)
