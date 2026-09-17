"""Validate native selection progress, quotas and exact telemetry coverage."""
import argparse,csv,json
from pathlib import Path
from stream_telemetry import full_coverage

def read(path):
    with path.open(newline='') as stream:return [{k:float(v) for k,v in r.items()} for r in csv.DictReader(stream)]

def distribution(values):
    v=sorted(values)
    return {name:v[int((len(v)-1)*q)] if v else None for name,q in [('p50',.5),('p95',.95),('p99',.99),('max',1)]}

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('capture',type=Path);p.add_argument('--output',type=Path,required=True);a=p.parse_args()
    frames=read(a.capture);rows=read(a.capture.with_suffix('.stream.csv'));errors=[];commits=[]
    coverage=full_coverage(frames,rows)
    if not coverage['passed']:errors.append('Missing, duplicate or foreign native companion identities')
    if not frames or min(r['free_kib'] for r in frames)<8192:errors.append('Missing or insufficient measured headroom')
    active=[r for r in rows if r.get('selection_entries',0)>0]
    if len(active)<20:errors.append('Too few native staged selection frames')
    for i,r in enumerate(rows):
        entries=r.get('selection_entries',-1);phase=r.get('selection_phase',-1)
        progress=r.get('selection_progress',-1);total=r.get('selection_total',-1)
        if entries<0 or entries>2048 or phase not in (0,1) or not 0<=progress<=total<=131072:
            errors.append('Invalid selection quota/state');break
        if phase and (r['ready'] or r['selection_revision']!=r['scene_revision']):
            errors.append('Incomplete or obsolete snapshot published as ready');break
        if not i:continue
        before=rows[i-1]
        same=before['selection_phase']==1 and before['selection_revision']==r['selection_revision'] and before['selection_restarts']==r['selection_restarts']
        if same and progress-before['selection_progress']!=entries:
            errors.append('Selected entry count differs from native progress');break
        if same and r['selection_commits']==before['selection_commits']+1:
            if phase or progress!=total:errors.append('Incomplete selection committed')
            commits.append({'frame':r['frame'],'stage':r['stage'],'cycle':r['cycles'],'entries':total,'stream_ms':r['stream_ms']})
    if len(commits)<3:errors.append('Fewer than three native multi-frame selections committed')
    result={'passed':not errors,'errors':errors,'coverage':coverage,
            'minimum_free_kib':min((r['free_kib'] for r in frames),default=None),
            'selection_frames':len(active),'maximum_entries_per_frame':max((r['selection_entries'] for r in active),default=None),
            'selection_frame_stream_ms':distribution([r['stream_ms'] for r in active]),
            'frame_ms':distribution([r['frame_ms'] for r in frames[1:]]),'commits':commits,
            'scope':'Offline xemu workload; fixture IDs in the stream companion identify camera-only or combined traversal cases. Stream timing includes selection, payload work and animation; no physical controller, stock-console or full-game acceptance.'}
    a.output.write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result,indent=2));raise SystemExit(0 if result['passed'] else 1)

if __name__=='__main__':main()
