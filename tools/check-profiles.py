"""Accept offline profile loading coverage; never a gameplay/hardware release gate."""
import argparse,csv,json
from pathlib import Path
from stream_telemetry import full_coverage
p=argparse.ArgumentParser(description=__doc__);p.add_argument('capture',type=Path);p.add_argument('--output',type=Path);a=p.parse_args()
def read(path):
    with path.open(newline='') as f:return [{k:float(v) for k,v in x.items()} for x in csv.DictReader(f)]
def distribution(values):
    v=sorted(values);return {k:v[int((len(v)-1)*q)] if v else None for k,q in [('p50',.5),('p95',.95),('p99',.99),('max',1)]}
main=read(a.capture);rows=read(a.capture.with_suffix('.profiles.csv'));errors=[]
def require(test,message):
    if not test:errors.append(message)
if not main or not rows:raise SystemExit('Missing native main/profile samples')
coverage=full_coverage(main,rows);require(coverage['passed'],'Incomplete/duplicate/foreign profile identities')
require(all(x['portrait_fixture']==12 and not x['world_active'] and not x['failures'] for x in main),'Wrong fixture, active account or errors')
require(min(x['free_kib'] for x in main)>=8192,'Insufficient measured free memory')
require(all(x['fixture']==12 and not x['errors'] and not x['compose_failures'] for x in rows),'Profile assertions failed')
require(max(x['completed'] for x in rows)==65535,'Not all sixteen profiles rendered')
require(max(x['cycles'] for x in rows)>=1,'No complete profile/cancellation/failure/recovery cycle')
require(max(x['cancelled'] for x in rows)>=16,'Rapid pending-profile cancellation not exercised')
require(max(x['expected_failures'] for x in rows)>=1,'Missing-profile failure not exercised')
require(all(x['failures']==x['expected_failures'] for x in rows),'Unexpected selection failures')
for stage in range(16):
    visible=[x for x in rows if x['stage']==stage and x['avatar_ready'] and x['drawn']]
    require(bool(visible),'Profile absent: '+str(stage))
    require(all(x['race']==stage//2+1 and x['sex']==stage%2 for x in visible),'Wrong visible identity: '+str(stage))
require(all(not x['avatar_ready'] and not x['drawn'] for x in rows if x['pending'] or not x['profile_ready']),'Incomplete profile was drawn')
require(all(x['race']==x['requested_race'] and x['sex']==x['requested_sex'] for x in rows if x['avatar_ready']),'Stale character identity drawn')
require(any(x['stage']==19 and x['avatar_ready'] and x['drawn'] for x in rows),'Recovery did not render')
require(all(not x['asset_failures'] for x in rows if x['stage']!=18),'Unexpected asset failure')
for field,limit in [('total_read_bytes',65536),('total_read_ops',16),('total_scan_bytes',65536),('total_allocations',3),('open_read_bytes',65536),('open_read_ops',8),('open_scan_bytes',65536),('compose_work_pixels',8192)]:
    require(all(0<=x[field]<=limit for x in rows),'Exceeded '+field)
result={'passed':not errors,'errors':errors,'main_samples':len(main),'profile_samples':len(rows),'coverage':coverage,
        'first_frame':main[0]['replay_frame'],'last_frame':main[-1]['replay_frame'],'minimum_free_kib':min(x['free_kib'] for x in main),
        'frame_ms':distribution([x['frame_ms'] for x in main[1:]]),'work_ms':distribution([x['work_ms'] for x in rows]),
        'maxima':{f:max(x[f] for x in rows) for f in ('cycles','completed','attempts','changes','cancelled','expected_failures','total_read_bytes','total_read_ops','total_scan_bytes','total_allocations','pending_bytes','avatar_bytes','animation_index_reads')},
        'scope':'Offline production profile selection, prepared appearances/outfits and renderer for sixteen race/sex profiles. Synthetic selection/cancellation/missing-profile/recovery; no server, gameplay, controller input or stock-hardware acceptance.'}
(a.output or a.capture.with_suffix('.profiles-check.json')).write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result,indent=2));raise SystemExit(bool(errors))
