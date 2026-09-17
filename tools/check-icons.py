"""Check isolated native action-icon submission and cache bounds; no combat claim."""
import argparse,csv,json,math
from pathlib import Path
p=argparse.ArgumentParser(description=__doc__);p.add_argument('capture',type=Path);a=p.parse_args()
rows=list(csv.DictReader(a.capture.open()));shown=[r for r in rows if r.get('portrait_fixture')=='2']
ready=[r for r in shown if r.get('icons_pending')=='0' and int(r.get('icons_loads',0))>0]
layers={int(r['actionbar_layer']) for r in ready}
expected=[6603,78,2457,6673,100,0x80000000|6948,0,0,133,116,686,172,403,331,585,2061,1752,2098,53,6673,1130,2973,5176,5185]
checks={'samples':len(ready)>=300,'three_layers':layers=={1,2,3},
 'offline_only':bool(shown) and all(r['world_active']=='0' and r['login_auth']=='0' and r['login_attempts']=='0' for r in shown),
 'correct_slots_and_icons':bool(ready) and all(int(r['icons_drawn'])==(6 if r['actionbar_layer']=='1' else 8) and
     all(int(r[f'actionbar_binding_{i}'])==expected[(int(r['actionbar_layer'])-1)*8+i] and int(r[f'actionbar_slot_{i}'])==(int(r['actionbar_layer'])-1)*8+i for i in range(8)) for r in ready),
 'bounded_allocations':bool(shown) and all(int(r['icons_bytes'])==677840 and int(r['icons_pending'])<=24 and 0<int(r['ui_batches'])<=128 and int(r['ui_quads'])<=2048 for r in shown),
 'bounded_uploads':bool(shown) and all(int(right['icons_loads'])-int(left['icons_loads'])<=int(right['replay_frame'])-int(left['replay_frame']) for left,right in zip(shown,shown[1:])),
 'cache_stable':bool(ready) and len({r['icons_loads'] for r in ready})==1,
 'no_failures':bool(shown) and all(r['failures']=='0' and r['icons_failures']=='0' and r['ui_failures']=='0' for r in shown),
 'headroom':bool(shown) and min(int(r['free_kib']) for r in shown)>=8192,
 'capture_complete':json.loads(a.capture.with_suffix('.lifecycle.json').read_text()).get('end_reason')=='duration_limit'}
def timing(field):
 values=sorted(int(r[field]) for r in ready)
 return {name:values[max(0,math.ceil(len(values)*q)-1)] if values else None for name,q in [('p50',.5),('p95',.95),('p99',.99),('max',1)]}
result={'passed':all(checks.values()),'checks':checks,'samples':len(shown),'ready_samples':len(ready),'layers':sorted(layers),
 'minimum_free_kib':min((int(r['free_kib']) for r in shown),default=None),'frame_ms':timing('frame_ms'),'upload_ms':timing('icons_upload_ms'),
 'scope':'64 MiB xemu, offline synthetic bindings and injected UI state. Screenshots verify graphics separately; no live-world, class combat, physical-controller or hardware acceptance.'}
a.capture.with_suffix('.icons-check.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result,indent=2));raise SystemExit(not result['passed'])
