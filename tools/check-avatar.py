"""Check native WXTB avatar draw evidence; screenshots remain a separate review."""
import argparse,csv,json
from pathlib import Path
p=argparse.ArgumentParser();p.add_argument('capture',type=Path);p.add_argument('--gear-replay',action='store_true');args=p.parse_args()
rows=list(csv.DictReader(args.capture.open()));last=rows[-1] if rows else {}
visible=[r for r in rows if r.get('world_active')=='1' and int(r.get('avatar_drawn',0))>0]
checks={
    'rendered_player':len(visible)>30,
    'final_avatar_complete':bool(visible) and all(last.get(f)==v for f,v in [('avatar_matched','1'),('avatar_ready','1'),('avatar_missing','0')]) and int(last['avatar_drawn'])>0,
    'animated_vertices':len({r['avatar_pose'] for r in visible if r['avatar_pose']!='0'})>20,
    'bounded_avatar':bool(rows) and max(int(r['avatar_bytes']) for r in rows)<5*1024*1024,
    'camera_in_range':bool(rows) and all(0<=int(r['camera_mm'])<=6000 for r in rows),
    'memory_headroom':bool(rows) and min(int(r['free_kib']) for r in rows)>=8192,
    'no_asset_errors':bool(rows) and all(r['failures']=='0' and r['region_failures']=='0' for r in rows),
}
if args.gear_replay:
    offhand=1<<16;mainhand=1<<15
    removed=[r for r in visible if r['scenario_stage']=='4' and r['equipment_16_entry']=='0']
    checks.update({
        'gear_scenario_passed':last.get('scenario_stage')=='8' and all(r['scenario_kind']=='7' for r in rows),
        'offhand_drawn_before_removal':any(r['scenario_stage']=='2' and int(r['avatar_equipment'])&offhand for r in visible),
        'offhand_removed_while_body_and_weapon_remain':len(removed)>10 and all(not(int(r['avatar_equipment'])&offhand) and int(r['avatar_equipment'])&mainhand for r in removed),
        'offhand_drawn_after_equip':any(r['scenario_stage']=='6' and int(r['avatar_equipment'])&offhand for r in visible),
        'offhand_drawn_after_reconnect':last.get('world_revision')=='2' and bool(int(last.get('avatar_equipment',0))&offhand),
    })
result={'passed':all(checks.values()),'checks':checks,
    'scope':'Native draw submissions and animated vertex changes. Controller replay is client-injected; screenshots, physical controls and Xbox hardware are separate checks.'}
args.capture.with_suffix('.avatar-check.json').write_text(json.dumps(result,indent=2)+'\n')
print(json.dumps(result,indent=2));raise SystemExit(0 if result['passed'] else 1)
