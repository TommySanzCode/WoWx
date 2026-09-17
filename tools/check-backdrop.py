"""Check native title allocation, animation and release. Visual parity is separate."""
import argparse,csv,json,statistics,struct
from pathlib import Path
p=argparse.ArgumentParser(description=__doc__);p.add_argument('capture',type=Path);p.add_argument('--world-transition',action='store_true');p.add_argument('--pack',type=Path,default=Path('build/xbox/TITLE.WXB'));a=p.parse_args()
data=a.pack.read_bytes();h=struct.unpack_from('<4s21I9f',data)
version,size,vertices,indices,batches,textures=h[1],h[2],h[4],h[6],h[10],h[12]
texture_offset=h[13];gpu=sum((struct.unpack_from('<6I',data,texture_offset+i*24)[1]+127)&~127 for i in range(textures))
expected=size+gpu+vertices*32;emitters=lights=capacity=0
if version>=2:
 emitters,emitter_offset,lights,light_offset,color_offset,capacity=struct.unpack_from('<6I',data,h[21])
 expected+=(2048*36+64*12+9*4+capacity*4*36 if emitters else 0)+(vertices*17+8*52 if lights else 0)
rows=list(csv.DictReader(a.capture.open()));life=a.capture.with_suffix('.lifecycle.json')
lifecycle=json.loads(life.read_text()) if life.exists() else {}
menu=[r for r in rows if r.get('backdrop_ready')=='1'];world=[r for r in rows if r.get('world_active')=='1']
checks={'completed_capture':lifecycle.get('end_reason')=='duration_limit' and lifecycle.get('guest_exit') is None,
 'title_drawn':len(menu)>300 and all(1<=int(r['backdrop_draws'])<=batches+emitters and 0<int(r['backdrop_triangles'])<=indices//3+capacity*2 for r in menu),
 'fixed_allocation':bool(menu) and all(int(r['backdrop_bytes'])==expected for r in menu),
 'animated_updates':bool(menu) and int(menu[-1]['backdrop_updates'])>int(menu[0]['backdrop_updates'])+300,
 'no_failures':bool(rows) and all(r.get('backdrop_failures')=='0' and r.get('failures')=='0' for r in rows),
 'memory_headroom':bool(rows) and min(int(r['free_kib']) for r in rows)>=8192,
 'title_only_frontend':bool(rows) and all(r.get('world_active')=='0' and r.get('lobby_phase')=='0' for r in menu)}
if emitters:
 checks['effects_active']=len([r for r in menu if int(r.get('effects_live',0))>500])>300
 checks['pool_bounded']=bool(menu) and all(int(r['effects_live'])<=capacity and int(r['effects_quads'])==int(r['effects_live']) and int(r['effects_peak'])<=capacity and r['effects_drops']=='0' for r in menu)
 checks['authored_emitters_and_lights']=bool(menu) and all(int(r['effects_emitters'])==emitters and int(r['effects_lights'])==lights for r in menu)
 if a.world_transition:checks['effects_released']=bool(world) and all(all(r[k]=='0' for k in ('effects_emitters','effects_lights','effects_live','effects_quads')) for r in world)
if a.world_transition:checks['released_for_world']=len(world)>300 and all(r['backdrop_ready']=='0' and r['backdrop_bytes']=='0' for r in world)
intervals=sorted(int(r['frame_ms']) for r in rows)
result={'passed':all(checks.values()),'checks':checks,'samples':len(rows),'expected_bytes':expected,'peak_particles':max([int(r.get('effects_peak',0)) for r in menu],default=0),'min_free_kib':min([int(r['free_kib']) for r in rows],default=0),
 'guest_intervals_ms':{k:intervals[min(int(len(intervals)*q),len(intervals)-1)] if intervals else None for k,q in [('p50',.5),('p95',.95),('p99',.99),('max',1)]},
 'scope':'Native geometry, independent animation updates, fixed CPU/GPU allocation and lifecycle. Screenshots validate framing separately. Effects counters verify bounded runtime, not exact PC visual parity; advanced motion, sprite timing, exact UI and hardware remain open.'}
a.capture.with_suffix('.backdrop-check.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result,indent=2));raise SystemExit(not result['passed'])
