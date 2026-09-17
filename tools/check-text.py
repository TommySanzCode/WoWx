"""Validate bounded native text rendering and presentation phase measurements."""
import argparse,csv,json,statistics
from pathlib import Path
p=argparse.ArgumentParser(description=__doc__);p.add_argument('capture',type=Path)
p.add_argument('--legacy',action='store_true');p.add_argument('--baseline',action='store_true');p.add_argument('--walking',action='store_true');a=p.parse_args()
rows=list(csv.DictReader(a.capture.open()))
selected=[r for r in rows if not a.walking or int(r['scenario_stage']) in (20,21,28,29)]
parts=['profile_'+x+'_ms' for x in ('scene_wait','text_submit','text_wait','swap')]
fields=bool(rows) and all(k in rows[0] for k in parts+['text_glyphs','text_mode','text_bytes','text_failures'])
checks={
 'measured_frames':fields and bool(selected) and any(int(r['profile_present_ms']) for r in selected),
 'presentation_intervals':fields and bool(selected) and all(sum(int(r[k]) for k in parts)==int(r['profile_present_ms']) for r in selected),
 'headroom':bool(rows) and min(int(r['free_kib']) for r in rows)>=8192,
 'no_font_errors':fields and bool(rows) and all(r['text_failures']=='0' for r in rows),
}
if not a.baseline:
 checks.update({
  'expected_backend':fields and bool(rows) and all(r['text_mode']==str(0 if a.legacy else 1) for r in rows),
  'bounded_storage':fields and bool(rows) and all(int(r['text_bytes'])==(0 if a.legacy else 157696) for r in rows),
  'visible_glyphs':fields and bool(rows) and max(int(r['text_glyphs']) for r in rows)>40 and all(0<=int(r['text_glyphs'])<=960 for r in rows),
 })
timing={}
if fields:
 for k in parts+['profile_present_ms','profile_work_ms']:
  v=sorted(int(r[k]) for r in selected)
  if v:timing[k]={'mean':statistics.mean(v),'p50':v[(len(v)-1)//2],'p95':v[int((len(v)-1)*.95)],'max':max(v)}
result={'passed':all(checks.values()),'checks':checks,'samples':len(selected),'milliseconds':timing,
 'scope':'Current-frame guest wall times for scene drain, text submission/drain and swap. These intervals do not isolate CPU cost. Actual screenshots and controller UI checks remain separate.'}
a.capture.with_suffix('.text-check.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result,indent=2));raise SystemExit(not result['passed'])
