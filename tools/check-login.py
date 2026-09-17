"""Validate the native account/realm replay or an untouched manual-login boot."""
import argparse,csv,json,math
from pathlib import Path
p=argparse.ArgumentParser(description=__doc__);p.add_argument('capture',type=Path);p.add_argument('--menu',action='store_true');p.add_argument('--baseline',type=Path);a=p.parse_args()
rows=list(csv.DictReader(a.capture.open()));last=rows[-1] if rows else {};lifecycle_path=a.capture.with_suffix('.lifecycle.json')
lifecycle=json.loads(lifecycle_path.read_text()) if lifecycle_path.exists() else {}
checks={'samples':len(rows)>60,'native_lifecycle':lifecycle.get('end_reason')=='duration_limit' and lifecycle.get('guest_exit') is None,
 'atlas_and_headroom':bool(rows) and all(r.get('ui_ready')=='1' and r.get('ui_bytes')=='1376256' and int(r['free_kib'])>=8192 for r in rows),
 'bounded_draws':bool(rows) and all(0<=int(r.get('ui_quads',-1))<=2048 for r in rows),
 'no_failures':bool(rows) and all(all(r[k]=='0' for k in ('failures','ui_failures','region_failures','text_failures','avatar_select_failures')) for r in rows)}
if a.menu:
 checks.update({'manual_menu':bool(rows) and all(r['login_mode']=='1' and r['login_phase']=='1' and r['login_attempts']=='0' and r['login_test_stage']=='0' and r['world_active']=='0' for r in rows),
  'menu_rendered':bool(rows) and all(int(r['ui_quads'])>40 for r in rows)})
else:
 checks.update({'fixture_mode':bool(rows) and all(r['login_mode']=='2' for r in rows),
  'keyboard_visited':any(r['login_keyboard']=='1' for r in rows),
  'rejected_login_handled':any(r['login_phase']=='5' and r['login_auth']=='8' for r in rows),
  'realms_received':any(r['login_phase']=='3' and int(r['login_count'])>0 for r in rows),
  'cancel_and_retry':last.get('login_attempts')=='3' and last.get('login_cancellations')=='1' and any(r['login_phase']=='1' and r['login_cancellations']=='1' for r in rows),
  'character_roster':any(r['lobby_phase']=='2' and int(r['lobby_count'])>0 for r in rows),
  'entered_saved_world':last.get('login_test_stage')=='16' and last.get('world_active')=='1' and last.get('lobby_phase')=='0',
  'no_replay_failure':all(r['login_test_stage']!='99' for r in rows)})
 if a.baseline:
  before=list(csv.DictReader(a.baseline.open()))[-1]
  fields=['player_guid_low','player_guid_high','player_level','player_xp','inventory_money','inventory_count','active_quests']+[f'equipment_{i}_{k}' for i in range(19) for k in ('entry','display','type')]
  checks['saved_progress']=bool(rows) and all(last[k]==before[k] for k in fields)
  checks['saved_position']=bool(rows) and all(abs(float(last[k])-float(before[k]))<.01 for k in ('x','y','z')) and abs(math.remainder(float(last['yaw'])-float(before['yaw']),2*math.pi))<.01
result={'passed':all(checks.values()),'checks':checks,'samples':len(rows),'scope':'Native injected controller login/error/realm/cancel/world flow or idle manual menu. Screenshots and physical controller transport require separate verification; no credentials in telemetry.'}
a.capture.with_suffix('.login-check.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result,indent=2));raise SystemExit(not result['passed'])
