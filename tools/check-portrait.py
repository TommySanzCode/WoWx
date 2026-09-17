"""Check a dedicated offline NV2A fixture, separately from live HUD acceptance."""
import argparse,csv,json,math
from pathlib import Path
p=argparse.ArgumentParser(description=__doc__);p.add_argument('capture',type=Path);a=p.parse_args()
rows=list(csv.DictReader(a.capture.open()));shown=[r for r in rows if r.get('portrait_fixture')=='1']
players={int(r['portrait_player_key']) for r in shown if int(r['portrait_player_draws'])>0}
creatures={int(r['portrait_target_key']) for r in shown if int(r['portrait_target_draws'])>0}
stable=[r for i,r in enumerate(shown) if i and r['portrait_player_key']==shown[i-1]['portrait_player_key'] and r['portrait_target_key']==shown[i-1]['portrait_target_key'] and int(r['portrait_player_draws']) and int(r['portrait_target_draws'])]
def timing(data,field):
    values=sorted(int(r[field]) for r in data)
    return {key:values[min(len(values)-1,math.ceil(len(values)*q)-1)] if values else None for key,q in [('p50',.5),('p95',.95),('p99',.99),('max',1)]}
checks={'samples':len(shown)>=600,'offline_only':bool(shown) and all(r['world_active']=='0' and r['login_auth']=='0' and r['login_attempts']=='0' for r in shown),
        'all_player_models':players=={0x80000000|(race<<1)|sex for race in range(1,9) for sex in range(2)},
        'all_prepared_creatures':len(creatures)==92,
        'no_failures':bool(shown) and all(r['failures']=='0' and r['portrait_failures']=='0' and r['ui_failures']=='0' for r in shown),
        'mask_bounds':bool(shown) and all(int(r['portrait_spans']) in (0,37,74) for r in shown),
        'headroom':bool(shown) and min(int(r['free_kib']) for r in shown)>=8192,
        'capture_complete':json.loads(a.capture.with_suffix('.lifecycle.json').read_text()).get('end_reason')=='duration_limit'}
result={'passed':all(checks.values()),'checks':checks,'samples':len(shown),'players':len(players),'creatures':len(creatures),
        'minimum_free_kib':min((int(r['free_kib']) for r in shown),default=None),'frame_ms':timing(shown,'frame_ms'),
        'stable_frame_ms':timing(stable,'frame_ms'),'submit_ms':timing(shown,'portrait_submit_ms'),
        'scope':'64 MiB xemu offline fixture with synthetic units; excludes terrain/full-world memory and physical hardware or controller proof. Screenshots required for visual acceptance.'}
a.capture.with_suffix('.portrait-check.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result,indent=2));raise SystemExit(not result['passed'])
