"""Validate native phase counters and summarize the measured work distribution."""
import argparse,csv,json
from pathlib import Path
p=argparse.ArgumentParser(description=__doc__);p.add_argument('capture',type=Path);p.add_argument('--walking',action='store_true');a=p.parse_args()
rows=list(csv.DictReader(a.capture.open()))
phases=['profile_'+x+'_ms' for x in ('update','move','stream','camera','actors','draw','ui','present')]
available=bool(rows) and all(k in rows[0] for k in phases+['profile_work_ms','profile_pace_ms'])
selected=[r for r in rows if available and (not a.walking or int(r['scenario_stage']) in (20,21,28,29))]
checks={
 'phase_fields_present':available,
 'measured_frames':bool(selected) and any(int(r['profile_work_ms'])>0 for r in selected),
 'contiguous_work_intervals':bool(selected) and all(sum(int(r[k]) for k in phases)==int(r['profile_work_ms']) for r in selected),
 'nonnegative_intervals':bool(selected) and all(int(r[k])>=0 for r in selected for k in phases+['profile_work_ms','profile_pace_ms']),
}
summary={}
for key in phases+['profile_work_ms','profile_pace_ms']:
 values=sorted(int(r[key]) for r in selected)
 if values:summary[key]={'mean':sum(values)/len(values),'p50':values[(len(values)-1)//2],'p95':values[int((len(values)-1)*.95)],'max':max(values)}
result={'passed':all(checks.values()),'checks':checks,'samples':len(selected),'walking_only':a.walking,'milliseconds':summary,
 'scope':'Current-frame guest wall time. Phases include waits within them, including GPU work. The frame-start interval belongs to the preceding work. No CPU/hardware performance claim.'}
a.capture.with_suffix('.profile-check.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result,indent=2));raise SystemExit(not result['passed'])
