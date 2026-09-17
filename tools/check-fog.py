"""Summarize resident Northshire fog on/off costs without claiming travel acceptance."""
import argparse,csv,json,math
from pathlib import Path
p=argparse.ArgumentParser(description=__doc__);p.add_argument('capture',type=Path);a=p.parse_args()
rows=[r for r in csv.DictReader(a.capture.open()) if r.get('portrait_fixture')=='6']
def distribution(group,key):
    values=sorted(int(r[key]) for r in group)
    return {name:values[max(0,math.ceil(len(values)*q)-1)] if values else None for name,q in [('p50',.5),('p95',.95),('p99',.99),('max',1)]}
phases={int(r['replay_frame'])//180%6 for r in rows}
checks={
    'samples':len(rows)>=1080,'three_camera_pairs':phases==set(range(6)),
    'offline':bool(rows) and all(r['world_active']=='0' and r['login_attempts']=='0' and r['login_auth']=='0' for r in rows),
    'resident_drawn':bool(rows) and all(int(r['draws'])>0 and int(r['triangles'])>0 for r in rows),
    'no_streaming_during_measurement':bool(rows) and len({r['loads'] for r in rows})==1,
    'no_asset_or_ui_failures':bool(rows) and all(r['failures']=='0' and r['ui_failures']=='0' for r in rows),
    'headroom':bool(rows) and min(int(r['free_kib']) for r in rows)>=8192,
    'capture_complete':json.loads(a.capture.with_suffix('.lifecycle.json').read_text()).get('end_reason')=='duration_limit',
}
pairs={}
for phase in sorted(phases):
    # Exclude the boundary frame: frame_ms describes the prior frame's interval.
    group=[r for r in rows if int(r['replay_frame'])//180%6==phase and int(r['replay_frame'])%180>0]
    pairs[str(phase)]={'view':phase//2+1,'fog':bool(phase&1),'samples':len(group),
                       'frame_ms':distribution(group,'frame_ms'),'draw_ms':distribution(group,'profile_draw_ms'),
                       'draws':sorted({int(r['draws']) for r in group}),'triangles':sorted({int(r['triangles']) for r in group})}
result={'passed':all(checks.values()),'checks':checks,'samples':len(rows),'minimum_free_kib':min((int(r['free_kib']) for r in rows),default=None),
        'views':pairs,'scope':'Offline resident Northshire camera pairs in 64 MiB native-scale xemu. Inspect screenshots for visual fog/alpha/UI behavior. Does not verify streaming, live gameplay, zone lighting, indoor/underwater rules, physical input or physical Xbox performance.'}
a.capture.with_suffix('.fog-check.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result,indent=2));raise SystemExit(not result['passed'])
