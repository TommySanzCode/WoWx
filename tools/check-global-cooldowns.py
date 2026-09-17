"""Check the isolated native GCD/modifier fixture against its injected timeline."""
import argparse
import csv
import json
import math
from pathlib import Path

p=argparse.ArgumentParser(description=__doc__)
p.add_argument('capture',type=Path)
a=p.parse_args()
rows=list(csv.DictReader(a.capture.open()))
shown=[r for r in rows if r.get('portrait_fixture')=='4']

def expected(r):
    frame=int(r['replay_frame']);cycle=frame//720;step=frame//90%8;offset=frame%90;delta=offset*33
    duration=1000 if step==3 else 1200 if step==2 else 1500
    left=step in (5,6)
    remaining=0 if left or (step==1 and offset>=15) else max(0,duration-delta)
    timers=[remaining]*8;mask=255 if remaining else 0
    if step==4:
        timers[1]=4000-delta;mask&=~2
    if left:
        timers[4]=(7500 if step==5 else 15000)-delta
    packets=[2,3+(offset>=15),6+(offset>=15)+(offset>=20),10,11,14,17,19][step]+cycle*19
    fields={
        'cooldown_fixture_step':step,'actionbar_layer':1 if left else 2,
        'cooldown_held_mask':0,'gcd_mask':mask,
        'cooldown_packets':packets,'cooldown_revision':packets,
        'modifier_updates':cycle*4+[0,0,1,2,2,3,4,4][step],
        'gcd_starts':cycle*6+[1,2,3,4,5,5,5,6][step],
        'gcd_cancels':cycle+(step>1 or (step==1 and offset>=15)),
        'cast_speed_milli':500 if step==3 else 1000,'spellmod_family':4 if left else 3,
    }
    fields.update({f'cooldown_remaining_{i}':timers[i] for i in range(8)})
    return {key:{'expected':value,'actual':r.get(key)} for key,value in fields.items() if int(r[key])!=value}

errors=[{'frame':r['replay_frame'],'fields':error} for r in shown if (error:=expected(r))]
steps={int(r['cooldown_fixture_step']) for r in shown}
checks={
    'samples':len(shown)>=500,'all_eight_steps':steps==set(range(8)),
    'cast_and_cancel_edges':all(any(int(r['cooldown_fixture_step'])==step and low<=int(r['replay_frame'])%90<high for r in shown) for step,low,high in [(1,0,15),(1,15,90),(2,0,15),(2,15,20),(2,20,90)]),
    'offline_only':bool(shown) and all(r['world_active']=='0' and r['login_auth']=='0' and r['login_attempts']=='0' for r in shown),
    'exact_timers_modifiers_context':bool(shown) and not errors,
    'catalog_and_state_bounded':bool(shown) and all(r['cooldown_ready']=='1' and r['cooldown_version']=='2' and r['cooldown_count']=='22357' and r['cooldown_bytes']=='1251992' and r['cooldown_state_bytes'] in ('60216','65336') for r in shown),
    'draw_storage_bounded':bool(shown) and all(int(r['ui_quads'])<=2048 and 0<int(r['ui_batches'])<=128 for r in shown),
    'no_failures':bool(shown) and all(r['failures']=='0' and r['cooldown_missing']=='0' and r['cooldown_overflow']=='0' and r['modifier_ambiguous']=='0' for r in shown),
    'headroom':bool(shown) and min(int(r['free_kib']) for r in shown)>=8192,
    'capture_complete':json.loads(a.capture.with_suffix('.lifecycle.json').read_text()).get('end_reason')=='duration_limit',
}
values=sorted(int(r['frame_ms']) for r in shown)
timing={name:values[max(0,math.ceil(len(values)*q)-1)] if values else None for name,q in [('p50',.5),('p95',.95),('p99',.99),('max',1)]}
result={'passed':all(checks.values()),'checks':checks,'samples':len(shown),'steps':sorted(steps),
    'minimum_free_kib':min((int(r['free_kib']) for r in shown),default=None),'frame_ms':timing,
    'maximum_ui_quads':max((int(r['ui_quads']) for r in shown),default=None),'mismatches':errors[:8],
    'scope':'64 MiB native-scale xemu; synthetic Vanilla packets/context and injected UI state through production cooldown/parser/UI. No realm login, physical controller, live class combat or physical hardware acceptance.'}
a.capture.with_suffix('.gcd-check.json').write_text(json.dumps(result,indent=2)+'\n')
print(json.dumps(result,indent=2));raise SystemExit(not result['passed'])
