"""Validate deterministic offline cooldown packet injection and native UI metrics."""
import argparse,csv,json,math
from pathlib import Path
p=argparse.ArgumentParser(description=__doc__);p.add_argument('capture',type=Path);a=p.parse_args()
rows=list(csv.DictReader(a.capture.open()))
shown=[r for r in rows if r.get('portrait_fixture')=='3']
steps={int(r['cooldown_fixture_step']) for r in shown}
def expected(r):
    frame=int(r['replay_frame']);step=frame//90%8;delta=(frame%90)*33
    timers=[0]*8;held=0
    if step==0:timers[1]=5000-delta;held=16
    if step in (0,3,4,6):timers[5]=3600000-(frame%720)*33
    if step in (1,2):timers[0]=7000-(step-1)*2970-delta
    if step in (3,4):timers[4]=15000-delta
    layer=2 if step in (1,2,5) else 3 if step==7 else 1
    return step==int(r['cooldown_fixture_step']) and layer==int(r['actionbar_layer']) and held==int(r['cooldown_held_mask']) and all(int(r[f'cooldown_remaining_{i}'])==timers[i] for i in range(8))
checks={
    'samples':len(shown)>=500,'all_eight_steps':steps==set(range(8)),
    'offline_only':bool(shown) and all(r['world_active']=='0' and r['login_auth']=='0' and r['login_attempts']=='0' for r in shown),
    'exact_timer_and_layer_state':bool(shown) and all(expected(r) for r in shown),
    'catalog_and_state_bounded':bool(shown) and all(r['cooldown_ready']=='1' and r['cooldown_count']=='22357' and (r['cooldown_bytes'],r['cooldown_state_bytes']) in [('447140','45076'),('1251992','60216'),('1251992','65336')] for r in shown),
    'packet_publication':bool(shown) and all(int(r['cooldown_packets'])==int(r['replay_frame'])//90+1 and r['cooldown_revision']==r['cooldown_packets'] for r in shown),
    'draw_storage_bounded':bool(shown) and all(int(r['ui_quads'])<=2048 and 0<int(r['ui_batches'])<=128 for r in shown),
    'no_failures':bool(shown) and all(r['failures']=='0' and r['cooldown_missing']=='0' and r['cooldown_overflow']=='0' for r in shown),
    'headroom':bool(shown) and min(int(r['free_kib']) for r in shown)>=8192,
    'capture_complete':json.loads(a.capture.with_suffix('.lifecycle.json').read_text()).get('end_reason')=='duration_limit',
}
values=sorted(int(r['frame_ms']) for r in shown)
timing={name:values[max(0,math.ceil(len(values)*q)-1)] if values else None for name,q in [('p50',.5),('p95',.95),('p99',.99),('max',1)]}
result={'passed':all(checks.values()),'checks':checks,'samples':len(shown),'steps':sorted(steps),
    'minimum_free_kib':min((int(r['free_kib']) for r in shown),default=None),'frame_ms':timing,
    'maximum_ui_quads':max((int(r['ui_quads']) for r in shown),default=None),
    'scope':'Stock-memory xemu; deterministic synthetic Vanilla packets and injected UI state, production parser/query/UI. No realm login, physical input, live class combat or hardware acceptance.'}
a.capture.with_suffix('.cooldowns-check.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result,indent=2));raise SystemExit(not result['passed'])
