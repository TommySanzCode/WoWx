"""Validate a WXC8 native camera sweep without moving the saved character."""
import csv,json,math,sys
from pathlib import Path
path=Path(sys.argv[1]);rows=list(csv.DictReader(path.open()))
active=[r for r in rows if r['world_active']=='1' and int(r['scenario_stage'])>0]
last=active[-1] if active else {};first=active[0] if active else {}
turning=[r for r in active if r['scenario_stage']=='2'];stages={int(r['scenario_stage']) for r in active}
checks={
 'correct_scenario':bool(rows) and all(r['scenario_kind']=='8' for r in rows),
 'completed_sweep':{1,2,3,4,5,8}<=stages and last.get('scenario_stage')=='8' and 9 not in stages,
 'heading_sweep':bool(turning) and max(float(r['yaw']) for r in turning)-min(float(r['yaw']) for r in turning)>6,
 'pitch_sweep':bool(active) and min(float(r['pitch']) for r in active)<-.648 and max(float(r['pitch']) for r in active)>.498,
 'restored_view':bool(active) and abs(math.atan2(math.sin(float(last['yaw'])-float(first['yaw'])),math.cos(float(last['yaw'])-float(first['yaw']))))<.002 and abs(float(last['pitch'])-float(first['pitch']))<.002,
 'no_translation':bool(active) and all(abs(float(r[f])-float(first[f]))<.01 for r in active for f in ('x','y','z')),
 'camera_collision_observed':bool(turning) and min(int(r['camera_mm']) for r in turning)<5500 and max(int(r['camera_mm']) for r in turning)==6000,
 'avatar_recovered':last.get('avatar_ready')=='1' and int(last.get('avatar_drawn',0))>0 and last.get('avatar_missing')=='0',
 'headroom':bool(rows) and min(int(r['free_kib']) for r in rows)>=8192,
 'no_asset_failures':bool(rows) and all(r['failures']=='0' and r['region_failures']=='0' for r in rows),
}
def timing(stage):
 times=sorted(int(r['frame_ms']) for r in active if int(r['scenario_stage']) in stage)
 return {'frames':len(times),'p50':times[len(times)//2] if times else None,'p95':times[int((len(times)-1)*.95)] if times else None,'max':max(times) if times else None}
result={'passed':all(checks.values()),'checks':checks,'moving_view_ms':timing({2,3,4,5}),'restored_view_ms':timing({8}),
 'scope':'Client-injected right-stick camera samples; original position and view restored. Does not validate physical controller transport or all world geometry.'}
path.with_suffix('.camera-check.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result,indent=2));raise SystemExit(0 if result['passed'] else 1)
