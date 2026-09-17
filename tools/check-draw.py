"""Check exact native geometry accounting and report guest submission/wait times."""
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
    frames=read(a.capture);rows=read(a.capture.with_suffix('.draw.csv'));streams=read(a.capture.with_suffix('.stream.csv'))
    errors=[];coverage=full_coverage(frames,rows);stream_coverage=full_coverage(frames,streams)
    if not coverage['passed'] or not stream_coverage['passed']:errors.append('Missing, duplicate or foreign companion identities')
    main_ids={(r['time_ms'],r['replay_frame']):r for r in frames}
    stream_ids={(r['time_ms'],r['frame']):r for r in streams};joined=[]
    for r in rows:
        key=(r['time_ms'],r['frame']);m=main_ids.get(key);s=stream_ids.get(key)
        if not m or not s:continue
        if r['scope']!=1 or r['fixture'] not in (7,8,11,13) or r['fixture']!=m['portrait_fixture']:
            errors.append('Expected matching offline fixture with immediate drain');break
        if r['indices']!=3*m['triangles'] or r['batches']<m['draws']:
            errors.append('Submitted indices differ from drawn geometry');break
        # The enclosing legacy timer may have an extra millisecond at its two boundaries.
        if not 0<=m['profile_draw_ms']-r['submit_ms']-r['drain_ms']<=2:
            errors.append('Submission plus drain does not match enclosing draw timer');break
        joined.append(dict(r,stage=s['stage'],cycle=s['cycles'],draw_ms=m['profile_draw_ms'],work_ms=m['profile_work_ms'],
                           region_ms=s['region_ms'],stream_ms=s['stream_ms'],index_phase=s['index_phase'],selection=s['selection_entries']))
    if not frames or min(r['free_kib'] for r in frames)<8192:errors.append('Missing or insufficient measured headroom')
    if not rows or max((r['largest_batch'] for r in rows),default=0)<=240:errors.append('Larger index packets were not exercised')
    before=sum(r['legacy_batches'] for r in rows);after=sum(r['batches'] for r in rows)
    if not before or after>=before:errors.append('No reduction in geometry index submissions')
    result={'passed':not errors,'errors':errors,'coverage':coverage,'stream_coverage':stream_coverage,
            'minimum_free_kib':min((r['free_kib'] for r in frames),default=None),'samples':len(rows),
            'index_batches':{'previous_240_index_rule':before,'submitted':after,'reduction_percent':100*(1-after/before) if before else None},
            'costs':{k:distribution([r[k] for r in rows]) for k in ('submit_ms','flush_wait_ms','flush_reset_ms','drain_ms','world_ms','actors_ms','player_ms','liquid_ms','flushes')},
            'submission_excluding_explicit_waits_ms':distribution([r['submit_ms']-r['flush_wait_ms']-r['flush_reset_ms'] for r in rows]),
            'frame_ms':distribution([r['frame_ms'] for r in frames[1:]]),
            'worst_draw_frames':sorted(joined,key=lambda r:r['draw_ms'],reverse=True)[:12],
            'scope':'Native xemu guest wall times. Submission includes pb_end cache flush/MMIO and emulator scheduling; the residual is not a pure CPU measurement. Final drain is GPU completion observed from the guest, not a hardware GPU timestamp. Batch reduction compares exact current geometry with the former 240-index rule; it does not establish an FPS improvement. Offline fixture; no input/controller, live gameplay or physical-console acceptance.'}
    a.output.write_text(json.dumps(result,indent=2)+'\n',encoding='utf-8');print(json.dumps({k:v for k,v in result.items() if k!='worst_draw_frames'},indent=2));raise SystemExit(bool(errors))

if __name__=='__main__':main()
