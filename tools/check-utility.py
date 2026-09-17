"""Check the native utility wheel, destinations and gameplay input capture."""
import argparse,csv,json
from pathlib import Path
p=argparse.ArgumentParser(description=__doc__);p.add_argument('capture',type=Path);a=p.parse_args()
rows=list(csv.DictReader(a.capture.open()));last=rows[-1] if rows else {}
settled=[r for r in rows if r['world_active']=='1' and int(r['player_level']) and int(r['inventory_count']) and int(r['replay_frame'])<600]
first=settled[-1] if settled else {}
wheel=[r for r in rows if r['utility_open']=='1']
held=[r for r in rows if r['utility_open']=='1' or r['utility_pending']=='1']
checks={
 'wheel_visible':len(wheel)>400,
 'all_directions_selected':{0,1,2,3,4}<={int(r['utility_selection']) for r in wheel},
 'bags_opened':any(r['inventory_open']=='1' and r['utility_action']=='1' for r in rows),
 'spellbook_opened':any(r['book_open']=='1' and r['utility_action']=='2' for r in rows),
 'journal_opened':any(r['journal_open']=='1' and r['utility_action']=='3' for r in rows),
 'settings_opened':any(r['menu']=='1' and r['utility_action']=='4' for r in rows),
 'six_deliberate_openings':last.get('utility_revision')=='6',
 'no_action_leak':bool(rows) and all(r['action']=='-1' and r['casts_accepted']=='0' and r['casts_rejected']=='0' for r in rows),
 'no_panel_behind_wheel':bool(held) and all(r['actionbar_layer']=='0' for r in held),
 'no_movement_or_camera_leak':bool(first) and all(abs(float(r[k])-float(first[k]))<.01 for r in rows if int(r['replay_frame'])>=600 for k in ('x','y','z','yaw','pitch')),
 'progress_preserved':bool(first) and all(last[k]==first[k] for k in ('player_guid_low','player_level','player_xp','inventory_money','inventory_count')),
 'closed_after_cancel':last.get('replay_active')=='0' and all(last.get(k)=='0' for k in ('utility_open','utility_pending','inventory_open','book_open','journal_open','menu')),
 'headroom':bool(rows) and min(int(r['free_kib']) for r in rows)>=8192,
 'no_failures':bool(rows) and all(all(r[k]=='0' for k in ('failures','region_failures','book_failures','avatar_select_failures')) for r in rows),
 'no_packet_faults':not a.capture.with_suffix('.faults.jsonl').exists(),
}
result={'passed':all(checks.values()),'checks':checks,'scope':'Injected native pad input; UI destinations, movement/cast isolation and unchanged saved progress. Physical controller and hardware remain separate.'}
a.capture.with_suffix('.utility-check.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result,indent=2));raise SystemExit(not result['passed'])
