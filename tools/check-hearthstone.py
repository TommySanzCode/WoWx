"""Check the complete native item-use, world-transfer and reconnect capture."""
import csv,json,sys
from pathlib import Path
path=Path(sys.argv[1]);rows=list(csv.DictReader(path.open()))
online=[r for r in rows if r['world_active']=='1']
def home(r):return abs(float(r['x'])+8949.95)<.1 and abs(float(r['y'])+132.493)<.1
checks={
 'started_at_mcbride':any(r['world_revision']=='1' and abs(float(r['x'])+8904.8)<.1 for r in online),
 'hearthstone_cast_accepted':any(r['last_spell']=='8690' and int(r['casts_accepted'])>0 for r in online),
 'transferred_without_relogin':any(r['world_revision']=='1' and home(r) and int(r['entity_count'])>0 for r in online),
 'reconnected_at_home':bool(rows and rows[-1]['world_active']=='1' and rows[-1]['world_revision']=='2' and home(rows[-1])),
 'no_asset_or_tile_failures':bool(rows) and all(r['failures']=='0' and r['region_failures']=='0' for r in rows),
 'memory_headroom':bool(rows) and min(int(r['free_kib']) for r in rows)>=8192,
 'no_packet_failures':not path.with_suffix('.faults.jsonl').exists() or not path.with_suffix('.faults.jsonl').read_text().strip(),
}
result={'passed':all(checks.values()),'checks':checks,'scope':'Native XBE in 64 MiB xemu; automated controller samples; offline fixture resets position and hearthstone cooldown only.'}
path.with_suffix('.check.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result,indent=2))
if not result['passed']:raise SystemExit(1)
