"""Check native staged table copies and publication with exact frame identities."""
import argparse
import csv
import json
from pathlib import Path
from stream_telemetry import full_coverage


def read(path):
    with path.open(newline='') as stream:
        return [{k:float(v) for k,v in row.items()} for row in csv.DictReader(stream)]


def distribution(values):
    values=sorted(values)
    return {name:values[int((len(values)-1)*q)] if values else None
            for name,q in [('p50',.5),('p95',.95),('p99',.99),('max',1)]}


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('capture',type=Path)
    parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args()
    frames=read(args.capture);rows=read(args.capture.with_suffix('.stream.csv'))
    coverage=full_coverage(frames,rows);errors=[];commits=[]
    if not coverage['passed']:errors.append('Incomplete, duplicate or foreign streaming identities')
    by_id={(r['time_ms'],r['replay_frame']):i for i,r in enumerate(frames)}
    copy_frames=[r for r in rows if r.get('index_copy_bytes',-1)>0]
    if not copy_frames:errors.append('No staged native copies exercised')
    if not frames or min(r['free_kib'] for r in frames)<8192:
        errors.append('Missing native memory samples or insufficient headroom')
    for i,r in enumerate(rows):
        copied=r.get('index_copy_bytes',-1);phase=r['index_phase']
        total=r.get('index_join_total',-1);progress=r.get('index_join_progress',-1)
        resident=r.get('index_join_bytes',-1)
        if copied<0 or copied>262144 or copied%64 or not 0<=progress<=total:
            errors.append('Invalid staged copy quota/progress');break
        if resident!=(total if phase in (4,5) else 0) or r['pending_index_bytes']<resident:
            errors.append('Unpublished allocation accounting mismatch');break
        if copied and phase not in (4,5):
            errors.append('Copy and publication/cancellation shared a frame');break
        if phase==5 and progress!=total:
            errors.append('Ready table contains incomplete copy');break
        if not i:continue
        before=rows[i-1]
        if before['index_phase'] in (4,5) and phase in (4,5):
            if total!=before['index_join_total'] or r['index_bytes']!=before['index_bytes']:
                errors.append('Published table changed while join was pending');break
            if progress-before['index_join_progress']!=copied:
                errors.append('Native copy progress differs from granted bytes');break
        if before['index_phase']==5 and phase==0 and r['index_bytes']==before['index_join_total']:
            if copied:errors.append('Publication frame also copied index bytes')
            frame=by_id.get((r['time_ms'],r['frame']))
            commits.append({'frame':r['frame'],'stage':r['stage'],'cycle':r['cycles'],
                            'published_bytes':r['index_bytes'],'region_ms':r['region_ms'],
                            'selection_ms':r['stream_ms'],
                            'next_frame_ms':frames[frame+1]['frame_ms'] if frame is not None and frame+1<len(frames) else None})
    if len(commits)<2:errors.append('Fewer than two complete native joined-table publications')
    result={'passed':not errors,'errors':errors,'coverage':coverage,
            'minimum_free_kib':min((r['free_kib'] for r in frames),default=None),
            'copy_frames':len(copy_frames),'maximum_copy_bytes':max((r['index_copy_bytes'] for r in copy_frames),default=0),
            'copy_region_ms':distribution([r['region_ms'] for r in copy_frames]),
            'frame_ms':distribution([r['frame_ms'] for r in frames[1:]]),'publications':commits,
            'scope':'Offline terrain fixture in xemu. Atomic native index publication and bounded copies; whole-buffer allocation, detach, selection and draw remain blocking. No physical Xbox, controller or combined gameplay acceptance.'}
    args.output.write_text(json.dumps(result,indent=2)+'\n')
    print(json.dumps(result,indent=2))
    raise SystemExit(0 if result['passed'] else 1)


if __name__=='__main__':main()
