"""Validate the display-only preview replay and its return to the saved world."""
import argparse,csv,json
from pathlib import Path
from preview_validation import customization_checks
p=argparse.ArgumentParser(description=__doc__);p.add_argument('capture',type=Path);p.add_argument('--outfits',action='store_true');p.add_argument('--all-scenes',action='store_true');p.add_argument('--customization-plan',type=Path);a=p.parse_args()
if a.customization_plan:a.outfits=a.all_scenes=True
rows=list(csv.DictReader(a.capture.open()));life=json.loads(a.capture.with_suffix('.lifecycle.json').read_text())
active=[r for r in rows if r['preview_active']=='1'];ready=[r for r in active if r['preview_ready']=='1' and int(r['preview_drawn'])>0]
identities={int(r['preview_identity']) for r in ready};expected={race|(sex<<8) for race in range(1,9) for sex in range(2)}
world=[r for r in rows if r['world_active']=='1'];drafts=[r for r in active if r['character_screen'] in ('1','6')]
draft_start=int(drafts[0]['time_ms']) if drafts else 0;draft_end=int(drafts[-1]['time_ms']) if drafts else 0
initial=[r for r in world if int(r['time_ms'])<draft_start];returned=[r for r in world if int(r['time_ms'])>draft_end]
baseline=initial[-1] if initial else {}
checks={'capture_completed':life.get('end_reason')=='duration_limit' and life.get('guest_exit') is None,
 'all_race_sex_models_drawn':identities==expected,
 'roster_and_creation_drawn':all(any(r['character_screen']==str(screen) for r in ready) for screen in (0,1)),
 'animated_poses':len({r['preview_pose'] for r in ready if r['preview_identity']=='1'})>10,
 'authored_backgrounds':all(any(int(r['preview_identity'])%256==race and r['preview_scene']=='1' for r in ready) for race in (range(1,9) if a.all_scenes else (1,3,5,6,7) if a.outfits else (1,3,7))),
 'rotation_and_zoom':bool(ready) and max(int(r['preview_rotation']) for r in ready)-min(int(r['preview_rotation']) for r in ready)>100 and len({r['preview_zoom'] for r in ready})>10,
 'no_runtime_failures':bool(rows) and all(r['preview_failures']=='0' and r['failures']=='0' for r in rows),
 'stock_memory_headroom':bool(rows) and min(int(r['free_kib']) for r in rows)>=8192,
 'no_preview_outside_lobby':all(r['lobby_phase']=='2' and r['world_active']=='0' for r in active),
 'released_for_world':len(returned)>300 and all(r['preview_active']=='0' and r['preview_bytes']=='0' and r['preview_ready']=='0' for r in returned),
 'saved_character_restored':bool(returned) and bool(baseline) and all(all(r[k]==baseline[k] for k in ('player_guid_low','player_guid_high','player_xp','inventory_money','player_level','map')) and all(abs(float(r[k])-float(baseline[k]))<.01 for k in ('x','y','z')) for r in returned[-300:]),
 'no_character_creation':bool(rows) and all(r['character_result_revision']=='0' for r in rows)}
if a.outfits:
 masks=(0,0x336,0x29a,0x3e,0x83a,0x332,0x88a,0x312,0x1ba)
 expected_outfits={(race,cl,sex) for race in range(1,9) for cl in range(1,12) for sex in range(2) if masks[race]&(1<<cl)}
 shown={(int(r['preview_identity'])&255,int(r.get('preview_class',0)),int(r['preview_identity'])>>8) for r in ready if r['character_screen']=='1' and r.get('preview_outfit')=='1' and r['preview_equipment_missing']=='0'}
 checks.update(all_80_starter_outfits_drawn=shown==expected_outfits,
  bounded_animation_index=bool(ready) and all(r.get('preview_index_reads')=='1' for r in ready),
  incremental_loading_exercised=any(int(r.get('preview_loading',0))>0 for r in active),
  combined_login_completed=any(r.get('login_test_stage')=='16' for r in rows))
missing_customization=[]
if a.customization_plan:
 customization,missing_customization=customization_checks(ready,json.loads(a.customization_plan.read_text()))
 checks.update(customization)
def timing(group):
 values=sorted(int(r['frame_ms']) for r in group)
 return {k:values[min(int(len(values)*q),len(values)-1)] if values else None for k,q in [('p50',.5),('p95',.95),('p99',.99),('max',1)]}
result={'passed':all(checks.values()),'checks':checks,'samples':len(rows),'ready_preview_samples':len(ready),'identities':sorted(identities),
 'minimum_free_kib':min((int(r['free_kib']) for r in rows),default=0),'preview_timing_ms':timing(ready),'all_preview_timing_ms':timing(active),'world_timing_ms':timing(returned),
 'missing_customization':missing_customization,
 'scope':'Internal controller replay; physical input/hardware unverified. Customization is checked only with --customization-plan. Background coverage is selected by --all-scenes/--outfits. Rendered-look telemetry and composed atlas hashes do not establish PC visual parity; screenshots assess visual correctness separately.'}
a.capture.with_suffix('.preview-check.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result,indent=2));raise SystemExit(not result['passed'])
