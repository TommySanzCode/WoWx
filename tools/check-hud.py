"""Check submitted HUD data in a native capture; screenshots remain required."""
import argparse,csv,json
from pathlib import Path
p=argparse.ArgumentParser(description=__doc__);p.add_argument('capture',type=Path);p.add_argument('--targets',action='store_true');p.add_argument('--portraits',action='store_true');a=p.parse_args()
rows=list(csv.DictReader(a.capture.open()));shown=[r for r in rows if r.get('hud_drawn')=='1']
targets=[r for r in shown if r.get('hud_target_present')=='1']
checks={
 'hud_submitted':len(shown)>=300,
 'world_only':bool(shown) and all(r['world_active']=='1' and r['lobby_phase']=='0' and r['menu']=='0' for r in shown),
 'bounded_ui':bool(shown) and all(r['ui_bytes']=='1376256' and r['ui_failures']=='0' and 0<int(r['hud_quads'])<=300 for r in shown),
 'authoritative_player_values':bool(shown) and all(r['hud_player_health']==r['player_health'] and r['hud_xp']==r['player_xp'] for r in shown),
 'player_resource_available':bool(shown) and any(0<=int(r['hud_player_power_type'])<5 and int(r['hud_player_max_power'])>0 for r in shown),
 'headroom':bool(rows) and min(int(r['free_kib']) for r in rows)>=8192,
 'capture_completed':json.loads(a.capture.with_suffix('.lifecycle.json').read_text()).get('end_reason')=='duration_limit',
}
if a.targets:
 ids={(r['hud_target_low'],r['hud_target_high']) for r in targets}
 last_target=max((int(r['time_ms']) for r in targets),default=0)
 checks.update(two_targets_shown=len(ids)>=2 and ('0','0') not in ids,
               target_cleared=bool(targets) and any(r['hud_target_present']=='0' and r['hud_target_low']=='0' and r['hud_target_high']=='0' and int(r['time_ms'])>last_target for r in shown))
if a.portraits:
 checks.update(portrait_catalog=bool(shown) and all(r.get('portrait_ready')=='1' and r.get('portrait_failures')=='0' and r.get('portrait_fixture')=='0' for r in shown),
               player_portrait=bool(shown) and sum(int(r.get('portrait_player_draws',0))>0 for r in shown)>=300,
               target_portraits=not a.targets or len({(r['hud_target_low'],r['hud_target_high']) for r in targets if int(r.get('portrait_target_draws',0))>0})>=2)
result=dict(passed=all(checks.values()),checks=checks,hud_samples=len(shown),target_samples=len(targets),
            power_types=sorted({int(r['hud_player_power_type']) for r in shown}),
            scope='Native submitted unit/resource data and memory; injected selection is not physical input. Portrait submissions do not prove framing/fidelity. Full PC layout, auras and hardware performance remain unverified.')
a.capture.with_suffix('.hud-check.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result,indent=2));raise SystemExit(not result['passed'])
