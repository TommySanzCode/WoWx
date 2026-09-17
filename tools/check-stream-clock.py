"""Check cooperative streaming slices; report guest timing separately from hardware."""
import argparse,csv,json
from pathlib import Path
from stream_telemetry import full_coverage

def distribution(values):
    v=sorted(values)
    return {name:v[int((len(v)-1)*q)] if v else None for name,q in [('p50',.5),('p95',.95),('p99',.99),('max',1)]}

def read(path):
    with path.open(newline='') as f:return [{k:float(v) for k,v in r.items()} for r in csv.DictReader(f)]

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('capture',type=Path)
    p.add_argument('--output',type=Path,required=True);a=p.parse_args()
    main=read(a.capture);rows=read(a.capture.with_suffix('.stream.csv'));errors=[]
    coverage=full_coverage(main,rows)
    if not coverage['passed']:errors.append('Incomplete, duplicate or foreign companion identities')
    if not rows or not main:errors.append('No native samples')
    lanes=('index','world','npc','player')
    for r in rows:
        if r.get('clock_enabled')!=1 or r.get('budget_enabled')!=1:
            errors.append('Clock or shared byte budget disabled');break
        limits=[r[w+'_clock_limit_ms'] for w in lanes]
        observed=[r[w+'_clock_observed_ms'] for w in lanes]
        mask=int(r['clock_yield_mask'])
        if sum(limits) not in (0,12) or any(x<0 or x>12 for x in limits) or mask<0 or mask>15:
            errors.append('Invalid clock shares');break
        if any((not limits[i] and observed[i]) or bool(mask&(1<<i))!=bool(limits[i] and observed[i]>=limits[i]) for i in range(4)):
            errors.append('Invalid yield/elapsed accounting');break
    if rows and not any(r.get('clock_yield_mask',0)>0 for r in rows):errors.append('No timed yield exercised')
    if main and min(r['free_kib'] for r in main)<8192:errors.append('Insufficient measured headroom')
    result={'passed':not errors,'errors':errors,'coverage':coverage,
        'minimum_free_kib':min((r['free_kib'] for r in main),default=None),
        'frame_ms':distribution([r['frame_ms'] for r in main[1:]]),
        'lanes':{w:{'yield_frames':sum(bool(int(r.get('clock_yield_mask',0))&(1<<i)) for r in rows),
                    'observed_ms':distribution([r.get(w+'_clock_observed_ms',-1) for r in rows])} for i,w in enumerate(lanes)},
        'scope':'Cooperative deadlines and exact native telemetry identities. Observed lane spans end at budget checks, not at I/O completion, and can include gaps between calls. Individual blocking calls, selection, copying, skinning and drawing are not preempted. Guest timings are not physical Xbox performance or full-world release acceptance.'}
    a.output.write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result,indent=2))
    raise SystemExit(0 if result['passed'] else 1)

if __name__=='__main__':main()
