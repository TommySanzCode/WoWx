"""Validate the isolated native action/cast batch against its injected timeline."""
import argparse
import csv
import json
import math
from pathlib import Path

p=argparse.ArgumentParser(description=__doc__)
p.add_argument('capture',type=Path)
a=p.parse_args()
rows=list(csv.DictReader(a.capture.open()))
shown=[r for r in rows if r.get('portrait_fixture')=='5']
aux=list(csv.DictReader(a.capture.with_suffix('.actions.csv').open()))

def expected(r):
    frame=int(r['frame']);step=frame//90%12;offset=frame%90;t=offset*33
    spell=phase=duration=elapsed=remaining=delay=infinite=revision=0
    if step in (0,1,2,11):
        spell=133;phase=1;duration=1500 if step==11 else 3000;elapsed=t;revision=1
        if step in (1,11) and offset>=15:duration+=500;delay=500;revision+=1
        edge=45 if step in (1,2) else 55 if step==11 else 90
        if offset>=edge:
            phase=5 if step==1 else 3;duration=800 if step==1 else 350;elapsed=(offset-edge)*33;delay=0;revision+=1
    if step in (7,8,9):
        spell=689;phase=2;duration=3000;elapsed=t;revision=1
        if step==8:
            if offset<45:infinite=1;duration=0xffffffff
            else:duration=1500;elapsed=(offset-45)*33;revision=2
        if step==9 and offset>=30:elapsed=1200+(offset-30)*33;revision=2
        if step==9 and offset>=60:phase=3;duration=350;elapsed=(offset-60)*33;revision=3
    if phase and not infinite and elapsed>=duration:spell=phase=duration=elapsed=delay=0
    remaining=0xffffffff if infinite else duration-elapsed
    fields=dict(step=step,fixture=5,cast_spell=spell,cast_phase=phase,cast_duration=duration,
                cast_elapsed=elapsed,cast_remaining=remaining,cast_delay=delay,cast_infinite=infinite,cast_revision=revision)
    # Actual Vanilla metadata plus explicitly synthetic resource/target/item state.
    if frame>=25:
        if step in (0,1,2,7,8,9,10,11):
            fields.update(action_0_cost=15 if step==11 else 30,action_0_current=0 if step==1 else 20 if step==11 else 1000,
                          action_0_flags=3072+(16 if step==1 else 32 if step==2 else 4 if step==10 else 0),
                          action_0_distance_milli=(77 if step==2 else 47 if step==11 else 17)*1000)
        if step in (3,4):
            fields.update(action_1_cost=150,action_1_current=0 if step==3 else 150,
                          action_1_flags=1024+(16 if step==3 else 0))
            fields['action_4_flags']=3072+(8 if step==3 else 0)
        if step in (3,4,5,6):
            fields.update(action_5_flags=4224 if step in (3,4) else 4096 if step==5 else 12288,
                          action_5_count=0 if step in (3,4) else 3 if step==5 else 1,action_5_charges=4 if step==6 else 0)
    return {k:{'expected':v,'actual':r.get(k)} for k,v in fields.items() if int(r[k])!=v}

errors=[{'frame':r['frame'],'fields':error} for r in aux if (error:=expected(r))]
steps={int(r['step']) for r in aux}
main_frames={int(r['replay_frame']) for r in shown}
checks={
    'samples':len(shown)>=900 and len(aux)>=900,
    'all_twelve_steps':steps==set(range(12)),
    'matching_main_frames':all(int(r['frame']) in main_frames for r in aux),
    'exact_cast_resource_range_item_timeline':bool(aux) and not errors,
    'both_catalogs_v2':bool(aux) and all(r['catalog_version']=='2' for r in aux) and all(r['cooldown_version']=='2' for r in shown),
    'fixed_state':bool(aux) and all(r['cast_state_bytes']=='28' and r['cooldown_state_bytes']=='65336' and r['inventory_bytes']=='8792' for r in aux),
    'offline_only':bool(shown) and all(r['world_active']=='0' and r['login_auth']=='0' and r['login_attempts']=='0' for r in shown),
    'draw_storage_bounded':bool(shown) and all(int(r['ui_quads'])<=2048 and 0<int(r['ui_batches'])<=128 for r in shown),
    'no_failures':bool(shown) and all(r['failures']=='0' and r['cooldown_missing']=='0' and r['cooldown_overflow']=='0' and r['modifier_ambiguous']=='0' for r in shown),
    'headroom':bool(shown) and min(int(r['free_kib']) for r in shown)>=8192,
    'capture_complete':json.loads(a.capture.with_suffix('.lifecycle.json').read_text()).get('end_reason')=='duration_limit',
}
values=sorted(int(r['frame_ms']) for r in shown)
timing={name:values[max(0,math.ceil(len(values)*q)-1)] if values else None for name,q in [('p50',.5),('p95',.95),('p99',.99),('max',1)]}
result={'passed':all(checks.values()),'checks':checks,'samples':len(shown),'action_samples':len(aux),'steps':sorted(steps),
        'minimum_free_kib':min((int(r['free_kib']) for r in shown),default=None),'frame_ms':timing,
        'maximum_ui_quads':max((int(r['ui_quads']) for r in shown),default=None),'mismatches':errors[:8],
        'scope':'64 MiB native-scale xemu, synthetic state and Vanilla packets through production parsers/UI. Item counts/charges are fixture data. No realm login, live combat, physical controller or hardware acceptance.'}
a.capture.with_suffix('.actions-check.json').write_text(json.dumps(result,indent=2)+'\n')
print(json.dumps(result,indent=2));raise SystemExit(not result['passed'])
